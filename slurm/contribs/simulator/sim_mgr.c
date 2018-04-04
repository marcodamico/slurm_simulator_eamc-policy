#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h> /* SYS_gettid */
#include <pwd.h>
#include <ctype.h>
#include <sim_trace.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <config.h>
#include <slurm/slurm_errno.h>
#include <src/common/forward.h>
#include <src/common/hostlist.h>
#include <src/common/node_select.h>
#include <src/common/parse_time.h>
#include <src/common/slurm_accounting_storage.h>
#include <src/common/slurm_jobcomp.h>
#include <src/common/slurm_protocol_pack.h>
#include <src/common/switch.h>
#include <src/common/xassert.h>
#include <src/common/xstring.h>
#include <src/common/assoc_mgr.h>
#include <src/common/slurm_sim.h>
#include "src/common/sim_funcs.h"
//#include "src/unittests_lib/tools.h"
#include <getopt.h>

#undef DEBUG
int sim_mgr_debug_level = 9;

#define sim_mgr_debug(debug_level, ...) \
    ({ \
     if(debug_level <= sim_mgr_debug_level) \
     printf(__VA_ARGS__); \
     })

#define SAFE_PRINT(s) (s) ? s : "<NULL>"

char  daemon1[1024];
char  daemon2[1024];
char  default_sim_daemons_path[] = "/sbin";
char* sim_daemons_path = NULL;

char SEM_NAME[] = "serversem";
sem_t* mutexserver;
/*ANA: Replacing signals with shared vars for slurmd registration ***/
char    sig_sem_name[]  = "signalsem";
sem_t* mutexsignal = SEM_FAILED;

static pid_t pid[2] = {-1, -1};
static int launch_daemons = 0;
char** envs;

long int sim_start_point; /* simulation time starting point */
long int sim_end_point;   /* simulation end point is an optional parameter */

//int total_log_jobs=0; /* ANA: global variable shared between sim_mgr and controller to help end simulation when all jobs have finished */ 
//bool terminate_simulation_from_ctr=0; /* ANA: it will be read by sim_mgr in order to terminate simulation when all jobs have finished. */

job_trace_t *trace_head, *trace_tail;
rsv_trace_t *rsv_trace_head, *rsv_trace_tail;

char**  global_envp;  /* Uhmmm ... I do not like this limitted number of env values */
time_t time_incr = 1;     /* Amount to increment the simulated time by on each pass */
int sync_loop_wait_time = 1000; /* minimum uSeconds that the sync loop waits
 	 	 	 	 	 	 	 	  before advancing to add time_incr to the
 	 	 	 	 	 	 	 	  simulated time*/
int signaled  = 0;     /* signal from slurmd */
char*  workload_trace_file = NULL; /* Name of the file containing the workload to simulate */
char   default_trace_file[] = "test.trace";
char   help_msg[]= "sim_mgr [endtime]\n\t[-c | --compath <cpath>]\n\t[-f | "
		   "--fork]\n\t[-a | --accelerator <secs>]\n\t[-w | --wrkldfile"
		   " <filename> ]\n\t[-h | --help]\n"
		   "\t\t[-t | --looptime] <uSeconds>\n"
		   "Notes:\n\t'endtime' is "
		   "specified as seconds since Unix epoch. If 0 is specified "
		   "then the\n\t\tsimulator will run indefinitely.\n\t'cpath' "
		   "is the path to the slurmctld and slurmd (applicable only if"
		   " launching\n\t\tdaemons).  Specification of this option "
		   "supersedes any setting of\n\t\tSIM_DAEMONS_PATH.  If "
		   "neither is specified then the sim_mgr looks in a\n\t\t"
		   "sibling directory of where it resides called sbin.  "
		   "Finally, if still\n\t\tnot found then the default is /sbin."
		   "\n\t'secs' for the accelerator switch is the interval, in "
		   "simulated seconds, to\n\t\tincrement the simulated time "
		   "after each cycle instead of merely one.\n\t'filename' is "
		   "the name of trace file containing the information of the "
		   "jobs to\n\t\tsimulate.\n"
		   "\t'uSeconds' specify the minimum real time in us that is equivalent\n"
		   "\t\tto an step increase in the simulation time. A larger uSeconds\n"
		   "\t\tmakes the simulation slower, a smaller one, faster. Minimum\n"
		   "\t\tvalue is 1.\n";

