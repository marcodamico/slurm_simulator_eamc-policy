#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_USERNAME_LEN 30
#define MAX_RSVNAME_LEN  30
#define MAX_QOSNAME      30
#define TIMESPEC_LEN     30
#define MAX_RSVNAME      30
#define MAX_MWF_STR_LEN  30
#define MAX_DEPNAME		 1024
#define MAX_WF_FILENAME_LEN	1024

static int earlier_submit_time = 0;

typedef struct job_trace {
    int  job_id;
    char username[MAX_USERNAME_LEN];
    long int submit; /* relative or absolute? */
    int  duration;
    int  wclimit;
    int  tasks;
    char qosname[MAX_QOSNAME];
    char partition[MAX_QOSNAME];
    char account[MAX_QOSNAME];
    int  cpus_per_task;
    int  tasks_per_node;
    char reservation[MAX_RSVNAME];
    char dependency[MAX_DEPNAME];
    struct job_trace *next;
    char manifest_filename[MAX_WF_FILENAME_LEN];
    char *manifest;
     // extra field to support extended modular workload format
    int modular_job_id;
    int total_components;
    char modular_jobname[MAX_MWF_STR_LEN];
    int submit_modular_job_time;
    int wait_modular_job_time;
    int modular_requested_time;
    int num_components_at_submitted_time;
    int component_job_id;
    char component_jobname[MAX_MWF_STR_LEN];
    int wait_component_job_time;
    int component_run_time;
    int status;
    // resource requirements - inputs
    int rreq_executable_number;
    char rreq_partition_name[MAX_QOSNAME];
    int rreq_nodes;
    int rreq_processes_per_node;
    int rreq_threads_per_process;
    int rreq_memory_per_node;
    int rreq_freq;
    int rreq_reference_power;
    int rreq_nam;
    int rreq_local_storage;
    char rreq_network_features[MAX_MWF_STR_LEN]; // network reqs
    char rreq_constraint[MAX_MWF_STR_LEN]; // features in SLRUM code
    char rreq_hint[MAX_MWF_STR_LEN]; // hint
    char rreq_licenses[MAX_MWF_STR_LEN];
    // resource allocation - outputs for comparison
    int ralloc_module_id;
    char ralloc_partition[MAX_QOSNAME];
    int ralloc_nodes;
    int ralloc_processes_per_node;
    int ralloc_threads_per_process;
    int ralloc_memory_per_node;
    int ralloc_freq;
    int ralloc_avg_power;
    int ralloc_nam;
    int ralloc_local_storage;
    char ralloc_network_features[MAX_MWF_STR_LEN]; // network reqs
    char ralloc_licenses[MAX_MWF_STR_LEN];
    // dependencies - inputs
    char after_complition_job_id[MAX_MWF_STR_LEN];
    char dependency_type[MAX_MWF_STR_LEN];
    int think_rreq_component_time;
} job_trace_t;

