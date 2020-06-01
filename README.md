Zero Kernel    {#mainpage}
===========

    |'''''||
        .|'   ...'||
       ||   .|...|||  ..  .... 
     .|'    ||    || .' .|...|| 
    ||......|'|...||'|. || 
                 .||. ||.'|...'

Zeke is a tiny Unix-like operating system implementation that has grown up from
a tiny single-user CMSIS-like embedded operating system.

**Sonarcloud**

https://sonarcloud.io/dashboard?id=Zeke-OS_zeke

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

Port Status
-----------

| HAL                   | Status        | Documentation                        |
|-----------------------|---------------|--------------------------------------|
| **ARM11**             | Stable        | [Running Zeke on ARM](/doc/arm.md)   |
| &nbsp;&nbsp;BCM2835   | Stable        |                                      |
| &nbsp;&nbsp;QEMU      | Stable        | [Running Zeke in QEMU](/doc/qemu.md) |
| **MIPSel32**          | Incomplete    | [Running Zeke on MIPS](/doc/mips.md) |
| &nbsp;&nbsp;JZ4780    | Incomplete    |                                      |


Build
-----

See [doc/build.md](/doc/build.md) for details about the build system.

### Prerequisites

To successfully compile Zeke, you'll need the following packages/tools:

- GNU `make` >3.8
- `llvm` 3.3 or 3.4
- `clang` 3.3 or 3.4
- `binutils` 2.24
    - `arm-none-eabi`
    - `mipsel-sde-elf`
- `bash`
- `mkdosfs` and `mtools` for creating a rootfs image
- `cloc` for source code stats
- `ncurses-devel`

**Doxygen targets**

- Doxygen
- `man` for viewing man pages

### Compiling

+ `configure`- Set defaults for files in config dir
+ `make all rootfs` - Compile the kernel, user space and create a rootfs image
+ `make kernel.img` - Compile the kernel
+ `make world` - Compile only the user space without creating a rootfs image
+ `make opttest` - Compile on target tests
+ `make clean` - Clean all compilation targets

Run `make help` for full list of targets and compilation flags.

```bash
$ KBUILD_DEFCONFIG=qemu_rpi_headless_testing.defconfig ./configure defconfig
$ ./configure menuconfig
$ make all -j4
```

`KBUILD_DEFCONFIG` can be set as any of the defconfig targest found under
the `defconfigs` directory.

Finally you can build the rootfs image by running:

```bash
$ make rootfs
```