/* Function prototypes */
void  generateJob(job_trace_t* jobd);
void  dumping_shared_mem();
void  fork_daemons(int idx);
char* getPathFromSelf(char*);
char* getPathFromEnvVar(char*);
int   getArgs(int, char**);
int   countEnvVars(char** envp);
uid_t userIdFromName(const char *name, gid_t* gid); /* function to get uid from username*/
#if 0
void  displayJobTraceT(job_trace_t* rptr);
#endif


int workflow_count = 0;
/* Management of the control on the job ids (trace vs. real)*/

/*
 * Pair of job ids corresponding to the id a job had in the trace and the one
 * that was actually raun with.
 */
typedef struct job_id_pair_t {
	uint32_t trace_job_id;
	uint32_t real_job_id;
};

/* List of job id pairs for the jobs that have been submitted already. Records
 * are stored in inverse order of growing by trace_job_id */
List job_id_list;

/*
 * create_job_id_pair - allocates a memory for a job_id_par_t, it will require
 * to be destroyed with _free_job_id_pair.
 */
struct job_id_pair_t *create_job_id_pair() {
	struct job_id_pair_t *pair_ptr;
	pair_ptr = xmalloc(sizeof(struct job_id_pair_t));
	pair_ptr->trace_job_id=NO_VAL;
	pair_ptr->real_job_id=NO_VAL;
	return pair_ptr;
}

/*
 * _free_job_id_pair deallocates memory.
 */
void _free_job_id_pair(struct job_id_pair_t *pair_ptr) {
	xfree(pair_ptr);
}

/*
 * Inits the job_id_list global variable.
 */
void _create_job_id_list() {
	job_id_list = list_create(_free_job_id_pair);
}

/*
 * _add_job_pair - adds info to the job_id_list for a job that had trace_job_id
 * id in the trace, but received real_job_id from slurmctld when submitted.
 */
void _add_job_pair(uint32_t trace_job_id, uint32_t real_job_id) {
	struct job_id_pair_t *pair_ptr=create_job_id_pair();
	pair_ptr->trace_job_id = trace_job_id;
	pair_ptr->real_job_id = real_job_id;
	if(!list_prepend(job_id_list, pair_ptr)) {
		error("Inserting pair of job_ids in job_id_list");
	}
}

/*
 * _get_real_job_id - returns the real job id corresponding to the trace job
 * id trace_job_id. If id not found, returns NO_VAL.
 */
uint32_t _get_real_job_id(uint32_t trace_job_id) {
	ListIterator *iter = list_iterator_create(job_id_list);
	struct job_id_pair_t *pair_ptr;
	uint32_t real_job_id=NO_VAL;
	while((pair_ptr=list_next(iter))) {
		if (pair_ptr->trace_job_id==trace_job_id) {
			real_job_id=pair_ptr->real_job_id;
			break;
		} else if(pair_ptr->trace_job_id<trace_job_id) {
			break;
		}
	}
	list_iterator_destroy(iter);
	return real_job_id;
}

/*
 * re_write_dependencies - creates a slurm format dependency string (e.g.
 * "afterok:2,afterok:3") with the same structure as dep_string but
 * replacing the trace job ids for real jobs ids.
 */
char *re_write_dependencies(char *dep_string) {
	char *token;
	char *saveptr1;
	uint32_t trace_job_id, real_job_id;
	char buffer[10*1024] = "\0";
	char buffer_2[10*1024] = "\0";
	char *new_dep = &buffer;
	char *new_dep_copy=&buffer_2;


	char first=1;
	for(token = strtok_r(dep_string, ":", &saveptr1);
		token;
		token = strtok_r(dep_string, ":", &saveptr1))
	{
		dep_string=NULL;
		char *dep_type = token;
		//assert_equal_str(token, "afterok", "Bad sbatch depedency format");
		token = strtok_r(NULL, ",", &saveptr1);
		trace_job_id = atoi(token);
		real_job_id = _get_real_job_id(trace_job_id);
		if (real_job_id==NO_VAL) {
			error("Real job id not found for job id: %d", trace_job_id);
			real_job_id=trace_job_id;
		}
		if (first) {
			sprintf(new_dep_copy, "%s%s:%d", new_dep, dep_type,
							real_job_id);
		} else {
			sprintf(new_dep_copy, "%s,%s:%d", new_dep, dep_type,
						    real_job_id);
		}
		char *aux=new_dep;
		new_dep = new_dep_copy;
		new_dep_copy = aux;
		first = 0;
	}
	return xstrdup(new_dep);
}

/* handler to USR2 signal*/
/*void
handlerSignal(int signo) {
	printf ("SIM_MGR [%d] got a %d signal--signaled: %d\n", getpid(), signo, signaled);
	//signaled++;
	*slurmd_registered += 1; /*ANA: Replacing signals with shared vars for slurmd registration ***/
