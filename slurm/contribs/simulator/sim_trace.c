#include <fcntl.h>
#include "sim_trace.h"
#include <unistd.h>
#include <string.h>
#include <math.h>
//#include "src/common/xmalloc.h" Including xmalloc and using malloc
#include <stdlib.h>

#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

char* read_file(char *file_name) {
	int file;
	int ret_val = 0;

	if ((file = open(file_name, O_RDONLY)) < 0 ) {
		printf("Error opening text file: %s\nAbort!\n", file_name);
		return NULL;
	}
	long length  = lseek (file, 0, SEEK_END);
	char *buffer = malloc (length);
	lseek (file, 0, SEEK_SET);
	if (buffer)
	{
	   ret_val = read(file, buffer, length);
	}
	close(file);

	if (ret_val > 0)
		return buffer;
	
	return NULL;
}

int read_job_trace_record(int trace_file, job_trace_t *job_trace) {
	//long record_type = 0;
	//char *manifest;
	//job_trace_t ref_record;
	ssize_t ret_val = 0;
/*	ssize_t ret_val=read(trace_file, &record_type, sizeof(record_type));
	if (!ret_val) {
		return ret_val;
	}

	int record_size = sizeof(ref_record);
	if (record_type != 0xFFFFFFFF) {
		record_size -= sizeof(manifest);
		record_size -= MAX_WF_FILENAME_LEN;
		lseek(trace_file, -sizeof(record_type), SEEK_CUR);
		strcpy(job_trace->manifest_filename, "|\0");
	}
*/	
	// RFS: see comment on init_trace_info function
	job_trace->next = NULL;

	ret_val = read(trace_file, job_trace, sizeof(job_trace_dummy_t));
	strcpy(job_trace->manifest_filename, "|\0");
	if (!ret_val)
		return ret_val;
	if (job_trace->manifest_filename[0]!='|') {
		/*Marco: changed xtrsup to strdup*/
		char *file_name_copy=strdup(job_trace->manifest_filename);
		char *real_file_name=strtok(file_name_copy,"-");
		if (!real_file_name)
			real_file_name=job_trace->manifest_filename;
		job_trace->manifest = read_file(real_file_name);
		free(file_name_copy);
		if (job_trace->manifest == NULL) {
			printf("Missing manifest file!! %s\n",
					real_file_name);
			return -1;
		}
	} else {
		job_trace->manifest=NULL;
	}
	return ret_val;
}

