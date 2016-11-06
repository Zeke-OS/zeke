#!/bin/bash

export MTOOLSRC=tools/mtools.conf

# Get core files from a file system image

if [ "$#" -ne 2 ]; then
    echo "Illegal number of arguments"
    exit 1
fi

IMG=$1
FILE=$2

mcopy -s "D:$FILE" ./
