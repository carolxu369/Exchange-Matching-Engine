#!/bin/bash
NUM_REQUESTS=500
NUM_THREAD=2

for ((i = 0; i < $NUM_THREAD; ++i))
do
    if [ $i -eq $(($NUM_THREAD-1)) ]
    then
        ./client $NUM_REQUESTS &
    else
        ./client $NUM_REQUESTS &
    fi
done
