#!/bin/bash

INPUTDIR=${1?Error: Invalid input directory}
OUTPUTDIR=${2?Error: Invalid output directory}
MAXTHREADS=${3?Error: Invalid threads number}

if [ ! -d "$INPUTDIR" ]
then
    echo "Error: $INPUTDIR is not a valid directory"

elif [ -e "$OUTPUTDIR" ]
then
    echo "Error: $OUTPUTDIR already exists"

elif [ "$MAXTHREADS" -lt 0 ]
then
    echo "Error: $MAXTHREADS is not a valid number of threads"

else
    mkdir ${OUTPUTDIR}
    for FILEPATH in $INPUTDIR/*.txt
    do
        FILENAME=$(basename $FILEPATH) 
        for i in $(seq 1 ${MAXTHREADS})
        do                                                               
            echo InputFile="${FILENAME//.txt}" NumThreads="$i"                                            
            OUTPUTFILENAME=${FILENAME//.txt}-$i                                                 
            ./tecnicofs $FILEPATH ${OUTPUTDIR}/${OUTPUTFILENAME}.txt $i | grep "TecnicoFS" 
        done     
    done
fi