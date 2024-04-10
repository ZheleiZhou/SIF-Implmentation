#!/bin/bash

commands=(
    "../script/run_3 ./test/aes_iknp_test"
)

output_files=(
    "../output/aes_iknp_WAN_3parties.txt"
)

for i in "${!commands[@]}"; do
    for j in {1..10}; do
        echo "Executing command: ${commands[$i]}"
        eval "${commands[$i]}" | tee -a "${output_files[$i]}"
    done
done