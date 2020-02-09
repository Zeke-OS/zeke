Introduction to Zeke
====================

Zeke is a portable operating system mostly targetted for ARM
processors.\[1\] Zero Kernel, as it was originally known, is a tiny
kernel implementation that was originally targeted for ARM Corex-M
microcontrollers. The reason to start this project was that most of
RTOSes available at the time for M0 feeled less or more bloated for very
simple applications. Secondly I found architectures based on ARMv6-M to
be quite challenging and interesting platforms for RTOS/kernel
development, especially when ARMv6-M is compared to ARMv7-M used in M4
core or any Cortex-A cores using real ARM architectures.

One of the original goals of Zero Kernel was to make it CMSIS-RTOS
compliant where possible, as some concepts of Zeke were not actually
CMSIS compliant from the begining. However the scope of the project
shifted pretty early and the kernel is no longer CMSIS compatible at any
level. Currently Zeke is moving towards POSIX-like system and its user
space is taking a Unix-like shape. Nowadays Zeke is a bit bloatty when
compared to the original standard of a bloated OS but I claim Zeke is
still quite tightly integrated system, compared to any other Unix-like
OS implementation.

Figure [\[figure:zeke\]](#figure:zeke) illustrates architectural layers
of the operating system. Scope of Zeke project is to implement a
portable Unix-like\[2\] operating system from scratch\[3\] that is
optimized for ARM architectures and is free of legacy.

In addition to portability another long standing goal has been
configurability and adjustable footprint of the kernel binary as well as
the whole OS image. This is achieved by modular kernel architecture and
static compile time configuration. Like for example
<span data-acronym-label="HAL" data-acronym-form="singular+short">HAL</span>
selection is already almost fully locked at compilation time.

Zeke includes a partially BSD compatible C standard library.

![Zeke: a Portable Operating
System.<span label="figure:zeke"></span>](pics/zeke.svg)

TODO Tell about the doc strucuture

There is a `coding_standards.markdown` file that you should read before
contributing. That document describes the guidelines for development and
contribution as well as gives some information about calling conventions and
initialization of kernel subsystems.

This directory also contains some auto generated as well as
manually written man pages under `man` and `sman` directories.

List of Abbreviations and Meanings
----------------------------------

- \[POSIX\] <span>Portable Operating System Interface for uniX</span> is a family of
  standards specifying a standard set of features for compatibility
  between operating systems.
- \[HAL\] <span>Hardware Abstraction Layer</span>.

- \[inode\] <span>inode</span> is the file index node in Unix-style file systems that is
- used to represent a file fie system object.
- \[LFN\] <span>Long File Name</span>.
- \[MBR\] <span>Master Boot Record</span>.
- \[ramfs\] <span>RAM file system</span>.
- \[SFN\] <span>Short File Name</span>.
- \[VFS\] <span>Virtual File System</span>.
- \[vnode\] <span>vnode</span> is like an inode but it's used as an abstraction
  level for compatibility between different file systems in Zeke. Particularly
  it's used at VFS level.

- \[bio\] <span>IO Buffer Cache</span>.
- \[dynmem\] (<span>dynamic memory</span>) is the block memory allocator system in Zeke,
  allocating in 1 MB blocks.
- \[VRAlloc\] or \[vralloc] <span>Virtual Region Allocator</span>.

- \[MIB\] <span>Management Information Base</span> tree.

- \[IPC\] </span>Inter-Process Communication</span>.
- \[shmem\] <span>Shared Memory</span>.
- \[RCU\] <span>Read-Copy-Update</span>.

- \[ELF\] <span>Executable and Linkable or Extensible Linking Format</span>.

- \[PUnit\] a portable unit testing framework for C.
- \[GCC\] GNU Compiler Collection.
- \[GLIBC\] The GNU C Library.
- \[CPU\] <span>Central Processing Unit</span> is, generally speaking, the main
  processor of some computer or embedded system.

- \[DMA\] <span>Direct Memory Access</span> is a feature that allows hardware
  subsystems to commit memory transactions without CPU's intervention.
- \[PC\] <span>Program Counter</span> register.

Footnotes
---------

1.  Zeke is portable to at least in a sense of portability between
    different ARM cores and architectures.
2.  The project is aiming to implement the core set of kernel features
    specified in
    <span data-acronym-label="POSIX" data-acronym-form="singular+short">POSIX</span>
    standard.
3.  At least almost from scratch, some functionality is derived from BSD
    kernels as well as some libraries are taken from other sources.
