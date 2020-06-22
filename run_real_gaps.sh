#!/bin/bash

exe=./real_gaps
min_epsilon=32
max_epsilon=1024
threads=$(getconf _NPROCESSORS_ONLN)
datasets=(books_200M_uint64 fb_200M_uint64)

(
  mkdir -p data
  cd data || exit
  wget -nc --content-disposition https://dataverse.harvard.edu/api/access/datafile/:persistentId?persistentId=doi:10.7910/DVN/JGVF9A/{EATHF7,A6HDNT}
  unzstd -T$threads "${datasets[@]/%/.zst}"
)

mkdir -p results
stdbuf -o 0 time $exe "${datasets[@]/#/data/}" -b -m$min_epsilon -M$max_epsilon -t$threads | tee results/real_gaps.csv
