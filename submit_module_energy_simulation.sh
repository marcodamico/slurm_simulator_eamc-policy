#!/bin/bash -xe
#SBATCH -J slurm-simulator-into-slurm
#SBATCH -o OUTPUTS/test1_%j.out
#SBATCH -e OUTPUTS/test1_%j.err
#SBATCH -c 4
#SBATCH --nodes=1
#SBATCH -t 2-00:00:00
##SBATCH --qos=xlong

#unset OMP_NUM_THREADS
#source enableenv_mn4

workload_name=$(basename $1)
sim_path=$PWD/"simulation_"$SLURM_JOBID
rm -rf $sim_path

mkdir $sim_path
cp -r install/* $sim_path
workload=$1
control_host="$SLURM_NODELIST"
slave_nnodes=5120

slurm_conf_template="$sim_path/slurm_conf/slurm_module_energy.conf.template"
slurm_db_template="$sim_path/slurm_conf/slurmdbd.conf.template"

slurmctld_port=$((($SLURM_JOBID%65535))) #12 is the max number of jobs that can enter mn4 node
range=$((($slurmctld_port%12)))
slurmctld_port=$(((($slurmctld_port*$range)%65535)))
if [ $slurmctld_port -le 10000 ] ; then
	slurmctld_port=$((($slurmctld_port+10000)))
fi
slurmctld_f_port=$((($slurmctld_port+12)))

slurmd_port=$(($slurmctld_port+144))

user=`(whoami)`

openssl genrsa -out $sim_path/slurm_conf/slurm.key 1024
openssl rsa -in $sim_path/slurm_conf/slurm.key -pubout -out $sim_path/slurm_conf/slurm.cert

#create script to run commands
sed -e s:TOKEN_PATH:$sim_path: \
	"run_command.template" > "run_command.sh"

cd $sim_path

sed -e s/{ID_JOB}/$SLURM_JOBID/ \
    -e s:{DIR_WORK}:$sim_path: slurm_varios/trace.sh.template > slurm_varios/trace.sh;

chmod +x slurm_varios/trace.sh

sed -e s:TOKEN_USER:$user: \
	-e s:TOKEN_SLURM_USER_PATH:$sim_path: \
	-e s:TOKEN_BF_QUEUE_LIMIT:2000: \
	-e s:TOKEN_NNODES_P1:$slave_nnodes: \
	-e s:TOKEN_NNODES_P2:$(($slave_nnodes+1)): \
	-e s:TOKEN_NNODES_END:$(($slave_nnodes*2)): \
	-e s:TOKEN_CONTROL_MACHINE:$control_host: \
	-e s:TOKEN_SLURMCTLD_PORT:$slurmctld_port-$slurmctld_f_port: \
	-e s:TOKEN_SLURMD_PORT:$slurmd_port: \
	$slurm_conf_template > $sim_path/slurm_conf/slurm.conf

#MARCO: enable to enable slurmdbd
#sed -e s:TOKEN_USER:$user: \
#       -e s:TOKEN_SLURM_USER_PATH:$sim_path: \
#       -e s:TOKEN_CONTROL_MACHINE:$control_host: \
#       $slurm_db_template > $sim_path/slurm_conf/slurmdbd.conf

export PATH=$PATH:$sim_path/slurm_programs/bin
export PATH=$PATH:$sim_path/slurm_programs/sbin
export SLURM_CONF=$sim_path/slurm_conf/slurm.conf
export SLURM_SIM_ID=$$
export SIM_DAEMONS_PATH=$sim_path/slurm_programs/sbin
export LIBEN_MACHINE=$"/home/bsc15/bsc15800/phd/slurm-git/slurm-simulator/deepest-sim/en_conf/2hw.conf"
export LIBEN_APPS="/home/bsc15/bsc15800/phd/slurm-git/slurm-simulator/deepest-sim/en_conf/2hw/apps"
export LD_LIBRARY_PATH="/home/bsc15/bsc15800/phd/slurm-git/slurm-simulator/deepest-sim/deepest/DEEPEST-API/lib":$LD_LIBRARY_PATH
export LRZ_MODEL="/home/bsc15/bsc15800/phd/slurm-git/slurm-simulator/deepest-sim/deepest/DEEPEST-API"

sim_mgr -f -w $workload

#MARCO: dump_db if slurmdbd is enabled
#mysqldump -u slurm --password=slurm > db.dump
#mysql -u slurm --password=slurm < clean_db
#kill any remaining process

#generate output
printf 'JobId;Nodes;NodeList;Submit time;Start time;End time;Wait time;Run time;Response time;Slowdown;Backfilled;Frequency;Energy;DefEnergy;Partition;AppID\n' > "o_"$workload_name".csv"

cat TRACES/trace.test.$$ | sed -e 's!JobId=!!' | sort -n | awk '{print "JobId="$1,$13,$12,$9,$10,$11,$16,$17,$18,$19,$7,$20}' | awk '-F[=/ /]' '{split($0,a); printf("%d;%d;%s;%d;%d;%d;%d;%d;%d;%f;%d;%f;%f;%f;%s;%d\n",a[2],a[4],a[6],a[8],a[10],a[12],a[10]-a[8],a[12]-a[10],a[12]-a[8],(a[12]-a[8])/(a[12]-a[10]),a[14],a[16],a[18],a[20],a[22],a[24]) }' >> "o_"$workload_name".csv"
