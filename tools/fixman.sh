#!/bin/bash

cd doc/man
# Try to fix doxygen bug that causes kern man pages to be named incorrectly.
# This fix won't however fix headings in those man pages.
if [ -d man39 ]; then
    mv man39 man9
    cd man9
    rename .39 .9 *.39
fi
