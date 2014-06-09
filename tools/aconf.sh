#!/bin/bash
# Parse a C header file from config.mk file.

if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: $0 [config.mk] [autoconf.h]"
    exit 1
fi
if [ ! -f "$1" ]; then
    echo "Config file '$1' doesn't exist"
    exit 1
fi

echo "/* Automatically generated file. */" >"$2"
echo -e "#pragma once\n#ifndef KERNEL_CONFIG_H\n#define KERNEL_CONFIG_H" >>"$2"
cat "$1"|sed 's/#.*$//g' |grep -e '^.*=.*$'|sed 's/\ *=\ */ /1'|sed 's/^/#define /' >>"$2"
echo "#endif" >>"$2"
