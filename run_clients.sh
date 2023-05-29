#!/bin/bash

echo "Enter the number of times to run the autoClient:"
read count

# Function to stop all client processes
stop_clients() {
    echo "Stopping the clients..."
    pkill -f autoHFTClientGang
}

# Register the stop_clients function to be called on script exit or interruption
trap stop_clients EXIT

# Run the clients concurrently
for ((i=0; i<count; i++))
do
    ./autoHFTClientGang & # Run each instance concurrently
done

# Continuous loop to read user input
while true; do
    read -r input

    # Check if the user wants to stop the clients
    if [[ $input == "s" ]]; then
        stop_clients
        break
    fi
done

