#! /bin/bash

# test number of arguments
if [ $# -ne 1 ]
then
    echo "try $0 [install|test]"
    exit
fi

# get the directory contains this script
DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

# environment variables
export WICHRUNTIME=$DIR

# make install the project
BUILD_DIR=/tmp/wich-build/
SAMPLE_DIR=$BUILD_DIR/malloc/samples/

function install {
    rm -rf $BUILD_DIR
    mkdir $BUILD_DIR 2>/dev/null
    cd $BUILD_DIR && cmake $DIR && make install    
}

function test {
    rm -rf $BUILD_DIR && mkdir $BUILD_DIR 2>/dev/null
    cd $BUILD_DIR && cmake $DIR >/dev/null  && make >/dev/null
    
    rm -rf $SAMPLE_DIR && mkdir $SAMPLE_DIR 2>/dev/null
    cp $DIR/malloc/samples/addr2index.py $SAMPLE_DIR
    cp $DIR/malloc/samples/ANSIC_MALLOC_FREE_TRACE.txt $SAMPLE_DIR
    
    cd $BUILD_DIR && ctest ./
}

# switch on commands
if [[ $1 == "install" ]]; then
    install
elif [[ $1 == "test" ]]; then
    test
fi