int read_job (FILE * trace_file_ptr, job_trace_t * job_trace) {
	job_trace_t ref_record;
	ssize_t ret_val = 0;
	ssize_t dummy = 0;

	ret_val = fscanf(trace_file_ptr, "%d;%ld;%d;%d;%d;%ld;%d;%ld;%d;%ld;%d;%29[^;];%29[^;];%ld;%29[^;];%29[^;];%29[^;];%ld",
	&job_trace->job_id, &job_trace->submit, &job_trace->wait_modular_job_time, 
	&job_trace->duration, &job_trace->tasks, 
	&dummy, &job_trace->rreq_memory_per_node, &dummy, &job_trace->wclimit, &dummy, 
	&job_trace->status, job_trace->username, job_trace->account, &dummy,
	job_trace->qosname,	job_trace->partition,
	job_trace->dependency, &dummy);

	job_trace->modular_job_id = job_trace->job_id;
	job_trace->submit_modular_job_time = job_trace->submit;
	job_trace->component_run_time = job_trace->duration;
	job_trace->modular_requested_time = job_trace->wclimit;
	job_trace->rreq_nodes = job_trace->tasks;
	job_trace->rreq_threads_per_process = job_trace->cpus_per_task;
	job_trace->rreq_processes_per_node = job_trace->tasks_per_node;

	// simple format in ascii (same from binary format)
	// for real logs this part of the code should be uncommented - for sintetic traces it is not necessary
	/* if (!earlier_submit_time)
		earlier_submit_time = job_trace->submit_modular_job_time;
	job_trace->submit_modular_job_time = job_trace->submit_modular_job_time - earlier_submit_time;*/
	
	sprintf(job_trace->modular_jobname,"Job %d",job_trace->modular_job_id);
	sprintf(job_trace->component_jobname,"HPC %d",job_trace->modular_job_id);
	sprintf(job_trace->rreq_partition_name,"%s",job_trace->partition);
	job_trace->total_components = 1;
	job_trace->tasks = 1;
/*
	if (job_trace->modular_job_id % 3 == 0)
		strcpy(job_trace->rreq_partition_name,"CM");
		strcpy(job_trace->rreq_partition_name,"CM");
	if (job_trace->modular_job_id % 3 == 1)
		strcpy(job_trace->rreq_partition_name,"ESB");
	if (job_trace->modular_job_id % 3 == 2)
		strcpy(job_trace->rreq_partition_name,"DAM");
*/		
	job_trace->qosname[0] = '\0';
	job_trace->reservation[0] = '\0';
	job_trace->dependency[0] = '\0';

	return ret_val;
}

void print_record (job_trace_t * new_job) {

         printf("%d;%d;%s;%d;%d;%d;%d;%d;%s;%d;%d;%d;%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s;%s;%s;%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s;%d;%s;%d\n",
		new_job->modular_job_id,
                new_job->total_components,
                new_job->modular_jobname,
                new_job->submit_modular_job_time,
                new_job->wait_modular_job_time,
                new_job->modular_requested_time,
                new_job->num_components_at_submitted_time,
                new_job->component_job_id,
		new_job->component_jobname,
                new_job->wait_component_job_time,
                new_job->component_run_time,
                new_job->status,
                new_job->rreq_executable_number,
                new_job->rreq_partition_name,
                new_job->rreq_nodes,
                1, //new_job->rreq_processes_per_node,
                1, //new_job->rreq_threads_per_process,
                new_job->rreq_memory_per_node,
                -1, //new_job->rreq_freq,
                -1, //new_job->rreq_reference_power,
                -1, //new_job->rreq_nam,
                -1, //new_job->rreq_local_storage,
                "-1", //new_job->rreq_network_features,
                "-1", //new_job->rreq_constraint,
                "-1", //new_job->rreq_hint,
                "-1", //new_job->rreq_licenses,
                -1, //new_job->ralloc_module_id,
                "-1", //new_job->ralloc_partition,
                -1, //new_job->ralloc_nodes,
                -1, //new_job->ralloc_processes_per_node,
                -1, //new_job->ralloc_threads_per_process,
                -1, //new_job->ralloc_memory_per_node,
                -1, //new_job->ralloc_freq,
                -1, //new_job->ralloc_avg_power,
                -1, //new_job->ralloc_nam,
                -1, //new_job->ralloc_local_storage,
                "-1", //new_job->ralloc_network_features,
                "-1", //new_job->ralloc_licenses,
                -1, //new_job->after_complition_job_id,
                "-1", //new_job->dependency_type,
                -1); //new_job->think_rreq_component_time);
}

int main (int argc, char **argv) {
        FILE * file = NULL;
	job_trace_t * new_job = NULL;
        int ret_val = 0;

        if (argc != 2) {
                printf("use: swf2mwf tracefile > output\n");
                exit(0);
        }

        file = fopen(argv[1], "r");

	new_job = (job_trace_t *) calloc(1,sizeof(job_trace_t));

        while ((ret_val = read_job(file, new_job)) > 0) {
		print_record(new_job);
		memset(new_job,0,sizeof(job_trace_t));
	}

        if (ret_val != EOF)
                printf("Error processing trace file %s\n", argv[1]);

        exit(0);
}
