#ifndef __SLURM_SIM_FUNCS_H__
#define __SLURM_SIM_FUNCS_H__

#ifdef SLURM_SIMULATOR

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#define _GNU_SOURCE
#include <dlfcn.h>
#include <sched.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <pwd.h>
#include "slurm/slurm.h"
#include "src/common/slurm_sim.h"
#include <sys/time.h>
#include <errno.h>
#include <execinfo.h>
#include <sys/syscall.h>

int (*real_gettimeofday)(struct timeval *,struct timezone *);
int attaching_shared_memory();
void get_semaphores_names(char *sim_sem, char *slurm_sem);
#endif
#endif  /*__SLURM_SIM_FUNCS_H__*/
