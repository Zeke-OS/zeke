Zeke Coding Standards & Generic Documentation
=============================================

Here is some misc documentation and general guidelines on topic of how to write
new code for Zeke.

Source Tree Directory Structure
-------------------------------

+ kern/include/         Most of the shared kernel header files.
+ kern/                 Most of the kernel code.
+ kern/fs/              Virtual file system abstraction and other file systems.
+ kern/hal/             Harware abstraction layer.
+ kern/kunit/           In-kernel unit test framework (KUnit).
+ kern/libkern/         Kernel "standard" library.
+ kern/libkern/kstring/ String functions.
+ kern/sched/           Thread scheduling.
+ kern/test/            Kernel space unit tests.
+ bin/                  Essential commands.
+ config/               Kernel build config.
+ include/              User space library headers.
+ lib/                  C runtime libraries and user space libraries.
+ modmakefiles/         Kernel module makefiles.
+ sbin/                 Essential system utilities.
+ test/                 User space unit tests.
+ test/punit/           User space unit test framework (PUnit).
+ tools/                Build tools and scripts.
+ usr/games/            Sources for games and demos.


Root Filesystem Hierarchy
-------------------------

+ /bin/                 Essential command binaries.
+ /dev/                 Virtual file system providing access to devices.
+ /lib/                 Essential libraries for /bin/ and /sbin/.
+ /mnt/                 Mount point for temporarily mounted filesystems.
+ /proc/                Virtual filesystem providing process information.
+ /sbin/                Essential system binaries.
+ /tmp/                 Temporary files.
+ /usr/games/           Games and demos.
+ kernel.img            The kernel image.


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

Historically both naming conventions have been used mixed case with underline
between module name and rest of the name, and everyting writen small case with
underlines. Third conventions was some ugly convention inherited from CMSIS,
which was somewhat hard to get rid of. All new source code must use the
following naming convention:

+ `module_feature_name`

### Function names

+ `module_comp_function` + module = name that also appears in filename
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

### Enums

Avoid using enums in kernel space, they are ugly, doesn't behave nicely and
doesn't add any additional protection. Usually enums even seems to generate
more code than using #defined values.

Enums are ok in user space and in interfaces between user space and kernel
space. Some standard things may even require using enums. Especially some
POSIX interfaces requires use of enums.


ABI and Calling Convention
--------------------------

Zeke uses mainly the default calling convention defined by GCC and Clang, which
is a bit different than the standard calling convention for ARM. Here is a brief
description of ABI and calling convention used in Zeke.

    +----------+------+-----------------------------------------------------+
    | Register | Alt. | Usage                                               |
    +----------+------+-----------------------------------------------------+
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
    +----------+------+-----------------------------------------------------+

Table partially based on:
http://en.wikipedia.org/wiki/Calling_convention
http://www.ethernut.de/en/documents/arm-inline-asm.html

If return value is a pointer or otherwise too large to be returned by pointer
stored in r0.

Stack is always full-descending.


Kernel Initialization 
---------------------

Kernel initialization order is defined as follows:

+ `hw_preinit`
+ `kmalloc_init()`
+ constructors
+ `hw_postinit`
+ `kinit()`

After kinit, scheduler will kick in and initialization continues in sinit(8)
process.

Every initializer function should contain `SUBSYS_INIT("XXXX init");` as a first
line of the function and optionally `SUBSYS_DEP(YYYY_init);` lines declaring
subsystem initialization dependencies.

### Kernel module/subsystem initializers

There are four kind of initializers supported at the moment:

+ *hw_preinit* for mainly hardware related initializers
+ *hw_postinit* for hardware related initializers
+ *constructor* (or init) for generic initialization

descturctors are not yet supported in Zeke but if there ever will be LKM
support the destructors will be called when unloading the module.

Following example shows constructor/intializer notation supported by Zeke:

    void mod_init(void) __attribute__((constructor));
    void mod_deinit(void) __attribute__((destructor));

    void mod_init(void)
    {
    SUBSYS_INIT();
    SUBSYS_DEP(modx_init); /* Module dependency */
    ...
    SUBSYS_INITFINI("mod OK");
    }

    void mod_deinit(void)
    { ... }

Constructor prioritizing is not supported and `SUBSYS_DEP` should be used to
tell dependencies between intializers.

### hw_preinit and hw_postinit

hw_preinit and hw_postinit can be used by including kinit.h header file and
using the following notation:

    void mod_preinit(void)
    {
    SUBSYS_INIT();
    ...
    SUBSYS_INITFINI("mod preinit OK");
    }
    HW_PREINIT_ENTRY(mod_preinit);

    void mod_postinit(void)
    { ... }
    HW_POSTINIT_ENTRY(mod_postinit);


Makefiles
---------

`kern/Makefile`is responsible of compiling the kernel and parsing kmod
makefiles. Module makefiles are following the naming convention `<module>.mk`
and are located under `kern/kmodmakefiles` directory.

Module makefiles are parsed like normal makefiles but care should be taken when
changing global variables in these makefiles. Module makefiles are mainly
allowed to only append IDIR variable and all other variables should be more or
less specific to the module makefile itself and should begin with the name of
the module.

An example of a module makefile (test.mk):

    # Test module
    # Mandatory file
    # If any source files are declared like this the whole module becomes
    # mandatory and won't be dropped even if its configuration is set to zero.
    test-SRC-1 += src/test/test.c
    # Optional file
    # If all files are declared like this then the module is optional and can be
    # enabled or disabled in the `config.mk`.
    test-SRC-$(configTEST_CONFIGURABLE) += src/test/source.c
    # Assembly file
    test-ASRC$(configTEST_CONFIGURABLE) += src/test/lowlevel.S

The kernel makefile will automatically discover `test-SRC-1` list and will
compile a new static library based on the compilation units in the list. Name of
the library is derived from the makefile's name and so should be the first word
of the source file list name.

### Userland Makefiles

Userland makefiles are constructed from `user_head.mk`, `user_tail.mk` and
the actual targets between includes. A good example of a user space Makefile
is `bin/Makefile` that compiles tools under `/bin`. A manifest file is
automatically generated by the make system and it will be used for creating
a rootfs image with `tools/mkrootfs.sh` script.
