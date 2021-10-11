#!/bin/bash

sudo pkill main

ARGS="-l 0 -- -p 3"

if [[ $# == 0 ]]; then
    sudo ./build/main $ARGS
elif [[ $# == 1 ]]; then
    sudo gdb --args ./build/main $ARGS
fi
