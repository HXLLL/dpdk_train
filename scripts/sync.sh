#!/bin/bash

host0="poweredge0-PowerEdge-R740"
ssh_name0="s0"
host1="poweredge1-PowerEdge-R740"
ssh_name1="s1"
dir="/home/huangxl/dpdk_train"
username="huangxl"

if [[ `hostname` == $host0 ]]; then
    echo "sync to poweredge1"
    rsync -a --delete "$dir/" $ssh_name1:$dir
elif [[ `hostname` == $host1 ]]; then
    echo "sync to poweredge0"
    rsync -a --delete "$dir/" $ssh_name0:$dir
fi
