#!/bin/bash

if [ -z "$1" ]; then
    echo Usage: $0 SCRIPTFILE
    exit 1
fi

# QEMU makes decissions based on the filename given and we are more compatible
# with the linux behavior than the non-linux way of loading a kernel image.
ln -sf kernel.elf vmlinux-kernel.elf
exec tools/exec_qemu "$1" qemu-system-arm -kernel vmlinux-kernel.elf \
    -cpu arm1176 -m 256 -M raspi -nographic -serial stdio \
    -monitor telnet::4444,server,nowait -d unimp,guest_errors -s \
    -sd zeke-rootfs.img \
    -append 'console=ttyS0,115200 root=/dev/emmc0p1 rootfstype=fatfs'
