Zero Kernel    {#mainpage}
===========

    |'''''||
        .|'   ...'||
       ||   .|...|||  ..  .... 
     .|'    ||    || .' .|...|| 
    ||......|'|...||'|. || 
                 .||. ||.'|...'

Zeke is a tiny Unix-like operating system implementation that has grown up from
an even smaller CMSIS-like embedded system.

Key Features
------------
- Kernel
    - Fully pre-emptible kernel mode
    - Object-oriented thread scheduling system
    - One-to-one kernel threads for user processes
    - freeBSD-like sysctl interface
- Processes
    - ASLR
    - Copy-On-Write virtual memory
    - Per process capabilities
    - Unix-like fork and exec
    - elf32 support
    - Linux-style elf32 core dumps
- IPC
    - Signals
    - mmap
    - pipes
    - pty
- File Systems
    - Complete file system abstraction (VFS)
    - FAT12/16/32 support
    - Fast RAM file system
    - MBR support
    - freeBSD-like device file interface
- Userland
    - Mostly C99 compliant libc
    - Standard user application separation by using POSIX processes
    - System call based kernel services

News
----

Scheduler system redesigned to provide easier API to implement new scheduling
policies as well as to make future migration to MP easier.

Port Status
-----------

    +-----------+-----------------------+
    | HAL       | Status                |
    +-----------+-----------------------+
    | ARM11     | OK                    |
    +-----------+-----------------------+
    |   BCM2835 | OK                    |
    +-----------+-----------------------+
    | MIPSel32  | -                     |
    +-----------+-----------------------+
    |   JZ4780  | -                     |
    +-----------+-----------------------+


Compiling and Testing
---------------------

### Prerequisites

To successfully compile Zeke, you'll need the following packages/tools:

- `make` >3.8
- `llvm` 3.3 or 3.4
- `clang` 3.3 or 3.4
- `binutils`
-- arm-none-eabi
-- mipsel-sde-elf
- `bash`
- `mkdosfs` and `mtools` for creating a rootfs image
- `cloc` for source code stats
- `ncurses-devel`

doc-book target:

- LaTeX + various packages
- latexmk
- gnuplot

Doxygen targets:

- Doxygen
- man for viewing man pages

### Compiling

+ `configure`- Set defaults for files in config dir
+ `make all rootfs` - Compile the kernel, user space and create a rootfs image
+ `make kernel.img` - Compile the kernel
+ `make world` - Compile only the user space without creating a rootfs image
+ `make opttest` - Compile on target tests
+ `make clean` - Clean all compilation targets

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

