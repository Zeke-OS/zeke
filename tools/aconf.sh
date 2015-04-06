#!/bin/bash
# Parse a C header file from config.mk files.

if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: $0 [config1.mk] ... [confign.mk] [autoconf.h]"
    exit 1
fi
if [ ! -f "$1" ]; then
    echo "Config file '$1' doesn't exist"
    exit 1
fi

HFILE="${@: -1}"

echo "/* Automatically generated file. */" >"$HFILE"
echo -e "#pragma once\n#ifndef KERNEL_CONFIG_H\n#define KERNEL_CONFIG_H" >>"$HFILE"
for conffile in "$@"; do
    if [ "$conffile" = "$HFILE" ]; then
        break
    fi
    cat "$conffile"|sed 's/#.*$//g'|grep -e '^.*=.*$'| \
        sed 's/=n$/=0/'|sed 's/=y$/=1/'|sed 's/=m$/=2/'| \
        sed 's/\ *=\ */ /1'| \
        sed 's/^/#define /' >>"$HFILE"
done
echo "#define KERNEL_VERSION \"$(git describe)\"" >>"$HFILE"
echo "#define KERNEL_RELENAME \"Tanforan\"" >>"$HFILE"
echo "#endif" >>"$HFILE"
