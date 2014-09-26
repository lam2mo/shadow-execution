#!/bin/bash

# ######################################
# Instrumenting NAS programs in $1
# ######################################


while read program
do
    echo "Instrumenting $program ..."
    ./instrument.sh $program .
done < $1
