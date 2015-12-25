#!/bin/bash

IMG=zeke-rootfs.img

export MTOOLSRC=tools/mtools.conf

function dir2img {
    local manifest="$(cat "$1/manifest")"
    if [ -z "$manifest" ]; then
        return 0
    fi
    local files=$(echo "$manifest" | sed "s|[^ ]*|$1/\0|g")

    mmd -D sS "C:$(echo $1 | sed 's|^\./||')"
    for file in $files; do
        local d=$(dirname $file | sed 's|^\./||')
        local destname="$d/$(basename $file)"
        mmd -D sS "C:$d"
        echo "INSTALL $destname"
        mcopy -s $file "C:$destname"
    done
}

dd if=/dev/zero of="$IMG" bs=1M count=10
mpartition -I -s 63 -t 20 -h 16 c:
mpartition -cpv -s 63 -t 20 -h 16 c:
mformat c:

# Copy the bootloader and kernel image
# TODO Get bootloader as an argument
BOOT="boot/rpi/bootcode.bin boot/rpi/config.txt boot/rpi/fixup.dat
    boot/rpi/start.elf boot/rpi/LICENSE.broadcom boot/rpi/cmdline.txt \
    kernel.img"
for file in $BOOT; do
    mcopy -s "$file" C:
    basename "$file"
done

# Create dirs
mmd C:dev
mmd C:mnt
mmd C:opt
mmd C:opt/test
mmd C:proc
mmd C:tmp
mmd C:root
mcopy README.markdown C:

# Copy binaries
MANIFEST=$(find . -name manifest -exec dirname {} \;)
for dir in $MANIFEST; do
    dir2img "$dir"
done
