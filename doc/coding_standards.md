Zeke Coding Standards and Guidelines
====================================

Here is some misc documentation and general guidelines on topic of how to write
new code for Zeke.

Source Tree Directory Structure
-------------------------------

+ `kern/include/`           Most of the shared kernel header files.
+ `kern/`                   Most of the kernel code.
+ `kern/fs/`                Virtual file system abstraction and file systems.
+ `kern/hal/`               Harware abstraction layer.
+ `kern/kunit/`             In-kernel unit test framework (KUnit).
+ `kern/libkern/`           Kernel "standard" library.
+ `kern/libkern/kstring/`   String functions.
+ `kern/sched/`             Thread scheduling.
+ `kern/test/`              Kernel space unit tests.
+ `bin/`                    Essential commands.
+ `config/`                 Kernel build config.
+ `include/`                User space library headers.
+ `lib/`                    C runtime libraries and user space libraries.
+ `modmakefiles/`           Kernel module makefiles.
+ `sbin/`                   Essential system utilities.
+ `opt/test/`               User space unit tests and PUnit unit test framework.
+ `tools/`                  Build tools and scripts.
+ `usr/examples/`           Examples programs for Zeke.
+ `usr/games/`              Sources for games and demos.


Root Filesystem Hierarchy
-------------------------

+ `/bin/`                   Essential command binaries.
+ `/dev/`                   Virtual file system providing access to devices.
+ `/lib/`                   Essential libraries for /bin/ and /sbin/.
+ `/mnt/`                   Mount point for temporarily mounted filesystems.
+ `/proc/`                  Virtual filesystem providing process information.
+ `/sbin/`                  Essential system binaries.
+ `/tmp/`                   Temporary files.
+ `/usr/games/`             Games and demos.
+ `kernel.img`              The kernel image.


Naming Conventions
------------------

### File names

+ `subsys.c|h`  Any module that implements some functionality
+ `subsys.c|h`  Kernel scope source module that provides some external
                sycallable functionality
+ `ksubsys.c`   Kernel part of a well known subsystem, for example `ksignal.c`
+ `lowlevel.S`  Assembly source code; Note that capital S for user files as file
                names ending with small s are reserved for compilation time
                files

*Warning:* Assembly sources with small suffix (.s) will be removed by
`make clean` as those are considered as automatically generated files.

### Global variables

+ `module_feature_name`

### Function names

+ `module_comp_function` + module = name that also appears in the filename
                         + comp   = component/functionality eg. thread
                                    components will change thread status


Standard Data Types
-------------------

### Typedefs

Typedefs were historically used in Zeke for most of the structs and portable
types. New code should only use typedefs for portability where we may want
or have to change the actual underlying type depending on the actual hardware
platform. Usually there should be no need to use typedef for structs unles it's
stated in POSIX or some other standard we wan't to follow.

Typedefs can be also used if the user of the data doesn't need to know the
exact type information for the data, i.e. the user is only passing a pointer
to the data. In this case the actual data might be stored in a struct only
defined in a source file and referenced elsewhere (i.e. headers) using a
forward declaration and a pointer.

### Enums

Avoid using enums in kernel space, they are ugly, doesn't behave nicely, and
doesn't add any additional protection. Usually enums even seems to generate
more code than using #defined values. Though there might be some good use
cases for enums due to better debuggability with GDB.

Enums are ok in user space and in interfaces between user space and kernel
space. Some standard things may even require using enums. Especially some
POSIX interfaces require use of enums.


ABI and Calling Convention
--------------------------

Zeke is mainly utilizing the default calling convention defined by GCC and
Clang, which is a bit different from the standard calling convention
defined by ARM. Here follows is a brief description of ABI and calling
convention used in Zeke.

| Register | Alt. | Usage                                               |
|----------|------|-----------------------------------------------------|
| r0       |      | First function arg./Return value for sizeof(size_t) |
|          |      | return values.                                      |
| r1       |      | Second function argument.                           |
| r2       |      | Third function argument.                            |
| r3       |      | Fourth function argument.                           |
| r4       |      | Local variable. (non stacked scratch for syscalls)  |
| r5 - r8  |      | Local variables.                                    |
| r9       | rfp  | Local variable/Real frame pointer?                  |
| r10      | sl   | Stack limit?                                        |
| r11      |      | Argument pointer.                                   |
| r12      | ip   | Temporary workspace?                                |
| r13      | sp   | Stack pointer.                                      |
| r14      | lr   | Link register.                                      |
| r15      | pc   | Program counter.                                    |

Table partially based on:

- http://en.wikipedia.org/wiki/Calling_convention
- http://www.ethernut.de/en/documents/arm-inline-asm.html

If return value is a pointer or otherwise too large to be returned by pointer
stored in r0.

Stack is always full-descending.
