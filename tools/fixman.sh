#!/bin/bash

VERCMP=$(dirname $0)/vercomp.sh
DXG_VER=$(doxygen --version)

$VERCMP "$DXG_VER" "1.8.8"
if [ $? -eq 2 ]; then # This was fixed in 1.8.8
    # Try to fix doxygen bug that causes kern man pages to be named incorrectly.
    # This fix won't however fix headings in those man pages.
    cd doc/man
    if [ -d man39 ]; then
        mv man39 man9
        cd man9
        rename .39 .9 *.39
    fi
fi
