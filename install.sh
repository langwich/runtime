#! /bin/bash

# get the directory contains this script
DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

# make install the project
BUILD_DIR=/tmp/wich-build
mkdir $BUILD_DIR 2>/dev/null
cd $BUILD_DIR && cmake $DIR && make install
