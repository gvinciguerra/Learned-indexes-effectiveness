#!/bin/bash

exe=./simulate
min_epsilon=1
max_epsilon=256
iterations=10000000
threads=$(getconf _NPROCESSORS_ONLN)
set -- -m$min_epsilon -M$max_epsilon -i$iterations -t$threads --met

mkdir -p results

stdbuf -o 0 time $exe pareto 10 7.741249472052228126504329 "$@" | tee results/assum0.15_pareto_scale10_shape7.741.csv
stdbuf -o 0 time $exe pareto 10 2.201850425154663097706407 "$@" | tee results/assum1.5_pareto_scale10_shape2.202.csv
stdbuf -o 0 time $exe pareto 10 2.002219758558193884725262 "$@" | tee results/assum15_pareto_scale10_shape2.002.csv

stdbuf -o 0 time $exe gamma 44.444444444444444444444444 5 "$@" | tee results/assum0.15_gamma_scale5_shape44.444.csv
stdbuf -o 0 time $exe gamma 0.4444444444444444444444444 5 "$@" | tee results/assum1.5_gamma_scale5_shape0.444.csv
stdbuf -o 0 time $exe gamma 0.0044444444444444444444444 5 "$@" | tee results/assum15_gamma_scale5_shape0.004.csv

stdbuf -o 0 time $exe lognormal 2 0.1491663800419510041113660 "$@" | tee results/assum0.15_lognormal_m2_s0.149.csv
stdbuf -o 0 time $exe lognormal 2 1.0856587844906179900527186 "$@" | tee results/assum1.5_lognormal_m2_s1.086.csv
stdbuf -o 0 time $exe lognormal 2 2.3282042434615322807798765 "$@" | tee results/assum15_lognormal_m2_s2.328.csv
