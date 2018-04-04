#!/bin/bash -xe
#SBATCH -J slurm-simulator-into-slurm
#SBATCH -o OUTPUTS/test1_%j.out
#SBATCH -e OUTPUTS/test1_%j.err
#SBATCH -c 48
#SBATCH --nodes=1
#SBATCH -t 01:00:00
#SBATCH --qos=debug

#unset OMP_NUM_THREADS
#source enableenv_mn4

bf_queue_limit=5000

sim_path="simulation_"$SLURM_JOBID
mkdir $sim_path
cp -r install/* $sim_path

sim_path="$(pwd)/$sim_path"

control_host="$SLURM_NODELIST"

slave_nnodes=3456

slurm_conf_template="$sim_path/slurm_conf/slurm.conf.template"

slurmctld_port=$((($SLURM_JOBID%65535))) #12 is the max number of jobs that can enter mn4 node
#not working, i will assume the risk instead of die trying 
#ok="0";
#while [ $ok -eq 0 ]
#do
#	netstat -ntl | grep [0-9]:$slurmctld_port -q ;
#	if [ $? -eq 1 ]
#    	then
#		echo qui
#        	ok="1"
#	else
#		echo li
#		slurmctld_porti=$(($slurmctld_port+1))
#	fi
#	echo ola
#done

slurmd_port=$(($slurmctld_port+12))
#ok="0";
#while [ $ok -eq 0 ]
#do
#        netstat -ntl | grep [0-9]:$slurmd_port -q ;
#	if [ $? -eq 1 ]
#        then
#                ok="1"
#        else
#                slurmctld_port=$(($slurmd_port+12))
#        fi
#	echo hola
#done

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
source env.sh
#./reset_user.sh
#slurmdbd
export SLURM_CONF=$(pwd)/slurm_conf/slurm.conf
sim_mgr $1 -f -w $2