//}*/


void
change_debug_level(int signum) {

	if(sim_mgr_debug_level > 0)
		sim_mgr_debug_level = 0;
	else
		sim_mgr_debug_level = 9;
}

void
terminate_simulation(int signum) {

	int i;

	dumping_shared_mem();

	sem_close(mutexserver);
	sem_unlink(SEM_NAME);

	slurm_shutdown(0); /* 0-shutdown all daemons without a core file */

	if(signum == SIGINT)
		exit(0);

	return;
}

/* Debugging */
void
dumping_shared_mem() {
	struct timeval t1;

	printf("Let's dump the shared memory contents\n");

	gettimeofday(&t1, NULL);
	printf("SIM_MGR[%u][%ld][%ld]\n", current_sim[0], t1.tv_sec, t1.tv_usec);

	return;
}

/* This is the main simulator function */
static void*
time_mgr(void *arg) {

	int child;
	long int wait_time;
	struct timeval t1 /*, t2*/;
	struct timeval t_start, t_end, i_loop;
	int i, j;

	printf("INFO: Creating time_mgr thread\n");

	current_sim      = timemgr_data + SIM_SECONDS_OFFSET;
	current_micro    = timemgr_data + SIM_MICROSECONDS_OFFSET;
	global_sync_flag = timemgr_data + SIM_GLOBAL_SYNC_FLAG_OFFSET;
	slurmd_registered = timemgr_data + SIM_SLURMD_REGISTERED_OFFSET; /*ANA: Replacing signals with shared vars for slurmd registration ***/

	memset(timemgr_data, 0, 32); /* st on 14-October-2015 moved from build_shared_memory to here as only sim_mgr should change the values, even if someone else "built" the shm segment. */

	current_sim[0]   = 1;
	//current_sim[0]   = sim_start_point;
	current_micro[0] = 0;

	/*signal (SIGUSR2, handlerSignal);*/
	/*printf("Waiting for signals, signaled: %d...\n", signaled);
	info("Waiting for signals..\n");
	sleep(5);
	while (signaled < 1 ){
		printf("... signaled: %d\n", signaled);
		sleep(5); 
	}*/
	/*ANA: Replacing signals with shared vars for slurmd registration ***/
	info("Waiting for slurmd registrations, slurmd_registered: %d...",
                                *slurmd_registered);
	//sleep(5);
        while (*slurmd_registered < 1 ) {
		info("before ... slurmd_registered: %d", *slurmd_registered);
                sleep(5);
                info("after ... slurmd_registered: %d", *slurmd_registered);
        }

	info("Done waiting.\n");

#ifdef DEBUG
	gettimeofday(&t1, NULL);
	info("SIM_MGR[%u][%ld][%ld]\n", current_sim[0], t1.tv_sec, t1.tv_usec);
#endif

	/* Main simulation manager loop */
	uint32_t seconds_to_finish=20;
	time_t time_end=0;

	while (1) {
		real_gettimeofday(&t_start, NULL);
		/* Do we have to end simulation in this cycle? */
		if (!time_end && (sim_end_point && sim_end_point <= current_sim[0] || *trace_recs_end_sim==-1)) { /* ANA: added condition for terminating sim using shmem var trace_recs_end_sim when all jobs have finished */
			fprintf(stderr, "End point of trace arrived, keep simulation for %d"
					" seconds for last jobs to be registered\n",
					seconds_to_finish);
			time_end=t_start.tv_sec;
		}
		if (time_end && (t_start.tv_sec-time_end>seconds_to_finish)) {
			fprintf(stderr, "Wait is over, time to end all processes in the"
					" simulation\n");
			terminate_simulation(SIGINT);
		}

		/* First going through threads and leaving a chance to execute code */

		gettimeofday(&t1, NULL);

		sim_mgr_debug(3, "SIM_MGR[%u][%ld][%ld]\n",
				current_sim[0], t1.tv_sec, t1.tv_usec);

#if 1
		/* Now checking if a new reservation needs to be created */
		if(rsv_trace_head &&
			(current_sim[0] >= rsv_trace_head->creation_time) ) {

			int exec_result;

			printf("Creation reservation for %s [%u - %ld]\n",
				rsv_trace_head->rsv_command, current_sim[0],
				rsv_trace_head->creation_time);

			child = fork();

			if (child == 0) { /* the child */
				char* const rsv_command_args[] = {
						rsv_trace_head->rsv_command,
						NULL
				};

				if(execve(rsv_trace_head->rsv_command,
						rsv_command_args,
						global_envp) < 0) {
					printf("Error in execve for %s\n",
						rsv_trace_head->rsv_command);
					printf("Exiting...\n");
					exit(-1);
				}
			}

			waitpid(child, &exec_result, 0);
			if(exec_result == 0)
				sim_mgr_debug(9, "reservation created\n");
			else
				sim_mgr_debug(9, "reservation failed");

			rsv_trace_head = rsv_trace_head->next;
		}
#endif

		/* Now checking if a new job needs to be submitted */
		while(trace_head) {

			int hour,min,sec;
			int exec_result;
			static int failed_submissions = 0;

			/*
			 * Uhmm... This is necessary if a large number of jobs
			 * are submitted at the same time but it seems
			 * to have an impact on determinism
			 */

#ifdef DEBUG
			info("time_mgr: current %u and next trace %ld\n",
					*(current_sim), trace_head->submit);
#endif

			if (*(current_sim) >= trace_head->submit) {

				job_trace_t *temp_ptr;

#ifdef DEBUG
				info("[%d] time_mgr--current simulated time: "
				       "%u\n", __LINE__, *current_sim);
#endif

				generateJob (trace_head);

				/* Let's free trace record */
				temp_ptr = trace_head;
				trace_head = trace_head->next;
				free(temp_ptr);

			} else {
				/*
				 * The job trace list is ordered in time so
				 * there is nothing else to do.
				 */
				break;
			}
		}

		/* Synchronization with daemons */
		//info("before unlocking next loop");
		sem_wait(mutexserver);
		info("unlocking next loop");
		*global_sync_flag = 1;
		sem_post(mutexserver);
		while(1) {
			sem_wait(mutexserver);
			if (*global_sync_flag == '*' || *global_sync_flag == 3) {
				sem_post(mutexserver);
				break;
			}
			sem_post(mutexserver);
                	usleep(sync_loop_wait_time);
		}
		/*
		 * Time throttling added but currently unstable; for now, run
		 * with only 1 second intervals
		 */
		real_gettimeofday(&t_end, NULL);
		*(current_sim) = *(current_sim) + time_incr;

		timersub(&t_end, &t_start, &i_loop);
		float speed_up;
		speed_up = ((float)(time_incr)*1000000.0) /
				   ((float)(i_loop.tv_sec*1000000+i_loop.tv_usec));
		float loop_time=(float)(i_loop.tv_sec*1000000+i_loop.tv_usec) /
				1000000.0;
		//printf("sim_mgr loop iteration time %lu, SU: %f, sim_inc: %d, sim_time: %d\n",
		//		i_loop.tv_usec, speed_up, time_incr, *(current_sim));
//#ifdef DEBUG
		info("[%d] time_mgr--current simulated time: %lu\n",
					__LINE__, *current_sim);
//#endif
	}

	return 0;
}

