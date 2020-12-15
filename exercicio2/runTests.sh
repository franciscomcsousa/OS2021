#!/bin/bash

INPUTDIR=${1?Error: Invalid input directory}
OUTPUTDIR=${2?Error: Invalid output directory}
MAXTHREADS=${3?Error: Invalid threads number}

# -d FILE -> FILE exists and is a directory
if [ ! -d "$INPUTDIR" ]
then
    echo "Error: $INPUTDIR is not a valid directory"

# -e FILE -> FILE exists
elif [ -e "$OUTPUTDIR" ]
then
    echo "Error: $OUTPUTDIR already exists"

# INTEGER1 -lt INTEGER2 -> INTEGER1 is numerically less than INTEGER2
elif [ "$MAXTHREADS" -lt 0 ]
then
    echo "Error: $MAXTHREADS is not a valid number of threads"

else
    mkdir ${OUTPUTDIR}
    for FILEPATH in "$INPUTDIR"/*.txt
    do
        #basename "FILE" -> extracts filename and extension from FILE (removes directory from filename)
        FILENAME=$(basename "$FILEPATH") 
        #seq INTEGER1 INTEGER2 -> goes from INTERGER1 to INTEGER2 
        for i in $(seq 1 ${MAXTHREADS})                          
        do                                                               
            echo InputFile= "${FILENAME}" NumThreads= "$i"                                            
            OUTPUTFILENAME=${FILENAME//.txt}-$i    
            #grep "TEXT" -> filters output to show only where TEXT appears                                             
            ./tecnicofs "$FILEPATH" ${OUTPUTDIR}/${OUTPUTFILENAME}.txt $i | grep "TecnicoFS" 
        done     
    done
fi