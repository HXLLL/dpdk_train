#!/bin/bash

TARGET=func

cd $TARGET
make
cd ..

# REMOTE_HOST=(cu0{2,3,4})
REMOTE_HOST=(s1)
DPU_HOST=d

LOCAL_DIR=/home/huangxl/Repo/dpdk_train
REMOTE_DIR=/home/huangxl/Repo/dpdk_train
DPU_DIR=/home/huangxl/dpdk_train

for remote in ${REMOTE_HOST[@]}; do
    rsync -a --delete $LOCAL_DIR/ $remote:$REMOTE_DIR
done

set -x;

rsync -a --delete $LOCAL_DIR/ $DPU_HOST:$DPU_DIR; ssh $DPU_HOST "bash -c \"cd $DPU_DIR/$TARGET; pwd; make clean; make\""