void
generateJob(job_trace_t* jobd) {
	job_desc_msg_t dmesg;
	submit_response_msg_t respMsg, *rptr = &respMsg;
	int rv, ix, jx;
	char script[8192], line[1024];
	uid_t uidt;
	gid_t gidt;

#if 0
	displayJobTraceT(jobd);
#endif
	sim_job_msg_t req;
	slurm_msg_t   req_msg;
	slurm_msg_t   resp_msg;
	slurm_addr_t  remote_addr;
	char* this_addr;

	uint32_t trace_job_id=jobd->job_id;

	sprintf(script,"#!/bin/bash\n");

	slurm_init_job_desc_msg(&dmesg);

	/* First, set up and call Slurm C-API for actual job submission. */
	dmesg.time_limit    = jobd->wclimit;
	dmesg.job_id        = NO_VAL;
	dmesg.name	    = "sim_job";
	uidt = userIdFromName(jobd->username, &gidt);
	dmesg.user_id       = uidt;
	dmesg.group_id      = gidt;
	dmesg.work_dir      = strdup("/tmp"); /* hardcoded to /tmp for now */
	dmesg.qos           = strdup(jobd->qosname);
	dmesg.partition     = strdup(jobd->partition);
	dmesg.account       = strdup(jobd->account);
	dmesg.reservation   = strdup(jobd->reservation);
	dmesg.dependency    = re_write_dependencies(jobd->dependency);
	dmesg.num_tasks     = jobd->tasks;
	dmesg.min_cpus      = jobd->tasks * jobd->cpus_per_task; 
	dmesg.cpus_per_task = jobd->cpus_per_task;
	dmesg.min_nodes     = jobd->tasks;
	dmesg.ntasks_per_node = jobd->tasks_per_node;

	/* Need something for environment--Should make this een more generic! */
	dmesg.environment  = (char**)malloc(sizeof(char*)*2);
	dmesg.environment[0] = strdup("HOME=/root");
	dmesg.env_size = 1;

	/* Standard dummy script. */
	sprintf(line,"#SBATCH -n %u\n", jobd->tasks);
	strcat(script, line);
	strcat(script, "\necho \"Generated BATCH Job\"\necho \"La Fine!\"\n");

	dmesg.script        = strdup(script);
	if (jobd->manifest!=NULL) {
//		dmesg.wf_program = strdup(jobd->manifest);
		dmesg.name=xmalloc(3+strlen(jobd->manifest_filename)+1+6+4+1);
		sprintf(dmesg.name, "wf_%s%d",
				jobd->manifest_filename,
				workflow_count);
		workflow_count+=1;
	} else if (strlen(jobd->manifest_filename)>1) {
		dmesg.name=xstrdup(jobd->manifest_filename+1);
	}

	if ( slurm_submit_batch_job(&dmesg, &rptr) ) {
		printf("Function: %s, Line: %d\n", __FUNCTION__, __LINE__);
		slurm_perror ("slurm_submit_batch_job");
	}

	_add_job_pair(trace_job_id, rptr->job_id);

	printf("\nResponse from job submission\n\terror_code: %u\n\t"
	       "job_id: %u\n\tstep_id: %u\n",
		rptr->error_code, rptr->job_id, rptr->step_id);
	printf("\n");

	/*
	 * Second, send special Simulator message to the slurmd to inform it
	 * when the given job should terminate. job_id is obtained from whatever
	 * slurmctld returned.
	 */
	slurm_msg_t_init(&req_msg);
	slurm_msg_t_init(&resp_msg);
	req.job_id       = rptr->job_id;
	req.duration     = jobd->duration;
	req_msg.msg_type = REQUEST_SIM_JOB;
	req_msg.data     = &req;
	req_msg.protocol_version = SLURM_PROTOCOL_VERSION;
	this_addr = "localhost";
	slurm_set_addr(&req_msg.address, (uint16_t)slurm_get_slurmd_port(),
						this_addr);
	if (!jobd->manifest || 1) {
		if (slurm_send_recv_node_msg(&req_msg, &resp_msg, 500000) < 0) {
			printf("check_events_trace: error in slurm_send_recv_node_msg\n");
		}
	}

	/* Should release the memory of the resp_msg and req_msg. */
}

