#!/bin/bash

# Check if all parameters are provided
if [ $# -ne 4 ]; then
    echo "Usage: $0 <executable> <protocol> <user> <num_instances>"
    echo "Example: $0 ./cliente UDP i0919688 10000"
    exit 1
fi

EXECUTABLE=$1
PROTOCOL=$2
ID=$3
NUM_INSTANCES=$4

# Validate if executable exists
if [ ! -x "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE is not executable or doesn't exist"
    exit 1
fi

# Spawn instances
for i in $(seq 1 $NUM_INSTANCES); do
    $EXECUTABLE $PROTOCOL $ID &
done

wait

echo "All $NUM_INSTANCES clients completed"