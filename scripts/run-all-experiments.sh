#!/bin/bash

# Usage: ./run_all_experiments <folder-to-configs> <output-dir>

mkdir -p $2
for CONFIG in ${1}/*
do
  echo $CONFIG
  filename=$(basename -- "$CONFIG")
  filename=${filename%.*}
  hexperiment $CONFIG ${2}/$(basename -- $filename)
done

