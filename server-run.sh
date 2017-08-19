#!/bin/bash

# Place with zip on server side
SERVER_PATH=/tmp/data

echo "Extract zip"
if [ -r ${SERVER_PATH}/data.zip ]; then
    echo "Found data zip in tmp folder"
    unzip -j ${SERVER_PATH}/data.zip '*.json' -d data
else
    echo "Local run"
    mkdir -p data
    cp local-data/* data/
fi

echo "Run server"
./web-server
