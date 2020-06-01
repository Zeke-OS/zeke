Running Zeke on ARM
===================

Zeke should boot just fine with, practically, any bootloader that is capable of
loading a linux image. However, Zeke only supports ATAGs for gathering device
information so some older bootloader may work better. Alhough ATAGs are optional
therefore practically any bootloader should work if you run the build with
`configATAG` unset.

