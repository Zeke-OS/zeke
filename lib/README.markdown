lib
===

User space libraries and crt that is used for both user psace programs and for
kernel. In particular this directory contains Zeke libc library that is needed
for user space work with Zeke.

Put here the CMSIS (and other) libraries provided by the chip manufacturers,
basically what ever you need.

*Recommended directory structure:*

    .
    |-crt
    |--armv6-m-libgcc
    |--libaebi-cortexm0
    |--rpi-libgcc
    |-STR912
    |--stdlib
    |---inc
    |---src
    |-libc
