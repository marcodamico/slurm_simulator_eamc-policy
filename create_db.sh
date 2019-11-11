#!/bin/bash
# Inspired from Slurm Simulator version by:
# Gonzalo P. Rodrigo <gprodrigoalvarez@lbl.gov>
#
# Creates the database required by the slurm simulator. Creates the users
# required by the local simulator and the remote management software.
#

mysql_user="root"
db_name="slurm_acct_db"
slurm_user="slurm"
slurm_pass="slurm"
echo "This script will create a MySQL database and configure two users as"\
" user ${mysql_user}. It will ask for the password of that mysql user three"\
" times."
echo "create database ${db_name};" | mysql -u "${mysql_user}" -p
echo "drop user '${slurm_user}'@'localhost';" \
	 "CREATE USER '${slurm_user}'@'localhost' IDENTIFIED BY '${slurm_pass}';"\
	 "GRANT ALL PRIVILEGES ON ${db_name}.* TO ${slurm_user}@localhost;" \
	 "FLUSH PRIVILEGES;" | mysql -u "${mysql_user}" -p

echo "********************************"
echo "********************************"
echo "IMPORTANT!!"
echo ""
echo "For the simulator to work MySQL needs to be configured so: It accepts" \
" connections from other hosts. this is done by setting" \
" bind-address = 0.0.0.0 in my.cnf (or equivalent)"
echo ""
echo "********************************"
echo "********************************"
