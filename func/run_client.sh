#!/bin/bash

sudo pkill main

ARGS="-l 0 -- --isclient=1 -p 1 -n 20000000 -t 2"

if [[ $# == 0 ]]; then
    sudo ./build/main $ARGS
elif [[ $# == 1 ]]; then
    sudo gdb --args ./build/main $ARGS
fi
