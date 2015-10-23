#! /bin/bash

# test number of arguments
if [ $# -ne 1 ]
then
    echo "try $0 [install|test]"
    exit
fi

# get the directory contains this script
DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

# make install the project
BUILD_DIR=/tmp/wich-build
TEST_DIR=/tmp/cunit

function install {
    rm -rf $BUILD_DIR
    mkdir $BUILD_DIR 2>/dev/null
    cd $BUILD_DIR && cmake $DIR && make install    
}

function test {
    rm -rf $BUILD_DIR && mkdir $BUILD_DIR 2>/dev/null
    rm -rf $TEST_DIR && mkdir $TEST_DIR 2>/dev/null

    cd $BUILD_DIR && cmake $DIR >/dev/null  && make >/dev/null
    cp $DIR/cunit/addr2index.py $DIR/cunit/ANSIC_MALLOC_FREE_TRACE.txt $BUILD_DIR/cunit
    cd $BUILD_DIR && ctest ./
}

# switch on commands
if [[ $1 == "install" ]]; then
    install
elif [[ $1 == "test" ]]; then
    test
fi
