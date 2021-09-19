#!/bin/bash

target=(func)

for x in ${target[@]}; do
    cd $x || continue
    make
    cd ..
done

echo "sync with s1"
rsync -a --delete ./ s1:~/dpdk_train