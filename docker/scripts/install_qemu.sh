#!/bin/bash -e
./configure --target-list=arm-softmmu,arm-linux-user,armeb-linux-user
make -j$(grep -c ^processor /proc/cpuinfo)
make install