uid_t
userIdFromName(const char *name, gid_t* gid) {
	struct passwd the_pwd;
	struct passwd *pwd=&the_pwd, *pwd_ptr;
	uid_t u;
	char *endptr;

	*gid = -1;			    /* In case of error, a -1     */
					    /* would be returned.         */

	if (name == NULL || *name == '\0')  /* On NULL or empty string    */
		return -1;                  /* return an error            */

	u = strtol(name, &endptr, 10);      /* As a convenience to caller */
	if (*endptr == '\0') {              /* allow a numeric string     */
		pwd = getpwuid(u);
		if (pwd == NULL)
			printf("Warning!  Could not find the group id "
			       "corresponding to the user id: %u\n", u);
		else
			*gid = pwd->pw_gid;
		return u;
	}

	getpwnam_r(name, pwd, NULL, 0, &pwd_ptr);
	if (pwd == NULL)
		return -1;

	*gid = pwd->pw_gid;

	return pwd->pw_uid;
}


int
insert_trace_record(job_trace_t *new) {

	if(trace_head == NULL){
		trace_head = new;
		trace_tail = new;
	} else {
		trace_tail->next = new;
		trace_tail = new;
	}
	return 0;
}

int
insert_rsv_trace_record(rsv_trace_t *new) {

	if(rsv_trace_head == NULL){
		rsv_trace_head = new;
		rsv_trace_tail = new;
	} else {
		rsv_trace_tail->next = new;
		rsv_trace_tail = new;
	}
	return 0;
}

