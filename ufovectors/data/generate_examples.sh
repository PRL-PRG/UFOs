#!/bin/bash

function generate_both {
    echo "generating ${1}_rand_int.bin" 
    ./gen "${1}_rand_int.bin" $2 random

    echo "generating ${1}_ones_int.bin" 
    ./gen "${1}_ones_int.bin" $2 1
}
    
function generate_ones {
    echo "generating ${1}_ones_int.bin" 
    ./gen "${1}_ones_int.bin" $2 1
}

#                   T  G  M  K  1   
generate_both 1K             1000    # 3.9 KB
generate_both 100K         100000    # 392 KB 
generate_both 1M          1000000    # 3.8 MB
generate_both 10M        10000000    # 38  MB
generate_both 100M      100000000    # 381 MB
generate_ones 1G       1000000000    # 3.7 GB
#                   T  G  M  K  1   


