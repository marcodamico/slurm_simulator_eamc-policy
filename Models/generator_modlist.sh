#!/bin/bash

TEMP='trace.tmp'
TEMP2='jobid.tmp'

if [ $# -ne 4 ]; then
    echo "./generator_modlist.sh <num_jobs> <output_basic> <output> <percent_flexible_jobs>"
    exit 0
fi

rm $2 2> /dev/null
rm $3 2> /dev/null

./m_lublin99 $1 50 cm > $TEMP
./m_lublin99 $1 75 esb >> $TEMP
./m_lublin99 $1 16 dam >> $TEMP

cat $TEMP | sort -n -t ";" -k2 > $TEMP2


jobid=1
while IFS='' read -r line || [[ -n "$line" ]]; do
        LINE=$(echo "$line" | cut -d ';' -f2-)
        echo "$jobid;$LINE;-1" >> $2 ## Create basic trace without module list to be used for comparison 
        number=$RANDOM
        let "number %= 100"
        if [ "$number" -le $4 ] && [ "$number" -gt 0 ]
        then
            PART=$(echo "$LINE" | cut -d ';' -f 15)
            if [ $PART = 'cm' ]
            then
                number1=$RANDOM
                let "number1 %= 100"
                if [ "$number1" -le 50 ]
                then
                    echo "$jobid;$LINE;$PART,esb,dam" >> $3
                else 
                    echo "$jobid;$LINE;$PART,dam,esb" >> $3
                fi
            elif [ $PART = 'esb' ]
            then
                echo "$jobid;$LINE;$PART,dam,cm" >> $3
            elif [ $PART = 'dam' ]
            then
                echo "$jobid;$LINE;$PART,esb,cm" >> $3
            fi
            ## PART is preferred module, but alternative modules should be added. TODO How do we add alternative modules, in which order? Both of them? 
            ## Provisional implementation at the moment
        else
            echo "$jobid;$LINE;-1" >> $3
        fi
        let "jobid++"
done < "$TEMP2"