int
init_trace_info(void *ptr, int op) {

	job_trace_t *new_trace_record;
	rsv_trace_t *new_rsv_trace_record;
	static int count = 0;

	if (op == 0) {
		new_trace_record = calloc(1, sizeof(job_trace_t));
		if (new_trace_record == NULL) {
			printf("init_trace_info: Error in calloc.\n");
			return -1;
		}

		*new_trace_record = *(job_trace_t *)ptr;

		if (count == 0) {
			sim_start_point = new_trace_record->submit; //Why from the first submit, better current_sim[0]=1 and shift the input trace to start at for e.g. 10s? 
			//sim_start_point = new_trace_record->submit - 60; //Why -60??
			/*first_submit = new_trace_record->submit;*/
		}

		count++;

		insert_trace_record(new_trace_record);
	}

	if (op == 1) {
		new_rsv_trace_record = calloc(1, sizeof(rsv_trace_t));
		if (new_rsv_trace_record == NULL) {
			printf("init_trace_info: Error in calloc for"
			       " new reservation\n");
			return -1;
		}

		*new_rsv_trace_record = *(rsv_trace_t *)ptr;
		new_rsv_trace_record->next = NULL;

		printf("Inserting new reservation trace record for time %ld\n",
		new_rsv_trace_record->creation_time);

		insert_rsv_trace_record(new_rsv_trace_record);
	}

	return 0;
}

#if 0
void displayJobTraceT(job_trace_t* rptr) {
	printf(
		"%8d"
		" %10s"
		" %12ld"
		" %10d"
		" %10d"
		" %7d"
		" %12s"
		" %12s"
		" %12s"
		" %15d"
		" %17d"
		" %12s"
		" %12s"
		"\n",
		rptr->job_id,
		SAFE_PRINT(rptr->username),
		rptr->submit,
		rptr->duration,
		rptr->wclimit,
		rptr->tasks,
		SAFE_PRINT(rptr->qosname),
		SAFE_PRINT(rptr->partition),
		SAFE_PRINT(rptr->account),
		rptr->cpus_per_task,
		rptr->tasks_per_node,
		SAFE_PRINT(rptr->reservation),
		SAFE_PRINT(rptr->dependency)
);
}
#endif

int init_job_trace() {
	int trace_file;
	job_trace_t new_job;
	int total_trace_records = 0;

        trace_recs_end_sim = timemgr_data + SIM_TRACE_RECS_END_SIM_OFFSET; /*ANA: Shared memory variable that will keep value of the total number of jobs in the log; once they are all finished controller will set it to -1 */

	trace_file = open(workload_trace_file, O_RDONLY);
	if (trace_file < 0) {
		printf("Error opening file %s\n", workload_trace_file);
		return -1;
	}

#if 0
	printf("%8s %10s %12s %10s %10s %7s %12s %12s %12s %15s %17s %12s %12s\n",
		"job_id:", "username:", "submit:", "duration:", "wclimit:",
		"tasks:", "qosname:", "partition: ", "account:", "cpus_per_task:",
		"tasks_per_node:", "reservation:", "dependency: ");
#endif
	int ret_val=0;
	while((ret_val=read_job_trace_record(trace_file, &new_job))>0) {
#if 0
		displayJobTraceT(&new_job);
#endif
		init_trace_info(&new_job, 0);
		total_trace_records++;
	}
	if (ret_val==-1) {
		printf("Error opening manifest\n");
		return -1;
	}

	printf("Trace initialization done. Total trace records: %d\n",
					total_trace_records);
        *trace_recs_end_sim=total_trace_records; /* ANA: update shared memory variable to total jobs in the log */ 
        printf("Trace initialization done. Total log jobs: %d\n",
                                        *trace_recs_end_sim);
	close(trace_file);

	return 0;
}

int init_rsv_trace() {

	int trace_file;
	rsv_trace_t new_rsv;
	int total_trace_records = 0;
	int count;
	char new_char;
	char buff[20];

	trace_file = open("rsv.trace", O_RDONLY);
	if(trace_file < 0){
		/*printf("Error opening file rsv.trace\n");*/
		printf("Warning!  Can NOT open file rsv.trace\n");
		return -1;
	}

	new_rsv.rsv_command = malloc(100);
	if(new_rsv.rsv_command < 0){
		printf("Malloc problem with reservation creation\n");
		return -1;
	}

	memset(buff, '\0', 20);
	memset(new_rsv.rsv_command, '\0', 100);

	while(1) {

		count = 0;

		/* First reading the creation time value */
		while(read(trace_file, &new_char, sizeof(char))) {

			if(new_char == '=')
				break;

			buff[count++] = new_char;
		}

		if(count == 0)
			break;

		new_rsv.creation_time = (long int)atoi(buff);

		count = 0;

		/* then reading the script name to execute */
		while(read(trace_file, &new_char, sizeof(char))) {

			new_rsv.rsv_command[count++] = new_char;

			if(new_char == '\n') { 
				new_rsv.rsv_command[--count] = '\0';
				break;
			}
		}

#if DEBUG
printf("Reading filename %s for execution at %ld\n",
		new_rsv.rsv_command, new_rsv.creation_time);
#endif

		init_trace_info(&new_rsv, 1);
		total_trace_records++;
		if(total_trace_records > 10)
			break;
	}


	printf("Trace initializarion done. Total trace records for "
			   "reservations: %d\n", total_trace_records);

	close(trace_file);

	return 0;
}

