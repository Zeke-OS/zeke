#!/bin/bash -e
./configure --with-python=python3.4 --host=x86_64-linux-gnu --target=arm-none-eabi
make -j$(grep -c ^processor /proc/cpuinfo)
make install
