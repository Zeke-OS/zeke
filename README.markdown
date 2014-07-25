Zero Kernel    {#mainpage}
===========

    |'''''||
        .|'   ...'||
       ||   .|...|||  ..  .... 
     .|'    ||    || .' .|...|| 
    ||......|'|...||'|. || 
                 .||. ||.'|...'

Zero Kernel is a tiny kernel implementation that was originally targeted for
ARM Corex-M microcontrollers. Reasons to start this project was that most of
currently available RTOSes for M0 were bit bloat and secondly I found
architectures based on ARMv6-M to be quite challenging platforms for RTOS/kernel
development. Especially when ARMv6-M is compared to ARMv7-M used in M4 core or
any Cortex-A cores using real ARM architectures.

One of the original goals of Zero Kernel was to make it CMSIS-RTOS compliant
where possible, as some concepts of Zeke were not CMSIS compliant from the
begining. However scope changed early and the kernel is no more CMSIS compatible
at any level. Currently Zeke is no moving towards POSIX-like system.

Key Features
------------
- For user applications
    - Dynamic prioritizing pre-emptive scheduling with penalties to prevent
      starvation
    - System call based kernel services
    - Standard user application separation by using POSIX processes
    - Many-to-Many thread scheduling implemented at kernel level
- Scalability
    - Adjustable footprint by compilation flags
- File Systems
    - Complete file system abstraction (VFS)
    - freeBSD-like device file interface
- Kernel
    - Fully pre-emptible kernel mode
    - freeBSD-like sysctl interface

News
----
- Coming "soon":
    - SD card support and file system X (tbd)
- ARM11/Raspberry Pi port ongoing

Port Status
-----------

    +-----------+-----------------------+
    | HAL       | Status                |
    +-----------+-----------------------+
    | ARM11     | OK                    |
    +-----------+-----------------------+
    | BCM2835   | Boots                 |
    +-----------+-----------------------+


Compiling and Testing
---------------------

### Prerequisites

To successfully compile Zeke, you'll need the following packages/tools:

- make >3.8
- gcc-arm-none-eabi
- llvm-3.3
- clang 3.3
- binutils
- bash
- cloc (for stats)

doc-book target:

- LaTeX + some packages
- latexmk
- gnuplot

Doxygen targets:

- Doxygen
- man for viewing man pages

### Compiling

+ `configure`- Set defaults for files in config dir
+ `make` - Compile the kernel
+ `make test` - Compile unit tests
+ `make clean` - Clean all targets

Run `make help` for full list of targets and compilation flags.

### Running Zeke in QEMU

Qemu from rpi branch of `git@github.com:Torlus/qemu.git` repository seems to work
best for BCM2835/Raspberry Pi version of Zeke.

    ../src/configure --prefix=[PATH TO BIN] --target-list=arm-softmmu,arm-linux-user,armeb-linux-user --enable-sdl
    make -j4 && sudo make install

### Running Zeke on real ARM

Zeke should boot just fine with, practically, any bootloader that is capable of
loading a linux image. However Zeke only supports ATAGs for gathering device
information so some older bootloader may work better. Though ATAGs are optional
so basically any bootloader should work if you compile Zeke without configATAG.

