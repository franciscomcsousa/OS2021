#!/bin/bash

inputdir=${1?Error: Invalid input directory}
outputdir=${2}
maxthreads=${3?Error: Invalid threads number}

mkdir ${outputdir}

for filepath in $inputdir/*.txt
do
    filename=$(basename $filepath) 
    for i in $(seq 1 ${maxthreads})
    do                                                               
        echo InputFile="${filename//.txt}" NumThreads="$i"                                            
        outputfilename=${filename//.txt}-$i                                                 
        ./tecnicofs $filepath ${outputdir}/${outputfilename}.txt $i | grep "TecnicoFS" 
    done     
done