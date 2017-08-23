#!/usr/bin/env bash

TYPE=$1
PHASE=$2

if [ ${TYPE} == "T" ]; then
    TYPE="TRAIN"
else
    TYPE="FULL"
fi
#-hide-failed 
./tester -addr http://127.0.0.1:8080 -hlcupdocs ./hlcupdocs/data/${TYPE}/ -test -phase ${PHASE}
