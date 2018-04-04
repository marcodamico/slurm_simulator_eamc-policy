#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "sim_trace.h"
#include <getopt.h>
#include <sys/stat.h>

#define DEFAULT_START_TIMESTAMP 1420066800

/*static const char DEFAULT_START[] = "2015-01-01 00:00:00";*/
static const char DEFAULT_OFILE[] = "simple.trace";
static const char DEFAULT_UFILE[] = "users.sim";
static const int  MAX_CPUS        = 10;
static const int  MAX_TASKS       = 10;
enum { MAXLINES = 10000 };

static void print_usage() {
	printf("Usage: trace_builder [-c cpus_per_tasks] [-d duration] "
	       "[-i id_start] [-l number_of_jobs] [-n tasks_per_node] "
	       "[-o output_file] [-r <random>] [-s submission_step] "
	       "[-t tasks] [-u user.sim_file] \n");
}


int
main(int argc, char **argv) {

	char lines[MAXLINES][100];
	unsigned long timestamp = DEFAULT_START_TIMESTAMP;

	int  cpus_per_task  = 1;
	int  tasks_per_node = 1;
	int  tasks          = 1;
	int  random         = 0;
	int  length         = 100;
	int  id_start       = 1001;
	int  duration       = 10;
	char *ofile         = NULL;
	char *ufile         = NULL;
	int  step           = 0;
	int  i              = 0;
	int  flag           = 0;

	int  index, options;

	opterr   = 0;


	while ((options = getopt (argc, argv, "c:d:i:l:n:o:rs:t:u:")) != -1)
		switch (options) {
		case 'c':
			cpus_per_task = atoi(optarg);
			break;
		case 'd':
			duration = atoi(optarg);
			break;
		case 'i':
			id_start = atoi(optarg);
			break;
		case 'l':
			length = atoi(optarg);
			break;
		case 'n':
			tasks_per_node = atoi(optarg);
			break;
		case 'o':
			ofile = optarg;
			break;
		case 'r':
			random = 1;
			break;
		case 's':
			step = atoi(optarg);
			break;
		case 't':
			tasks = atoi(optarg);
			break;
		case 'u':
			ufile = optarg;
			break;
		default:
			print_usage();
			return 0;
		}

	if (!ofile) ofile = (char*)DEFAULT_OFILE;
	if (!ufile) ufile = (char*)DEFAULT_UFILE;


	for (index = optind; index < argc; index++) {
		printf ("Non-option argument %s\n", argv[index]);
		flag++;
	}

	if (flag) {
		print_usage();
		exit(-1);
	}

	/* reding users file */
	FILE *up = fopen(ufile, "r");
	if (up == 0) {
		fprintf(stderr, "failed to open input.txt\n");
		exit(1);
	} else	{
		while (i < MAXLINES && fgets(lines[i], sizeof(lines[0]), up)) {
			/* lines[i][strlen(lines[i])-1] = '\0'; */
		        i = i + 1;
		}
	}

	fclose(up);

	int j=0,k=0;
	job_trace_t new_trace;
	int trace_file;
	int written;
	int t1,t2;
	char username[100];
	char account[100];

	/* open trace file: */
	if ((trace_file = open(ofile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR |
						S_IRGRP | S_IROTH)) < 0) {
		printf("Error opening trace file\n");
		return -1;
        }

	
	for (j=0; j<(length); j++ ){
		new_trace.job_id = id_start;
		id_start++;
		new_trace.submit = timestamp + step;
		timestamp = timestamp + step;	

		if (random==1) {
			new_trace.duration = rand() % duration;
		} else {
			new_trace.duration = duration;
		}

		new_trace.wclimit = new_trace.duration+2;

		k  = rand() % i;
		t1 = strcspn (lines[k],":");
		t2 = strlen(lines[k]);
		strncpy(username, lines[k], t1);
		username[t1] = '\0';
		strncpy(account, lines[k]+t1+1, t2-1);
		strtok(account,"\n");

		/* printf("username = %s - %d\n", username, strlen(username)); */
		/* printf("account = %s - %d\n", account, strlen(account));    */


		sprintf(new_trace.username, "%s", username);
		sprintf(new_trace.account, "%s", account);
		sprintf(new_trace.partition, "%s", "normal");
		
		if (random == 1) {
			new_trace.tasks = (rand() % MAX_TASKS) + 1;
			new_trace.cpus_per_task = (rand() % MAX_CPUS) + 1;
                        new_trace.tasks_per_node = (rand() % 2) + 1 ;
		} else {
			new_trace.cpus_per_task = cpus_per_task;
			new_trace.tasks_per_node = tasks_per_node;        
			new_trace.tasks = tasks;
		}

		sprintf(new_trace.reservation, "%s", "");
		sprintf(new_trace.qosname, "%s", "");

		written = write(trace_file, &new_trace, sizeof(new_trace));
		if (written != sizeof(new_trace)) {
			printf("Error writing to file: %d of %ld\n",
					written, sizeof(new_trace));
			exit(-1);
		}
	}

	/* printf("WRITTEN %d TRACES \n", j); */
	
	close(trace_file);


	printf ("cpus_per_task = %d, duration = %d, length = %d, index = %d, "
		"tasks_per_node = %d, random = %d    , out_file= %s, step = %d,"
		" tasks = %d, user_file = %s\n",
		cpus_per_task,
		duration,
		length,
		id_start,
		tasks_per_node,
		random,
		ofile,
		step,
		tasks,
		ufile);

	return 0;
}
