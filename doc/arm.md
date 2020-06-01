Running Zeke on ARM
===================

Zeke should boot just fine with, practically, any bootloader that is capable of
loading a Linux image. However, Zeke only supports ATAGs for gathering device
information, so some older ARM bootloader may work better. Although, ATAGs are
optional with Zeke and therefore practically any bootloader should work if you
run the build with the `configATAG` KConfig knob unset.

