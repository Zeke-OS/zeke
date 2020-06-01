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


Documentation
-------------

- [Introduction to Zeke](/doc/README.md)
- [Build System](/doc/build.md)
- [Zeke Coding Standards and Guidelines](/doc/coding_standards.md)
