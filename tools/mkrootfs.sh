#!/bin/bash

IMG=zeke-rootfs.img

function dir2img {
    local files=$(cat "$1/manifest" | sed "s|[^ ]*|$1/\0|g")
    mmd -i "$IMG" "$1"
    mcopy -i "$IMG" -s $files "::$1"
}

mkfs.msdos -C "$IMG" -F 16 16384
if [ $? -ne 0 ]; then
    exit 1
fi
dir2img lib
dir2img sbin
