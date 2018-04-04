//#ifdef SLURM_SIMULATOR
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>


#define MAX_USERNAME_LEN 30
#define MAX_RSVNAME_LEN  30
#define MAX_QOSNAME      30
#define TIMESPEC_LEN     30
#define MAX_RSVNAME      30
#define CPUS_PER_NODE    8 
#define MAX_WF_FILENAME_LEN     1024
static const char DEFAULT_OFILE[]    = "simple.trace"; //The output trace name should be passed as an argument on command line.
#define MAX_DEPNAME              1024

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
} job_trace_t;




int main(int argc, char* argv[])
{
    int i;
    int idx=0, errs=0;
    int nrecs=10000; // Number of records has to be entered here each time. This should be done differently.
    job_trace_t* job_trace,* job_trace_head,* job_arr,* job_ptr;
    char const* const fileName = argv[1]; /* should check that argc > 1 */
    FILE* file = fopen(fileName, "r"); /* should check the result */
    char line[256];
    char *p;

    job_arr = (job_trace_t*)malloc(sizeof(job_trace_t)*nrecs);
    if (!job_arr) {
                printf("Error.  Unable to allocate memory for all job records.\n");
                return -1;
    }
    job_trace_head = &job_arr[0];
    job_ptr = &job_arr[0];
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        printf("%s", line);
        p = strtok(line, " ");
        i=0;
        while(p!=NULL){
            if(i==0) { printf("%s", p); job_arr[idx].job_id = atoi(p); }   
            if(i==1) { printf("%s", p); job_arr[idx].submit = 100 + atoi(p); }  // why submit cannot start from 0? 
            if(i==3) { printf("%s", p); job_arr[idx].duration = atoi(p); }   
            if(i==7) { printf("%s", p); job_arr[idx].tasks = ceil((float) (atoi(p)/CPUS_PER_NODE)); }
            //if(i==7) { printf("%s", p); job_arr[idx].tasks = ceil((float) atoi(p)); }
            if(i==8) { printf("%s", p); job_arr[idx].wclimit = ceil((double)atoi(p)/60); }   
            //if(i==11) { printf("%s", p); strcpy(job_arr[idx].username, p); }   
            //if(i==12) { printf("%s", p); strcpy(job_arr[idx].account, p); }   
            //if(i==15) { printf("%s", p); strcpy(job_arr[idx].partition, p); }   
            printf(" %s\n", p);
            p = strtok(NULL," ");
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
        strcpy(job_arr[idx].partition, "normal");
        strcpy(job_arr[idx].account, "1000");
        //strcpy(job_trace->manifest_filename, "|\0");
        //job_trace->manifest=NULL; 
        idx++; 
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);

    
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
    while(j<nrecs){
         written = write(trace_file, &job_arr[j], sizeof(job_trace_t));
              if (written <= 0) {
                        printf("Error! Zero bytes written.\n");
                        ++errs;
              }

        //job_ptr = job_ptr->next;
        j++;
    }
    close(trace_file);
    free(job_arr);
    return 0;
}


