#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <getopt.h>

#include "sim_trace.h"

static int time_format = 0;           /* Default to Unix Epoch Format */
char* workload_trace_file = NULL;
char default_trace_file[] = "test.trace";
char help_msg[] = "list_trace [-w | --wrkldfile <filename>] [-u | --unixtime]"
		  " [-r | --humantime] [-h | --help]\n";

int getArgs(int argc, char** argv);

int main(int argc, char *argv[]) {

	int trace_file, record_idx = 0;
	job_trace_t job_trace;
	char buf[20];		

	if ( !getArgs(argc, argv) ) {
		printf("Usage: %s\n", help_msg);
		exit(-1);
	}

	if ( (trace_file = open(workload_trace_file, O_RDONLY)) < 0 ) {
		printf("Error opening workload file: %s\nAbort!\n",
					workload_trace_file);
		exit(-1);
	}

#if 0
	printf("JOBID  \tUSERNAME  \tPARTITION  \tACCOUNT  \tQOS     \t"
	       "SUBMIT               \tDURATION  \tWCLIMIT  \tTASKS  \tWF\n");
	printf("=====  \t========  \t=========  \t=======  \t======  \t"
	       "===================  \t========  \t======== \t===== \t=====\n");
#else
	/*printf("%12d \t%12s \t%12s \t%12s \t%12s \t%19s \t%12d \t%8d \t%10s"*/
	printf("%8s \t%12s \t%12s \t%12s \t%12s \t%12s \t%19s \t%12s \t%8s "
		"\t%10s \t%20s\n",
		"INDEX", "JOBID", "USERNAME", "PARTITION", "ACCOUNT", "QOS",
		"SUBMIT", "DURATION","WCLIMIT","TASKS", "WF");

	printf("%8s \t%12s \t%12s \t%12s \t%12s \t%12s \t%19s \t%12s \t%8s "
		"\t%10s \t%20s\n",
		"======", "=====", "========", "=========", "=======", "======",
		"======", "========", "========", "=====", "=====");
#endif
	while(read_job_trace_record(trace_file, &job_trace)) {
		++record_idx;

		if (time_format)
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
					localtime(&job_trace.submit));
		else
			sprintf(buf, "%ld", job_trace.submit);
	
		/*printf("%5d  \t%8s  \t%9s  \t%7s  \t%6s  \t%s  \t%8d  \t%7d  "
		       "\t%5d(%d,%d)",*/
		/*printf("%5d  \t%8s  \t%9s  \t%7s  \t%6s  \t%s  \t%8d  \t%7d  "
		       "\t%5d(%d,%d)",*/
		printf("%8d \t%12d \t%12s \t%12s \t%12s \t%12s \t%19s "
			"\t%12d \t%8d \t%5d(%d,%d) \t%20s",
			record_idx,
			job_trace.job_id,
			job_trace.username,
			job_trace.partition,
			job_trace.account,
			job_trace.qosname,
			buf,
			job_trace.duration,
			job_trace.wclimit,
			job_trace.tasks,
			job_trace.tasks_per_node,
			job_trace.cpus_per_task,
			job_trace.manifest_filename);

		if (strlen(job_trace.reservation) > 0)
			printf(" RES=%s", job_trace.reservation);
		if (strlen(job_trace.dependency) > 0)
			printf(" DEP=%s", job_trace.dependency);

		printf("\n");
	}

	return 0;
}

int
getArgs(int argc, char** argv) {
	static struct option long_options[] = {
		{"wrkldfile",      1, 0, 'w'},
		{"unixtime",       0, 0, 'u'},
		{"humantime",      0, 0, 'r'},
		{"help",           0, 0, 'h'},
	};
	int opt_char, option_index;
	char* ptr;
	int valid = 1;

	while (1) {
		if ( (opt_char = getopt_long(argc, argv, "w:urh", long_options,
						&option_index)) == -1 )
			break;
		switch (opt_char) {
			case ('w'):
				workload_trace_file = strdup(optarg);
				break;
			case ('u'):
				time_format = 0;
				break;
			case ('r'):
				time_format = 1;
				break;
			case ('h'):
				printf("%s\n", help_msg);
				exit(0);
		};
	}

	if (!workload_trace_file) workload_trace_file = default_trace_file;

	return valid;
}
