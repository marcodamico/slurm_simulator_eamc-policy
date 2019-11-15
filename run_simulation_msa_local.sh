#!/bin/bash -xe
workload_name=$(basename $1)
#change this path
sim_path="/Users/anajokanovic/SIM_V17_JUELICH/SLURM_SIMULATOR/s_"$$$workload_name"bfq"$2
rm -rf $sim_path

mkdir -p $sim_path
cp -r install/* $sim_path
workload=$1
control_host=$(hostname)

slave_nnodes=512 #EDIT: number of computing nodes

slurm_conf_template="$sim_path/slurm_conf/slurm_msa.conf.template"
slurm_db_template="$sim_path/slurm_conf/slurmdbd.conf.template"

slurmctld_port=$((($$%65535))) #12 is the max number of jobs that can enter mn4 node
range=$((($slurmctld_port%12)))
slurmctld_port=$(((($slurmctld_port*$range)%65535)))

if [ $slurmctld_port -le 10000 ] ; then
        slurmctld_port=$((($slurmctld_port+10000)))
fi
slurmctld_f_port=$((($slurmctld_port+12)))

slurmd_port=$((($slurmctld_port+144)))

user=`(whoami)`

openssl genrsa -out $sim_path/slurm_conf/slurm.key 1024
openssl rsa -in $sim_path/slurm_conf/slurm.key -pubout -out $sim_path/slurm_conf/slurm.cert

cd $sim_path

sed -e s/{ID_JOB}/$$/ \
    -e s:{DIR_WORK}:$sim_path: slurm_varios/trace.sh.template > slurm_varios/trace.sh;

chmod +x slurm_varios/trace.sh

sed -e s:TOKEN_USER:$user: \
	-e s:TOKEN_SLURM_USER_PATH:$sim_path: \
    -e s:TOKEN_BF_QUEUE_LIMIT:1000: \
    -e s:TOKEN_CONTROL_MACHINE:$control_host: \
	-e s:TOKEN_NNODES:$slave_nnodes: \
    -e s:TOKEN_SLURMCTLD_PORT:$slurmctld_port-$slurmctld_f_port: \
    -e s:TOKEN_SLURMD_PORT:$slurmd_port: \
    $slurm_conf_template > $sim_path/slurm_conf/slurm.conf

#MARCO: enable to enable slurmdbd
#sed -e s:TOKEN_USER:$user: \
#	-e s:TOKEN_SLURM_USER_PATH:$sim_path: \
#	-e s:TOKEN_CONTROL_MACHINE:$control_host: \
#	$slurm_db_template > $sim_path/slurm_conf/slurmdbd.conf

DEFUNCT=$(ps -ef | grep slurm | grep defunct | tr -s " " | cut -d " " -f 2)
if [[ $DEFUNCT ]]; then
kill -9 $(ps -ef | grep slurm | grep defunct | tr -s " " | cut -d " " -f 2)
fi

export PATH=$PATH:$sim_path/slurm_programs/bin
export PATH=$PATH:$sim_path/slurm_programs/sbin
export SLURM_CONF=$sim_path/slurm_conf/slurm.conf
export SLURM_SIM_ID=$$
#valgrind --trace-children=yes --leak-check=yes sim_mgr -f -w $workload
#slurmdbd #MARCO: enable to enable slurmdbd
sim_mgr -f -m $workload

#MARCO: dump_db if slurmdbd is enabled
#mysqldump -u slurm --password=slurm > db.dump
#mysql -u slurm --password=slurm < clean_db
#kill any remaining process
killall -9 slurmd
killall -9 slurmctld
killall -9 slurmdbd
killall -9 sim_mgr

#generate output
printf 'JobId;Nodes;NodeList;Submit time;Start time;End time;Wait time;Run time;Response time;Slowdown;Backfilled;Frequency;Energy;DefEnergy;Partition\n' > "o_"$workload_name".csv"

cat TRACES/trace.test.$$ | sed -e 's!JobId=!!' | sort -n | awk '{print "JobId="$1,$13,$12,$9,$10,$11,$16,$17,$18,$19,$7}' | awk '-F[=/ /]' '{split($0,a); printf("%d;%d;%s;%d;%d;%d;%d;%d;%d;%f;%d;%f;%f;%f;%s\n",a[2],a[4],a[6],a[8],a[10],a[12],a[10]-a[8],a[12]-a[10],a[12]-a[8],(a[12]-a[8])/(a[12]-a[10]),a[14],a[16],a[18],a[20],a[22]) }' >> "o_"$workload_name".csv"

#cat TRACES/trace.test.$$ | sed -e 's!JobId=!!' | sort -n | awk '{print "JobId="$1,$12,$11,$8,$9,$10,$15}' | awk '-F[=/ /]' '{split($0,a); printf("%d;%d;%s;%d;%d;%d;%d;%d;%d;%f;%d\n",a[2],a[4],a[6],a[8],a[10],a[12],a[10]-a[8],a[12]-a[10],a[12]-a[8],(a[12]-a[8])/(a[12]-a[10]),a[14]) }' >> "o_"$workload_name".csv"

#cat TRACES/trace.test.$$ | sed -e 's!JobId=!!' | sort -n | awk '{print $8,$9,$10,$15}' | awk -F'[=/ /]' '{split($0,a); print a[2], a[4], a[6], a[8] }' > "o_"$workload_name

#cat TRACES/trace.test.$$ | sed -e 's!JobId=!!' | sort -n | awk '{ print $1,$7,$8,$9,$10,$13 }' | awk -F'[=/ /]' '{split($0,a); printf("%s\t%s\t%s\t%s\t%s\t-1\t-1\t%s\t%s\t-1\t1\t-1\t-1\t-1\t-1\t-1\t-1\t-1\n",a[1]-1,a[5],a[7]-a[5],a[9]-a[7],a[11],a[11],a[3])}' > "o_"$workload_name".swf"
