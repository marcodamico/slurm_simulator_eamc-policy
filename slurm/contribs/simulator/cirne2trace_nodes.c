//#ifdef SLURM_SIMULATOR
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

#include "sim_trace.h"

#define CPUS_PER_NODE     8

static const char DEFAULT_OFILE[]    = "simple.trace"; //The output trace name should be passed as an argument on command line.

typedef struct swf_job_trace {
    int job_num;
    time_t sub_time;
    int wait_time;
    int run_time;
    int alloc_cpus;
    int avg_cpu_time;
    int mem;
    int req_cpus;
    int req_time;
    int req_mem;
    int status;
    int uid;
    int gid;
    int app_num;
    int queue_num;
    int part_num;
    int prev_job_num;
    int think_time_prev_job;
} swf_job_trace_t;

int main(int argc, char* argv[])
{
    int i;
    int idx = 0, errs = 0;
    int nrecs = 0;
    job_trace_dummy_t* job_trace,* job_trace_head,* job_arr,* job_ptr;
    swf_job_trace_t *swf_job_arr;
    char const* const fileName = argv[1]; /* should check that argc > 1 */
    FILE* file = fopen(fileName, "r"); /* should check the result */
    char line[256];
    char *p;
    char ch;
    int job_index = 0;
    long  start_time, first_arrival = 0;

    while(!feof(file)) {
        ch = fgetc(file);
        if(ch == '\n')
        {
            nrecs++;
        }
    }
	printf("N jobs %d", nrecs);
    rewind(file);

    job_arr = (job_trace_dummy_t*)malloc(sizeof(job_trace_dummy_t)*nrecs);
    if (!job_arr) {
                printf("Error.  Unable to allocate memory for all job records.\n");
                return -1;
    }
    swf_job_arr = (swf_job_trace_t *)malloc(sizeof(swf_job_trace_t) * nrecs);
    if (!swf_job_arr) {
                printf("Error.  Unable to allocate memory for all job records.\n");
                return -1;
    }

    job_trace_head = &job_arr[0];
    job_ptr = &job_arr[0];
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        printf("%s", line);
	if (strstr(line,"cancelled")) //cancelled jobs not supported
		continue;
	if(strncmp(line, "#", 1) == 0)
                continue;
        if(strncmp(line, ";", 1) == 0)
                continue;
        p = strtok(line, " \t");
	i=0;
        while(p!=NULL){
            if(i==0) { 
		job_arr[idx].job_id = ++job_index; 
		swf_job_arr[idx].job_num = job_arr[idx].job_id;
		printf("Index is: %d", job_index);
	    }
            else if(i==4) {
		printf("Submit time: %s\n", p); 
		if (first_arrival == 0) 
		    first_arrival = atol(p); 
		job_arr[idx].submit = 100 + atol(p) - first_arrival;
		swf_job_arr[idx].sub_time = job_arr[idx].submit;
	    }  // why submit cannot start from 0?
	    else if(i==5) {
		printf("Ntasks/nodes: %s\n", p);
		job_arr[idx].tasks = atoi(p);
		swf_job_arr[idx].req_cpus = job_arr[idx].tasks * CPUS_PER_NODE;
	    }
            else if(i==6) {
		printf("Wallclock limit: %s\n", p);
		job_arr[idx].wclimit = ceil((double)atoi(p) / 60.0f);
		swf_job_arr[idx].req_time = job_arr[idx].wclimit * 60;
	    }
            else if(i==7) {
		printf("Startime: %s\n", p); 
		start_time = atol(p);
	    }
            else if(i==8) {
		printf("End time: %s\n", p);
		uint32_t n;
		sscanf(p, "%"SCNu32, &n);
		job_arr[idx].duration = (uint32_t) n - start_time;
		swf_job_arr[idx].run_time = job_arr[idx].duration;
		swf_job_arr[idx].wait_time = (long) start_time - swf_job_arr[idx].sub_time - first_arrival + 100;
		printf("Duration: %d\n", job_arr[idx].duration);
	    }
		//if(i==11) { printf("%s", p); strcpy(job_arr[idx].username, p); }   
            //if(i==12) { printf("%s", p); strcpy(job_arr[idx].account, p); }   
            //if(i==15) { printf("%s", p); strcpy(job_arr[idx].partition, p); }   
            p = strtok(NULL," \t");
            i++; 
        }
        // assuming pure MPI application; for threaded one we will have to do it differently.
        //job_arr[idx].cpus_per_task = 1;
        //if(job_arr[idx].tasks < CPUS_PER_NODE) job_arr[idx].tasks_per_node = job_arr[idx].tasks; 
        //else job_arr[idx].tasks_per_node = CPUS_PER_NODE;

        //assuming one task per node, and as many threads as CPUs per node
        job_arr[idx].cpus_per_task = CPUS_PER_NODE;
        job_arr[idx].tasks_per_node = 1;

        // for now keep username, partition and account constant.
        strcpy(job_arr[idx].username, "tester");
        strcpy(job_arr[idx].partition, "normal,normal2");
        strcpy(job_arr[idx].account, "1000");
	swf_job_arr[idx].alloc_cpus = swf_job_arr[idx].req_cpus;
	swf_job_arr[idx].avg_cpu_time = -1;
	swf_job_arr[idx].mem = -1;
	swf_job_arr[idx].req_mem = -1;
	swf_job_arr[idx].status = 1;
	swf_job_arr[idx].uid = -1;
	swf_job_arr[idx].gid = -1;
	swf_job_arr[idx].app_num = -1;
	swf_job_arr[idx].queue_num = -1;
	swf_job_arr[idx].part_num = -1;
	swf_job_arr[idx].prev_job_num = -1;
	swf_job_arr[idx].think_time_prev_job = -1;
        idx++;

    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);

    FILE *swf_fp = fopen("swf_trace","w"); 
    int trace_file, written;
    char *ofile         = NULL;
    if (!ofile) ofile = (char*)DEFAULT_OFILE;
    /* open trace file: */
    if ((trace_file = open(ofile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR |
                                                S_IRGRP | S_IROTH)) < 0) {
       printf("Error opening trace file %s\n", ofile);
       return -1;
    }
    job_ptr = job_trace_head;
    int j=0;
    int nrecs_limit = idx;
    while(j<nrecs_limit){
         written = write(trace_file, &job_arr[j], sizeof(job_trace_dummy_t));
              if (written <= 0) {
                        printf("Error! Zero bytes written.\n");
                        ++errs;
              }
	fprintf(swf_fp, "%d\t%ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",swf_job_arr[j].job_num,
                                swf_job_arr[j].sub_time,
                                swf_job_arr[j].wait_time,
                                swf_job_arr[j].run_time,
                                swf_job_arr[j].alloc_cpus,
                                swf_job_arr[j].avg_cpu_time,
                                swf_job_arr[j].mem,
                                swf_job_arr[j].req_cpus,
                                swf_job_arr[j].req_time,
                                swf_job_arr[j].req_mem,
                                swf_job_arr[j].status,
                                swf_job_arr[j].uid,
                                swf_job_arr[j].gid,
                                swf_job_arr[j].app_num,
                                swf_job_arr[j].queue_num,
                                swf_job_arr[j].part_num,
                                swf_job_arr[j].prev_job_num,
                                swf_job_arr[j].think_time_prev_job);
        //job_ptr = job_ptr->next;
        j++;
    }
    fclose(swf_fp);
    close(trace_file);
    free(job_arr);
    return 0;
}


