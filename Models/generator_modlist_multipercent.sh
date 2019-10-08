#!/bin/bash

TEMP='trace.tmp'
TEMP2='jobid.tmp'

if [ $# -ne 7 ]; then
    echo "./generator_modlist.sh <num_jobs> <output_basic> <output1> <output2> <output3> <output4> <percent_flexible_jobs>"
    exit 0
fi

rm $2 2> /dev/null
rm $3 2> /dev/null
rm $4 2> /dev/null
rm $5 2> /dev/null
rm $6 2> /dev/null

./m_lublin99 $1 50 cm > $TEMP
./m_lublin99 $1 75 esb >> $TEMP
./m_lublin99 $1 16 dam >> $TEMP

cat $TEMP | sort -n -t ";" -k2 > $TEMP2

first_p=$7
second_p=$((first_p - 20))
third_p=$((first_p - 40))
fourth_p=$((first_p - 60))

jobid=1
while IFS='' read -r line || [[ -n "$line" ]]; do
        LINE=$(echo "$line" | cut -d ';' -f2-)
        echo "$jobid;$LINE;-1" >> $2 ## Create basic trace without module list to be used for comparison 
        number=$RANDOM
        let "number %= 100"
        # Max number of flexible jobs
        if [ "$number" -le $first_p ] && [ "$number" -gt 0 ]
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

        # Max number of flexible jobs minus 20
        if [ "$number" -le $second_p ] && [ "$number" -gt 0 ]
        then
            PART=$(echo "$LINE" | cut -d ';' -f 15)
            if [ $PART = 'cm' ]
            then
                number1=$RANDOM
                let "number1 %= 100"
                if [ "$number1" -le 50 ]
                then
                    echo "$jobid;$LINE;$PART,esb,dam" >> $4
                else
                    echo "$jobid;$LINE;$PART,dam,esb" >> $4
                fi
            elif [ $PART = 'esb' ]
            then
                echo "$jobid;$LINE;$PART,dam,cm" >> $4
            elif [ $PART = 'dam' ]
            then
                echo "$jobid;$LINE;$PART,esb,cm" >> $4
            fi
            ## PART is preferred module, but alternative modules should be added. TODO How do we add alternative modules, in which order? Both of them? 
            ## Provisional implementation at the moment
        else
            echo "$jobid;$LINE;-1" >> $4
        fi

        # Max number of flexible jobs minus 40
        if [ "$number" -le $third_p ] && [ "$number" -gt 0 ]
        then
            PART=$(echo "$LINE" | cut -d ';' -f 15)
            if [ $PART = 'cm' ]
            then
                number1=$RANDOM
                let "number1 %= 100"
                if [ "$number1" -le 50 ]
                then
                    echo "$jobid;$LINE;$PART,esb,dam" >> $5
                else
                    echo "$jobid;$LINE;$PART,dam,esb" >> $5
                fi
            elif [ $PART = 'esb' ]
            then
                echo "$jobid;$LINE;$PART,dam,cm" >> $5
            elif [ $PART = 'dam' ]
            then
                echo "$jobid;$LINE;$PART,esb,cm" >> $5
            fi
            ## PART is preferred module, but alternative modules should be added. TODO How do we add alternative modules, in which order? Both of them? 
            ## Provisional implementation at the moment
        else
            echo "$jobid;$LINE;-1" >> $5
        fi

        # Max number of flexible jobs minus 60
        if [ "$number" -le $fourth_p ] && [ "$number" -gt 0 ]
        then
            PART=$(echo "$LINE" | cut -d ';' -f 15)
            if [ $PART = 'cm' ]
            then
                number1=$RANDOM
                let "number1 %= 100"
                if [ "$number1" -le 50 ]
                then
                    echo "$jobid;$LINE;$PART,esb,dam" >> $6
                else
                    echo "$jobid;$LINE;$PART,dam,esb" >> $6
                fi
            elif [ $PART = 'esb' ]
            then
                echo "$jobid;$LINE;$PART,dam,cm" >> $6
            elif [ $PART = 'dam' ]
            then
                echo "$jobid;$LINE;$PART,esb,cm" >> $6
            fi
            ## PART is preferred module, but alternative modules should be added. TODO How do we add alternative modules, in which order? Both of them? 
            ## Provisional implementation at the moment
        else
            echo "$jobid;$LINE;-1" >> $6
        fi

 
        let "jobid++"
done < "$TEMP2"