int
open_global_sync_semaphore() {
	mutexserver = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if(mutexserver == SEM_FAILED) {
		perror("unable to create server semaphore");
		sem_unlink(SEM_NAME);
		return -1;
	}

	return 0;
}
/*ANA: Replacing signals with shared vars for slurmd registration ***/
static int
open_slurmd_ready_semaphore()
{
        mutexsignal = sem_open(sig_sem_name, O_CREAT, 0755, 1);
        if(mutexsignal == SEM_FAILED) {
                perror("unable to create mutexsignal semaphore");
                sem_unlink(sig_sem_name);
                return -1;
        }

        return 0;
}
/**********************************************************************/
int
main(int argc, char *argv[], char *envp[]) {

	pthread_attr_t attr, attr2;
	pthread_t id_mgr/*, id_launch*/;   
	int i, status_ctld, status_slud;
	char thelib[1024];
	struct stat buf;
	int ix, envcount = countEnvVars(envp);

	/*struct sigaction sa;
	sa.sa_handler = handlerSignal;

	if(sigaction(SIGUSR2, &sa, NULL) == -1){
		printf("SIM_MGR: Unable to create handler for SIGUSR2!\n");
	}*/

	/*if (signal (SIGUSR2, handlerSignal) == SIG_ERR){
		printf("SIM_MGR: Unable to create handler for SIGUSR2!\n");
	}*/

	_create_job_id_list();
	if ( !getArgs(argc, argv) ) {
		printf("Usage: %s\n", help_msg);
		exit(-1);
	}

	printf("SIM_MGR_PID is %d\n", getpid());

	/* Determine location of the simulator library and Slurm daemons */

	if (!sim_daemons_path) {
		sim_daemons_path = getPathFromEnvVar("SIM_DAEMONS_PATH");}
	if (!sim_daemons_path)  {sim_daemons_path = getPathFromSelf("sbin");}

	if (!sim_daemons_path) {
		sim_daemons_path = default_sim_daemons_path;
	}

	/*sprintf(env_var1, "LD_LIBRARY_PATH=%s", sim_library_path);*/
	sprintf(daemon1,"%s/slurmctld", sim_daemons_path);
	sprintf(daemon2,"%s/slurmd",    sim_daemons_path);

	if ( stat(daemon1, &buf) == -1 ) {
		printf("Error!  Can not stat the slurmctld command: (%s).\n"
		       "Aborting the sim_mgr.\n", daemon1);
		exit(-1);
	}
	if ( stat(daemon2, &buf) == -1 ) {
		printf("Error!  Can not stat the slurmd command: (%s).\n"
		       "Aborting the sim_mgr.\n", daemon2);
		exit(-1);
	}

	printf("Settings:\n--------\n");
	printf("launch_daemons: %d\nsim_daemons_path: %s\n"
		"sim_end_point: %ld\nslurmctld: %s\nslurmd: %s\nenvironment "
		"\n\n\n",
		launch_daemons, SAFE_PRINT(sim_daemons_path), sim_end_point, daemon1, daemon2);

	/* Set up global environment list for later use in forking the daemons. */
	envs = (char**)malloc(sizeof(char*)*(envcount+1));
	global_envp = (char**)malloc(sizeof(char*)*(envcount+1));
	printf("%d env", envcount);
	for(ix=0; ix<envcount; ix++) {
		envs[ix] = envp[ix];
	}
	envs[ix]   = NULL;

	i = 0;
	while(envp[i]){
		global_envp[i] = envp[i++];
	}
	global_envp[i] = NULL;

	if(open_global_sync_semaphore() < 0){
		printf("Error opening the global synchronization semaphore.\n");
		return -1;
	};
	/*ANA: Replacing signals with shared vars for slurmd registration ***/
	if (open_slurmd_ready_semaphore() < 0) {
                error("Error opening the Simulator Slurmd-ready semaphore.");
                return -1;
        };

        if(init_job_trace() < 0){
                printf("An error was detected when reading trace file. "
                       "Exiting...\n");
                return -1;
        }

	if(init_rsv_trace() < 0){
                printf("An error was detected when reading trace file. \n"
                       /*"Exiting...\n"*/);
                /*return -1;*/
        }


	/* Launch the slurmctld and slurmd here */
	if (launch_daemons) {
		if(pid[0] == -1) fork_daemons(0);
		if(pid[1] == -1) fork_daemons(1);
		waitpid(pid[0], &status_ctld, 0);
		waitpid(pid[1], &status_slud, 0);
	}


	pthread_attr_init(&attr);
	signal(SIGINT, terminate_simulation);
	signal(SIGHUP, terminate_simulation);
	signal(SIGUSR1, change_debug_level);

	pthread_attr_init(&attr);

	/* This is a thread for flexibility */
	while (pthread_create(&id_mgr, &attr, &time_mgr, 0)) {
		printf("Error with pthread_create for time_mgr\n"); 
		return -1;
	}

	pthread_join(id_mgr, NULL);
	sleep(1);

	/*free(sim_library_path);*/
	return 0;
}

