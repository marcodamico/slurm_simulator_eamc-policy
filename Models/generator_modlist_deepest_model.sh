
#!/bin/bash

TEMP='trace.tmp'
TEMP2='jobid.tmp'

if [ $# -ne 3 ]; then
    echo "./generator_modlist.sh <num_jobs> <output_basic> <output> "
    exit 0
fi

rm $2 2> /dev/null
rm $3 2> /dev/null

./m_lublin99 $1 50 cm 1 > $TEMP
./m_lublin99 $1 75 esb 1 >> $TEMP
./m_lublin99 $1 16 dam 1 >> $TEMP

cat $TEMP | sort -n -t ";" -k2 > $TEMP2


jobid=1
while IFS='' read -r line || [[ -n "$line" ]]; do
        LINE=$(echo "$line" | cut -d ';' -f2-)
        echo "$jobid;$LINE;-1" >> $2 ## Create basic trace without module list to be used for comparison 
        PART=$(echo "$LINE" | cut -d ';' -f 15)
        if [ $PART = 'cm' ]
        then    
            echo "$jobid;$LINE;$PART,dam" >> $3
        elif [ $PART = 'esb' ]
        then
            number=$RANDOM
            let "number %= 100"
            if [ "$number" -le 50 ]
            then
                echo "$jobid;$LINE;$PART,cm" >> $3
            else
                echo "$jobid;$LINE;-1" >> $3
            fi
         elif [ $PART = 'dam' ]
         then   
            number1=$RANDOM
            let "number1 %= 100"
            if [ "$number1" -le 57 ]
            then
                number2=$RANDOM
                let "number2 %= 100"
                if [ "$number2" -le 75 ]
                then
                    echo "$jobid;$LINE;$PART,esb" >> $3
                else    
                    echo "$jobid;$LINE;$PART,cm" >> $3
                fi
            else
                echo "$jobid;$LINE;-1" >> $3
            fi 
         else
            echo "$jobid;$LINE;-1" >> $3
         fi
         let "jobid++"
done < "$TEMP2"

