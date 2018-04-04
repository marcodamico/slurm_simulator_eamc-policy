#include <fcntl.h>
#include "sim_trace.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
//#include "src/common/xmalloc.h" Including xmalloc and using malloc
#include <stdlib.h>


char* read_file(char *file_name) {
	int file;
	if ((file = open(file_name, O_RDONLY)) < 0 ) {
		printf("Error opening text file: %s\nAbort!\n", file_name);
		return NULL;
	}
	long length  = lseek (file, 0, SEEK_END);
	char *buffer = malloc (length);
	lseek (file, 0, SEEK_SET);
	if (buffer)
	{
	   read(file, buffer, length);
	}
	close(file);
	return buffer;
}

int read_job_trace_record(int trace_file, job_trace_t *job_trace) {
	long record_type = 0;
	char *manifest;
	job_trace_t ref_record;
	ssize_t ret_val;
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
*/	ret_val = read(trace_file, job_trace, sizeof(job_trace_t));
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
