#!/bin/bash

TOP_DIR=..

mkdir -p ${TOP_DIR}/build-release

pushd ${TOP_DIR}/build-release
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release ..
make
popd

docker build -t hlcup ..
