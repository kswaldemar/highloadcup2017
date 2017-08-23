#!/bin/bash
root=~/Development/Projects/highloadcup2017

mkdir -p /tmp/hlcup-full-run
pushd /tmp/hlcup-full-run
ln -s ${root}/testing/hlcupdocs/data/FULL/data ./data
#perf record -g 
${root}/build-release/web-server
popd
