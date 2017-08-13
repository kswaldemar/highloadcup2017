#!/bin/bash

mkdir -p build-release

pushd build-release
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release ..
make
popd

docker build -t hlcup .
