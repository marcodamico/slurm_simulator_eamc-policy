#!/bin/bash -xe
sim_path=/home/bsc15/bsc15800/phd/slurm-git/slurm-simulator/deepest-sim/simulation_9688916

cd $sim_path

export PATH=$PATH:$sim_path/slurm_programs/bin
export PATH=$PATH:$sim_path/slurm_programs/sbin
export SLURM_CONF=$sim_path/slurm_conf/slurm.conf
export SIM_DAEMONS_PATH=$sim_path/slurm_programs/sbin
export LD_LIBRARY_PATH="/home/marcodamico/PhD/energy/deepest/DEEPEST-API/lib/":$LD_LIBRARY_PATH
export LRZ_MODEL="/home/marcodamico/PhD/energy/deepest/DEEPEST-API"

$1
