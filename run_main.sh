#!/bin/bash

exe=./simulate
min_epsilon=1
max_epsilon=256
iterations=10000000
threads=$(getconf _NPROCESSORS_ONLN)
set -- -m$min_epsilon -M$max_epsilon -i$iterations -t$threads

mkdir -p results

stdbuf -o 0 time $exe uniform 0 1   "$@" | tee results/uniform_0_1_met3.csv
stdbuf -o 0 time $exe uniform 1 10  "$@" | tee results/uniform_0_10_met3.csv
stdbuf -o 0 time $exe uniform 1 100 "$@" | tee results/uniform_10_100_met4.481.csv

stdbuf -o 0 time $exe pareto 1 2.5 "$@" | tee results/pareto_scale2_shape2.5_met1.25.csv
stdbuf -o 0 time $exe pareto 1 3   "$@" | tee results/pareto_scale3_shape3_met3.csv
stdbuf -o 0 time $exe pareto 1 3.5 "$@" | tee results/pareto_scale4_shape3.5_met5.25.csv

stdbuf -o 0 time $exe lognormal 1 1    "$@" | tee results/lognormal_m1_s1_met0.582.csv
stdbuf -o 0 time $exe lognormal 1 0.75 "$@" | tee results/lognormal_m1_s0.75_met1.324.csv
stdbuf -o 0 time $exe lognormal 1 0.5  "$@" | tee results/lognormal_m1_s0.5_met3.521.csv

stdbuf -o 0 time $exe gamma 1 1 "$@" | tee results/gamma_scale1_shape1_met1.csv
stdbuf -o 0 time $exe gamma 2 3 "$@" | tee results/gamma_scale3_shape2_met2.csv
stdbuf -o 0 time $exe gamma 3 6 "$@" | tee results/gamma_scale6_shape3_met3.csv
