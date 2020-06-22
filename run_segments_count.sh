#!/bin/bash

exe=./segments_count
step=100
epsilon=50
length=1000000
iterations=10000
threads=$(getconf _NPROCESSORS_ONLN)
set -- -e$epsilon -s$step -n$length -i$iterations -t$threads

mkdir -p results

stdbuf -o 0 time $exe uniform 0 1 "$@" | tee count_uniform_0_1_met3_epsilon$epsilon.csv
stdbuf -o 0 time $exe pareto 2 2.5 "$@" | tee count_pareto_scale2_shape2.5_met1.25_epsilon$epsilon.csv
stdbuf -o 0 time $exe lognormal 1 1 "$@" | tee count_lognormal_m1_s1_met0.582_epsilon$epsilon.csv
stdbuf -o 0 time $exe gamma 1 1 "$@" | tee count_gamma_scale1_shape1_met1_epsilon$epsilon.csv
