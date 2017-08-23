#!/bin/bash
root=~/Development/Projects/highloadcup2017

mkdir -p /tmp/hlcup-train-run
pushd /tmp/hlcup-train-run
ln -s ${root}/testing/hlcupdocs/data/TRAIN/data ./data
perf record -g ${root}/build-debug/web-server
popd
rm -rf /tmp/hlcup-train-run