int
countEnvVars(char** envp) {
	int rv = 0;
	while(envp[rv]) ++rv;
	return rv;
}

void
fork_daemons(int idx) {
	char* args[3];

#if DEBUG
	printf("PROCESS: %d THREAD: %d FILE: %s FUNCTION: %s LINE: %d--about to"
	       " fork with idx: %d mutex_lock\n", getpid(), syscall(SYS_gettid),
		__FILE__, __FUNCTION__, __LINE__, idx);
#endif
	if(pid[idx]==-1) {
		pid[idx] = fork();
		if(pid[idx]==0) {
			if (idx==0) {
				args[0] = daemon1;
				args[1] = "-c";
				args[2] = NULL;
				execvpe(daemon1, args, envs);
			} else {
				args[0] = daemon2;
				args[1] = NULL;
				execvpe(daemon2, args, envs);
			}
			printf("Reached here--something went wrong! Error %s", strerror(errno));
		}
	}
}

/*
 * Tries to find a path based on the location of the sim_mgr executable.
 * If found, appends the "subpath" argument to it.
 */
char*
getPathFromSelf(char* subpath) {
	char* path,* ptr,* newpath = NULL;

	path = getenv("_");
	if (path) {
		newpath = strdup(path);
		ptr = strrchr(newpath, '/');
		if (ptr) {
			*ptr = '\0';
			ptr = strrchr(newpath, '/');
			if (ptr) {
				*ptr = '\0';
			} else {
				sprintf(newpath,"..");
			}
			strcat(newpath,"/");
			strcat(newpath, subpath);
		}
	}

	return newpath;
}

char*
getPathFromEnvVar(char* env_var) {
	char* path, *newpath = NULL;

	path = getenv(env_var);
	if(path) newpath = strdup(path);

	return newpath;
}

/*
 * Returns 1 if input is valid
 *         0 if input is not valid
 */
int
getArgs(int argc, char** argv) {
	static struct option long_options[]  = {
		{"fork",	0, 0, 'f'},
		{"compath",	1, 0, 'c'},
		{"accelerator",	1, 0, 'a'},
		{"wrkldfile",	1, 0, 'w'},
		{"help",	0, 0, 'h'},
		{"looptime",	1, 0, 't'}
	};
	int ix = 0, valid = 1;
	int opt_char, option_index;
	char* ptr;

	while (1) {
		if ((opt_char = getopt_long(argc, argv, "fc:ha:w:t:" , long_options,
						&option_index)) == -1 )
			break;

		switch(opt_char) {
			case ('f'):
				launch_daemons = 1;
				break;
			case ('c'):
				sim_daemons_path = strdup(optarg);
				break;
			case ('a'): /* Eventually use strtol instead of atoi */
				time_incr = atoi(optarg);
				break;
			case ('w'):
				workload_trace_file = strdup(optarg);
				break;
			case ('h'):
				printf("%s\n", help_msg);
				exit(0);
			case ('t'):
				sync_loop_wait_time = atoi(optarg);
				if (sync_loop_wait_time <= 0) {
					valid = 0;
				}
				break;
		};
	}

	if (optind < argc)
		for(ix = optind; ix<argc; ix++) {
			sim_end_point = strtol(argv[ix], &ptr, 10);

			/* If the argument is an empty string or ptr does not
			 * point to a Null character then the input is not
			 * valid.
			 */
			if ( !(argv[ix][0] != '\0' && *ptr == '\0') ) valid = 0;
		}

	if (!workload_trace_file) workload_trace_file = default_trace_file;

	return valid;
}
