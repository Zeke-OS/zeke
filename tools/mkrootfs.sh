#!/bin/bash

IMG=zeke-rootfs.img

function dir2img {
    local manifest="$(cat "$1/manifest")"
    if [ -z "$manifest" ]; then
        return 0
    fi
    local files=$(echo "$manifest" | sed "s|[^ ]*|$1/\0|g")

    mmd -i "$IMG" "$1"
    for file in $files; do
        local d=$(dirname $file | sed 's|^\./||')
        mmd -i "$IMG" -D sS "$d"
        echo "$d/$(basename $file)"
        mcopy -i "$IMG" -s $file "::$d"
    done
}

mkfs.msdos -C "$IMG" -F 16 16384
if [ $? -ne 0 ]; then
    exit 1
fi

# Copy bootloader and kernel image
# TODO Get bootloader as an argument
BOOT="boot/rpi/bootcode.bin boot/rpi/config.txt boot/rpi/fixup.dat
    boot/rpi/start.elf boot/rpi/LICENSE.broadcom boot/rpi/cmdline.txt \
    kernel.img"
for file in $BOOT; do
    mcopy -i "$IMG" -s "$file" "::"
    basename "$file"
done

# Copy binaries
mmd -i "$IMG" dev
mmd -i "$IMG" mnt
mmd -i "$IMG" opt
mmd -i "$IMG" opt/test
mmd -i "$IMG" proc
mmd -i "$IMG" tmp
mmd -i "$IMG" root
mcopy -i "$IMG" -s README.markdown "::root"

MANIFEST=$(find . -name manifest -exec dirname {} \;)
for dir in $MANIFEST; do
    dir2img $dir
done
