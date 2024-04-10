#!/bin/bash

commands=(
 #   "../script/run_32 ./test/and_arith_test"
 #   "../script/run_32 ./test/and_pcg_test"
 #   "../script/run_10 ./test/aes_iknp_test"
 #   "../script/run_10 ./test/aes_2r_test"
    "../script/run_10 ./test/aes_wrk_test"
)

output_files=(
 #   "../output/and_arith_WAN_32parties.txt"
 #   "../output/and_pcg_WAN_32parties.txt"
 #   "../output/aes_iknp_WAN_10parties.txt"
 #   "../output/aes_2r_WAN_10parties.txt"
    "../output/aes_wrk_WAN_10parties.txt"
)

for i in "${!commands[@]}"; do
    for j in {1..10}; do
        echo "Executing command: ${commands[$i]}"
        eval "${commands[$i]}" | tee -a "${output_files[$i]}"
    done
done