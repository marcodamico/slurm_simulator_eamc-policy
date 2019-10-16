
#!/bin/bash

TEMP='trace.tmp'
TEMP2='jobid.tmp'

if [ $# -ne 5 ]; then
    echo "./generator_modlist.sh <num_jobs> <output_basic> <output> <output_less_flex1> <output_less_flex2>"
    exit 0
fi

rm $2 2> /dev/null
rm $3 2> /dev/null

./m_lublin99 6000 50 cm 52 > $TEMP
./m_lublin99 6000 75 esb 34 >> $TEMP
./m_lublin99 6000 16 dam 500 >> $TEMP

cat $TEMP | sort -n -t ";" -k2 > $TEMP2


jobid=1
while IFS='' read -r line || [[ -n "$line" ]]; do
        LINE=$(echo "$line" | cut -d ';' -f2-)
        echo "$jobid;$LINE;-1" >> $2 ## Create basic trace without module list to be used for comparison 
        PART=$(echo "$LINE" | cut -d ';' -f 15)
        if [ $PART = 'cm' ]
        then    
            echo "$jobid;$LINE;$PART,dam" >> $3
            number3=$RANDOM
            let "number3 %= 100"
            if [ "$number3" -le 85 ]
            then
                echo "$jobid;$LINE;$PART,dam" >> $4
            else
                echo "$jobid;$LINE;-1" >> $4
            fi
            if [ "$number3" -le 70 ]
            then
                echo "$jobid;$LINE;$PART,dam" >> $5
            else
                echo "$jobid;$LINE;-1" >> $5
            fi
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
            if [ "$number" -le 35 ]
            then
                echo "$jobid;$LINE;$PART,cm" >> $4
            else
                echo "$jobid;$LINE;-1" >> $4
            fi
            if [ "$number" -le 20 ]
            then
                echo "$jobid;$LINE;$PART,cm" >> $5
            else
                echo "$jobid;$LINE;-1" >> $5
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
            if [ "$number1" -le 43 ]
            then
                number4=$RANDOM
                let "number4 %= 100"
                if [ "$number4" -le 75 ]
                then
                    echo "$jobid;$LINE;$PART,esb" >> $4
                else    
                    echo "$jobid;$LINE;$PART,cm" >> $4
                fi
            else
                echo "$jobid;$LINE;-1" >> $4
            fi 
            if [ "$number1" -le 28 ]
            then
                number5=$RANDOM
                let "number5 %= 100"
                if [ "$number5" -le 75 ]
                then
                    echo "$jobid;$LINE;$PART,esb" >> $5
                else    
                    echo "$jobid;$LINE;$PART,cm" >> $5
                fi
            else
                echo "$jobid;$LINE;-1" >> $5
            fi 
         else
            echo "$jobid;$LINE;-1" >> $3
            echo "$jobid;$LINE;-1" >> $4
            echo "$jobid;$LINE;-1" >> $5
         fi
         let "jobid++"
done < "$TEMP2"

