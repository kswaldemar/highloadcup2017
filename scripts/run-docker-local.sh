#!/bin/bash
#DATA=TRAIN
DATA=FULL
docker run --rm -p 8080:80 -v "/home/valdemar/Development/Projects/highloadcup2017/testing/hlcupdocs/data/${DATA}/data:/root/data" -t hlcup:latest
