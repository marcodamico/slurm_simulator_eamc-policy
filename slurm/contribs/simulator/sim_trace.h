#ifndef __SIM_TRACE_H_
#define __SIM_TRACE_H_

#include <stdio.h>

#ifdef SLURM_SIMULATOR

#define MAX_USERNAME_LEN 30
#define MAX_RSVNAME_LEN  30
#define MAX_QOSNAME      30
#define TIMESPEC_LEN     30
#define MAX_RSVNAME      30
#define MAX_MWF_STR_LEN  30
#define MAX_DEPNAME		 1024
#define MAX_WF_FILENAME_LEN	1024

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

/* dummy to keep compatibility with old binary format, used when read per bytes either than per columns */
typedef struct job_trace_dummy {
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
} job_trace_dummy_t;

typedef struct rsv_trace {
    long int           creation_time;
    char *             rsv_command;
    struct rsv_trace * next;
} rsv_trace_t;

int read_job_trace_record(int fd, job_trace_t *record);
int read_job_trace_record_ascii(FILE * fd, job_trace_t *record, int trace_format);
#endif
#endif
