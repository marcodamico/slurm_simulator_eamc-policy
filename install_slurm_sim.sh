#!/bin/bash

work_dir=`pwd`
slurm_install_dir="${work_dir}/install"
slurm_source_dir="${work_dir}/slurm"

cd slurm

echo "Regenerating automake Makefiles in Slurm"
./autogen.sh > ../autogen.output.txt

cd $work_dir

./compile_and_install_slurm.sh "${slurm_source_dir}" "${slurm_install_dir}"
