#!/bin/bash

TEMP='trace.tmp'
TEMP2='jobid.tmp'

if [ $# -ne 2 ]; then
    echo "./generator.sh <num_jobs> <output>"
    exit 0
fi

rm $2 2> /dev/null

./m_lublin99 $1 50 CM > $TEMP
./m_lublin99 $1 150 ESB >> $TEMP
./m_lublin99 $1 25 DAM >> $TEMP

cat $TEMP | sort -n -t ";" -k2 > $TEMP2

jobid=1
while IFS='' read -r line || [[ -n "$line" ]]; do
	LINE=$(echo "$line" | cut -d ';' -f2-)
	echo "$jobid;$LINE" >> $2
	let "jobid++"
done < "$TEMP2"

 
