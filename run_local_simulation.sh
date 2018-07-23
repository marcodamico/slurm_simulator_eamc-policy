#!/bin/bash -xe

SLURM_JOBID=$(find . -iname "simulation_*" | wc -l)
SLURM_JOBID=$(echo "$SLURM_JOBID + 1" | bc);


bf_queue_limit=5000

sim_path="simulation_"$SLURM_JOBID
mkdir $sim_path
cp -r install/* $sim_path

sim_path="$(pwd)/$sim_path"

control_host="$SLURM_NODELIST"

slave_nnodes=3456

slurm_conf_template="$sim_path/slurm_conf/slurm.conf.template"

openssl genrsa -out $sim_path/slurm_conf/slurm.key 1024
openssl rsa -in $sim_path/slurm_conf/slurm.key -pubout -out $sim_path/slurm_conf/slurm.cert

cd $sim_path

sed -e s/{ID_JOB}/$SLURM_JOBID/ \
    -e s:{DIR_WORK}:$sim_path: slurm_varios/trace.sh.template > slurm_varios/trace.sh;

chmod +x slurm_varios/trace.sh

sed -e s:TOKEN_SLURM_USER_PATH:$sim_path: \
    -e s:TOKEN_CONTROL_MACHINE:$control_host: \
    -e s:TOKEN_NNODES:$slave_nnodes: \
    -e s:TOKEN_SLURMCTLD_PORT:$slurmctld_port: \
    -e s:TOKEN_SLURMD_PORT:$slurmd_port: \
    -e s:TOKEN_BF_QUEUE_LIMIT:$bf_queue_limit: \
    $slurm_conf_template > $sim_path/slurm_conf/slurm.conf

#./run_sim.sh $1 $2 $3 > $SLURM_JOBID.log
#./run_sim_sudo.sh $1 $2  2>&1 > $3 &
#./run_sim_sudo.sh $1 $2
# 2>&1 > $3 &

DEFUNCT=$(ps -ef | grep slurm | grep defunct | tr -s " " | cut -d " " -f 2)
if [[ $DEFUNCT ]]; then
kill -9 $(ps -ef | grep slurm | grep defunct | tr -s " " | cut -d " " -f 2)
fi

source env.sh
#./reset_user.sh
#slurmdbd
export SLURM_CONF=$(pwd)/slurm_conf/slurm.conf
sim_mgr $1 -f -w $2

