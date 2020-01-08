#!/bin/bash
if [ $# -ne 2 ];
then
        echo "Script needs to be invoked with two arguments."
    echo "./compile_and_install_slurm.sh  /slurm_source /target_install_dir"
fi

slurm_source_dir="$1"
install_dir="$2"

echo "Compiling and installing Slurm from ${slurm_source_dir} to "\
"${install_dir}"

mkdir -p "${install_dir}"
mkdir -p "${install_dir}/slurm_varios/var/state"
mkdir -p "${install_dir}/slurm_varios/var/spool"
mkdir -p "${install_dir}/slurm_varios/log"

export LIBS=-lrt
#old
#export CFLAGS="-D SLURM_SIMULATOR -D WF_API"

#perf
export CFLAGS="-D SLURM_SIMULATOR"
#profiling and debugging, also add --enable-memory-leak-debug to configure command
#export CFLAGS="-D SLURM_SIMULATOR -g -pg -Og -ggdb -g3 -fno-omit-frame-pointer"

cd "${slurm_source_dir}"

echo "Running Configure"

./configure --exec-prefix=$install_dir/slurm_programs \
--bindir=$install_dir/slurm_programs/bin \
--sbindir=$install_dir/slurm_programs/sbin \
--datadir=$install_dir/slurm_varios/share \
--includedir=$install_dir/slurm_varios/include \
--libdir=$install_dir/slurm_varios/lib \
--libexecdir=$install_dir/slurm_varios/libexec \
--localstatedir=$install_dir/slurm_varios \
--sharedstatedir=$install_dir/slurm_varios \
--mandir=$install_dir/slurm_varios/man \
--prefix=$install_dir/slurm_programs --sysconfdir=$install_dir/slurm_conf \
--enable-front-end \
--enable-simulator 2> slurm_configure.log

echo "Compiling and installing..."
make -j4 && make -j install && cd contribs/simulator && make && make -j install && cd ../..