int read_job_trace_record_ascii(FILE * trace_file_ptr, job_trace_t *job_trace, int trace_format) {
	//long record_type = 0;
	//char *manifest;
	//job_trace_t ref_record;
	ssize_t ret_val = 0;
	ssize_t dummy = 0;
/*	ssize_t ret_val=read(trace_file, &record_type, sizeof(record_type));
	if (!ret_val) {
		return ret_val;
	}

	int record_size = sizeof(ref_record);
	if (record_type != 0xFFFFFFFF) {
		record_size -= sizeof(manifest);
		record_size -= MAX_WF_FILENAME_LEN;
		lseek(trace_file, -sizeof(record_type), SEEK_CUR);
		strcpy(job_trace->manifest_filename, "|\0");
	}
*/	
	// RFS: see comment on init_trace_info function
	job_trace->next = NULL;

	// simple format in ascii (same from binary format)
	if (trace_format == 1) {
		ret_val = fscanf(trace_file_ptr, "%d;%29[^;];%ld;%d;%d;%d;%29[^;];%29[^;];%d;%d",
		&job_trace->job_id, job_trace->username,
		&job_trace->submit, &job_trace->duration, &job_trace->wclimit,
		&job_trace->tasks, job_trace->partition, job_trace->account,
		&job_trace->cpus_per_task, &job_trace->tasks_per_node);

		job_trace->qosname[0] = '\0';
		job_trace->reservation[0] = '\0';
		job_trace->dependency[0] = '\0';

		strcpy(job_trace->manifest_filename, "|\0");
		if (!ret_val)
			return ret_val;
		if (job_trace->manifest_filename[0]!='|') {
			/*Marco: changed xtrsup to strdup*/
			char *file_name_copy=strdup(job_trace->manifest_filename);
			char *real_file_name=strtok(file_name_copy,"-");
			if (!real_file_name)
				real_file_name=job_trace->manifest_filename;
			job_trace->manifest = read_file(real_file_name);
			free(file_name_copy);
			if (job_trace->manifest == NULL) {
				printf("Missing manifest file!! %s\n",
						real_file_name);
				return -1;
			}
		} else {
			job_trace->manifest=NULL;
		}
	}

// standard format in ascii (all columns from models)
	if (trace_format == 2) {
                //3;3;-1;950;8;-1;-1;-1;-1;-1;1;tester;-1;-1;1;esb;-1;-1;esb,dam,cm
		ret_val = fscanf(trace_file_ptr, "%d;%ld;%d;%d;%d;%ld;%d;%ld;%d;%ld;%d;%29[^;];%29[^;];%ld;%29[^;];%29[^;];%29[^;];%ld;%29[^\n]",
		&job_trace->job_id, &job_trace->submit, &job_trace->wait_modular_job_time, 
		&job_trace->duration, &job_trace->tasks, 
		&dummy, &job_trace->rreq_memory_per_node, &dummy, &job_trace->wclimit, &dummy, 
		&job_trace->status, job_trace->username, job_trace->account, &dummy,
		job_trace->qosname,	job_trace->partition,
		job_trace->dependency, &dummy, job_trace->module_list);

                if(job_trace->wclimit == -1)
                    job_trace->wclimit = MAX(1,ceil((float)(job_trace->duration/60))*2);   // We set wclimit as twice the job duration if SWF does not have wclimit info. TODO Add a model.  
                job_trace->duration = MAX(1,ceil((float)(job_trace->duration/60)));
                
                if(job_trace->tasks == 0){
                    printf("sim_trace: Trace contains jobs with job_trace->tasks %u\n", job_trace->tasks );
                    job_trace->tasks = 1;
                }
		job_trace->cpus_per_task = 1;
		job_trace->tasks_per_node = 1;

		job_trace->account[0] = '\0';
		job_trace->qosname[0] = '\0';
		job_trace->reservation[0] = '\0';

		if(!strcmp(job_trace->dependency, "-1"))
			job_trace->dependency[0] = '\0';

                if(!strcmp(job_trace->module_list, "-1\0")){
                        job_trace->module_list[0] = '\0';
                }

		strcpy(job_trace->manifest_filename, "|\0");
		if (!ret_val)
			return ret_val;
		if (job_trace->manifest_filename[0]!='|') {
			/*Marco: changed xtrsup to strdup*/
			char *file_name_copy=strdup(job_trace->manifest_filename);
			char *real_file_name=strtok(file_name_copy,"-");
			if (!real_file_name)
				real_file_name=job_trace->manifest_filename;
			job_trace->manifest = read_file(real_file_name);
			free(file_name_copy);
			if (job_trace->manifest == NULL) {
				printf("Missing manifest file!! %s\n",
						real_file_name);
				return -1;
			}
		} else {
			job_trace->manifest=NULL;
		}
	}

	// modular workload format ascii (extended format)
	if (trace_format == 3) {
		ret_val = fscanf(trace_file_ptr,
		"%d;%d;%29[^;];%d;%d;%d;%d;%d;%29[^;];%d;%d;%d;%d;%29[^;];%d;%d;%d;%d;%d;%d;%d;%d;%29[^;];%29[^;];%29[^;];%29[^;];%d;%29[^;];%d;%d;%d;%d;%d;%d;%d;%d;%29[^;];%29[^;];%29[^;];%29[^;];%d",
		&job_trace->modular_job_id,
		&job_trace->total_components,
		job_trace->modular_jobname,
		&job_trace->submit_modular_job_time,
		&job_trace->wait_modular_job_time,
		&job_trace->modular_requested_time,
		&job_trace->num_components_at_submitted_time,
		&job_trace->component_job_id,
		job_trace->component_jobname,
		&job_trace->wait_component_job_time,
		&job_trace->component_run_time,
		&job_trace->status,
		&job_trace->rreq_executable_number,
		job_trace->rreq_partition_name,
		&job_trace->rreq_nodes,
		&job_trace->rreq_processes_per_node,
		&job_trace->rreq_threads_per_process,
		&job_trace->rreq_memory_per_node,
		&job_trace->rreq_freq,
		&job_trace->rreq_reference_power,
		&job_trace->rreq_nam,
		&job_trace->rreq_local_storage,
		job_trace->rreq_network_features,
		job_trace->rreq_constraint,
		job_trace->rreq_hint,
		job_trace->rreq_licenses,
		&job_trace->ralloc_module_id,
		job_trace->ralloc_partition,
		&job_trace->ralloc_nodes,
		&job_trace->ralloc_processes_per_node,
		&job_trace->ralloc_threads_per_process,
		&job_trace->ralloc_memory_per_node,
		&job_trace->ralloc_freq,
		&job_trace->ralloc_avg_power,
		&job_trace->ralloc_nam,
		&job_trace->ralloc_local_storage,
		job_trace->ralloc_network_features,
		job_trace->ralloc_licenses,
		job_trace->after_complition_job_id,
		job_trace->dependency_type,
		&job_trace->think_rreq_component_time);

		job_trace->job_id = job_trace->modular_job_id;
		job_trace->submit = job_trace->submit_modular_job_time;
		job_trace->duration = job_trace->component_run_time;
		job_trace->wclimit = job_trace->modular_requested_time;
		job_trace->tasks = job_trace->rreq_nodes * job_trace->rreq_processes_per_node;
		job_trace->cpus_per_task = 	job_trace->rreq_threads_per_process;
		job_trace->tasks_per_node = job_trace->rreq_processes_per_node;

		strcpy(job_trace->partition, job_trace->rreq_partition_name);
		strcpy(job_trace->username, "tester");
		strcpy(job_trace->account, "1000"); 

		job_trace->qosname[0] = '\0';
		job_trace->reservation[0] = '\0';
		job_trace->dependency[0] = '\0';

		strcpy(job_trace->manifest_filename, "|\0");
		if (!ret_val)
			return ret_val;
		if (job_trace->manifest_filename[0]!='|') {
			/*Marco: changed xtrsup to strdup*/
			char *file_name_copy=strdup(job_trace->manifest_filename);
			char *real_file_name=strtok(file_name_copy,"-");
			if (!real_file_name)
				real_file_name=job_trace->manifest_filename;
			job_trace->manifest = read_file(real_file_name);
			free(file_name_copy);
			if (job_trace->manifest == NULL) {
				printf("Missing manifest file!! %s\n",
						real_file_name);
				return -1;
			}
		} else {
			job_trace->manifest=NULL;
		}
	}

	return ret_val;
}
