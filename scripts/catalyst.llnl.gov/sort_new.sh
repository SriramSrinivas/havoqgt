#!/bin/bash
function lookup_page_id { echo -n $1 | sha1sum | cut -f 1 -d ' ' | xargs -I % grep -in -e % /p/lscratchf/havoqgtu/wikipedia-enwiki-20151201/output/pagelist_lookup;}

#sort the output based on visited frequency: most visited first. If the sorted has generated, then no need to execute these two commands again
filename="$1"
echo $filename
sortoutfilename="sorted_out"
sortoutfilename+=$filename
rankoutfilename="sorted_res"
rankoutfilename+=$filename
line=$(grep -m 1 "visted" $filename -n | cut -f1 -d:)
tail -n +$line $filename | sort -k5,5 -n -r > $sortoutfilename

#extract the IDs as the input to the python script, which is used to find the id-to-domain mapping
 ss=""
 cnt=0
 while read -r line; do
	echo "$line"
        #echo $line | cut -f 2 -d ' ' | xargs -I % grep -in -e /p/lscratchf/havoqgtu/WebDataCommons/2012/nodes/* 
	ss+=$( echo $line | cut -f 2 -d ' ')
	ss+=" "
	cnt=$(($cnt+1))	
	if [ $cnt -gt 20 ]; then
	  break;
 	fi
 done < $sortoutfilename
echo $ss
#$(python search.py $ss)
./search.py $rankoutfilename $ss

