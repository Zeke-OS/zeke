Running Zeke on ARM
===================

Zeke should boot just fine with, practically, any bootloader that is capable of
loading a Linux image. However, Zeke only supports ATAGs for gathering device
information, so some older ARM bootloader may work better. Although, ATAGs are
optional with Zeke and therefore practically any bootloader should work if you
run the build with the `configATAG` KConfig knob unset.

Running on Raspberry Pi
-----------------------

First, configure the build using `rpi.defconfig` as a template.

```
KBUILD_DEFCONFIG=rpi.defconfig ./configure defconfig
```

After this you may do some manual configuration changes by running:

```
./configure menuconfig
```

Then run the build:

```
make all opttest -j4
make rootfs
```

Finally you can copy the resulting rottfs image to an SD card.

```
dd if=zeke-rootfs.img of=/dev/sdX bs=1M
```

Boot it!
