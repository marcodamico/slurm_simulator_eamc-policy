#!/bin/bash
  

if [ $# -ne 2 ]; then
    echo "./generator_modlist.sh <output_basic> <output>"
    exit 0
fi

rm $2 2> /dev/null
rm $3 2> /dev/null

TEMP=$1
replace="-1"

while IFS='' read -r line || [[ -n "$line" ]]; do
        MOD=$(echo "$line" | cut -d ';' -f 19)
        if [[ $MOD -ne "-1" ]]
        then
            #echo "$MOD"
            PART=$(echo $MOD | cut -d ',' -f 1)
            #echo "$PART"
            LINE=$(echo ${line/$MOD/-1}) 
            echo ${LINE/$PART/$MOD} >> $2
        else
            echo "$line" >> $2
        fi
done < "$TEMP"


#1;3;-1;592;8;-1;-1;-1;-1;-1;1;tester;-1;-1;1;dam;-1;-1;dam,cm
#2;4;-1;608;8;-1;-1;-1;-1;-1;1;tester;-1;-1;1;cm;-1;-1;cm,dam
#3;4;-1;608;8;-1;-1;-1;-1;-1;1;tester;-1;-1;1;esb;-1;-1;-1
#4;4;-1;6;16;-1;-1;-1;-1;-1;1;tester;-1;-1;0;dam;-1;-1;dam,esb
#5;5;-1;10;4;-1;-1;-1;-1;-1;1;tester;-1;-1;0;cm;-1;-1;cm,dam
#6;5;-1;10;4;-1;-1;-1;-1;-1;1;tester;-1;-1;0;esb;-1;-1;-1
