#!/bin/bash

# Check if all parameters are provided
if [ $# -ne 3 ]; then
    echo "Usage: $0 <executable> <protocol> <num_instances>"
    echo "Example: $0 ./cliente UDP 10000"
    exit 1
fi

EXECUTABLE=$1
PROTOCOL=$2
NUM_INSTANCES=$3

# Validate if executable exists
if [ ! -x "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE is not executable or doesn't exist"
    exit 1
fi

# Spawn instances
for i in $(seq 1 $NUM_INSTANCES); do
    $EXECUTABLE $PROTOCOL &
done

wait

echo "All $NUM_INSTANCES clients completed"