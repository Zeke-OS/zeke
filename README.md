Zero Kernel    {#mainpage}
===========

    |'''''||
        .|'   ...'||
       ||   .|...|||  ..  .... 
     .|'    ||    || .' .|...|| 
    ||......|'|...||'|. || 
                 .||. ||.'|...'

Zero Kernel is a tiny kernel implementation originally targeted for ARM Corex-M
microcontrollers. Reasons to start this project was that most of currently
available RTOSes for M0 feels too bloat and secondly I found  architectures
based on ARMv6-M to be quite challenging platforms for any kind of RTOS/kernel
development. Especially when ARMv6-M is compared to ARMv7-M used in M4 core or
any Cortex-A cores using real ARM architectures.

One of the original goals of Zero Kernel was to make it CMSIS-RTOS compliant
where ever possible. This is a quite tought task as CMSIS is not actually
designed for operating systems using system calls and other features not so
commonly used on embedded platforms. Currently the core of Zeke is not even
dependent of CMSIS libraries at all. Also as target is shifting Zeke now
only provides CMSIS-RTOS-like interface with lot of new features while some
braindead features are removed.

Key features
------------
- Dynamic prioritizing pre-emptive scheduling with penalties to prevent
  starvation
- System call based kernel services
- High configurability & Adjustable footprint
- BSD-like sysctl interface
- Kernel threads

News
----
- There will be optional support for full pre-emption of kernel mode
- Merging some things from freebsd (signal handling, sysctl)
- Dev subsys removed and will be soon replaced with devfs and
  virtual file system
- Coming "soon": Full POSIX process & thread support
- ARM11/Raspberry Pi port ongoing

Port status
-----------

    +-----------+-----------------------+
    | HAL       | Status                |
    +-----------+-----------------------+
    | ARM11     | OK                    |
    +-----------+-----------------------+
    | BCM2835   | Boots                 |
    +-----------+-----------------------+

Prerequisites
-------------

To successfully compile Zeke, you'll need the following packages/tools:

- make
- gcc-arm-none-eabi
- llvm-3.0
- clang 3.0
- binutils
- unifdef
- bash

Compiling
---------

Run make help for latests information about targets and compilation flags.

Run Zeke in emulator
--------------------

Qemu from rpi branch of git@github.com:Torlus/qemu.git repository seems to work
best for BCM2835/Raspberry Pi version of Zeke.

    ../src/configure --prefix=[PATH TO BIN] --target-list=arm-softmmu,arm-linux-user,armeb-linux-user --enable-sdl
    make -j4 && sudo make install

Run Zeke on real hw
-------------------

Zeke should boot just fine with practically any bootloader that is capable of
loading linux image. However Zeke only supports ATAGs so some older bootloader
(like found in Raspbian is better) but ATAGs are just extra so basically any
bootloader should work.

