/*
 * simdate: will print the time of the simulator (when using the simulator).
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>  /* Map call and definitions */
#include <pthread.h>
#include <semaphore.h> /* For sem_open */
#include <time.h>      /* For ctime */
#include <fcntl.h>     /* O_RDWR */
#include <getopt.h>    /* getopt_long call */

#include "src/common/sim_funcs.h"

#define SLURM_SIM_SHM "/tester_slurm_sim.shm"

/* Offsets */
#define SIM_SECONDS_OFFSET           0
#define SIM_MICROSECONDS_OFFSET      4
#define SIM_GLOBAL_SYNC_FLAG_OFFSET 16
#define SIM_PTHREAD_SLURMCTL_PID     8 
#define SIM_PTHREAD_SLURMD_PID      12


/* Pointers to shared memory data */
void *         timemgr_data;
unsigned int * current_sim;
unsigned int * current_micro;
int  *         slurmctl_pid;
int  *         slurmd_pid;
char *         global_sync_flag;

/* Global variables supporting debugging functionality */
char    SEM_NAME[]           ="serversem";
sem_t * mutexserver          = SEM_FAILED;
int     incr                 = 0;
char    new_global_sync_flag = ' ';

/* Funtions */
       int    open_global_sync_sem();
       void   close_global_sync_sem();
const  char * _format_date();


static struct option long_options[] = {
	{"showmem",		0, 0, 's'},
	{"timeincr",		1, 0, 'i'},
	{"flag",		1, 0, 'f'},
	{"help",		0, 0, 'h'}
};
char optstr[] = "si:f:h";
char help_msg[] = "simdate\n"
		  "\t[-s | --showmem]\n"
		  "\t[-i | --timeincr <seconds>]\n"
		  "\t[-f | --flag <1-4 | '*'>]\n"
		  "\t[-h | --help]`\n"
		  "\n\nNotes: The simdate command's primary function is to\n"
		  "display the current simulated time.  If no arguments are\n"
		  "given then this is the output.  However, it also serves to\n"
		  "both display the synchronization semaphore value\n"
		  "and the contents of the shared memory segment.\n"
		  "Furthermore, it can increment the simulated time and set\n"
		  "the global sync flag.\n";
char valid_sync_flags[] = {'C', 'S', '*'};


void __attribute__ ((constructor)) simdate_init(void) {
    if(attaching_shared_memory() < 0) {
        printf("Error building shared memory and mmaping it\n");
    };
}


/* Semaphore Functions */
void
close_global_sync_sem() {
	if(mutexserver != SEM_FAILED) sem_close(mutexserver);
}

int
open_global_sync_sem() {
	int iter = 0;
	while(mutexserver == SEM_FAILED && iter < 1 /*10*/) {
		mutexserver = sem_open(SEM_NAME,0,0644,0);
		/*if(mutexserver == SEM_FAILED) sleep(1);*/
		++iter;
	}

	if(mutexserver == SEM_FAILED)
		return -1;
	else
		return 0;
}

/* Shared Memory Segment Functions */

const char*
_format_date() {
	time_t now = *current_sim;
	return ctime(&now);
}

/* Returns 1 if valid and 0 invalid */
int validate_sync_flag() {
	int ix, rv = 0;
#if 0
	int num_flags = sizeof(valid_sync_flags)/sizeof(char);

	for(ix=0; ix<num_flags; ix++)
		if (new_global_sync_flag == valid_sync_flags[ix])
			rv = 1;
#else
	if ((new_global_sync_flag>=1 && new_global_sync_flag<=4) ||
				new_global_sync_flag == '*')
			rv = 1; 
#endif

	return rv;
}

int
main (int argc, char *argv[]) {
	int val = -22;
	int option_index, opt_char, show_mem = 0;

	if (argc == 1) {
		printf("%s", _format_date());
		return 0;
	}

	while ((opt_char = getopt_long(argc, argv, optstr, long_options,
						&option_index)) != -1) {
		switch (opt_char) {
		case ('s'): show_mem = 1;                     break;
		case ('i'): incr = atoi(optarg);              break;
		case ('f'):
			if (optarg[0] == '*')
				new_global_sync_flag = optarg[0];
			else
				new_global_sync_flag = atoi(optarg);

			if (!validate_sync_flag()) {
				printf("Error! Invalid Sync Flag.\n");
				return -1;
			}
			break;
		case ('h'): printf("%s", help_msg);           return 0;
		};
	}

	/* Display semaphore sync value if requested via command-line option */
	if (show_mem) {
		open_global_sync_sem();
		if(mutexserver!=SEM_FAILED) sem_getvalue(mutexserver, &val);
		printf("Global Sync Semaphore Value: %d\n", val);
		close_global_sync_sem();
	}

	/* Get Semaphore Information if it exists */
	if (timemgr_data) {
		current_sim      = timemgr_data + SIM_SECONDS_OFFSET;
		current_micro    = timemgr_data + SIM_MICROSECONDS_OFFSET;
		slurmctl_pid     = timemgr_data + SIM_PTHREAD_SLURMCTL_PID;
		slurmd_pid       = timemgr_data + SIM_PTHREAD_SLURMD_PID;
		global_sync_flag = timemgr_data + SIM_GLOBAL_SYNC_FLAG_OFFSET;

		/* Change any values if requested via command-line options */
		if(incr) {
			printf("Incrementing the Simulator time by %d seconds.\n",
							incr);
			*current_sim += incr;
		}

		if(new_global_sync_flag != ' ') {
			printf("Changing the Simulator sync flag to %c.\n",
							new_global_sync_flag);
			*global_sync_flag = new_global_sync_flag;
		}

		/* Display shared mem values if requested via command-line option */
		if (show_mem) {
			printf("current_sim      = %u %s\n", *current_sim,
							_format_date());
			printf("current_micro    = %u\n", *current_micro);
			printf("slurmctl_pid     = %d\n",  *slurmctl_pid);
			printf("slurmd_pid       = %d\n",  *slurmd_pid);
			printf("global_sync_flag = %d\n",  *global_sync_flag);
		}
	} else {
		printf("The shared memory segment (%s) was not found.\n",
							SLURM_SIM_SHM);
	}

	return 0;
}
