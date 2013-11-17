Zeke Coding Standards & Generic Documentation
=============================================

Here is some misc documentation and generic guidelines on how to code for Zeke.

Directory Structure
-------------------

+ src/          Most of kernel code.
+ src/generic   Generic data structures.
+ src/kstring   String functions.
+ src/hal       Harware abstraction layer.
+ src/fs        Virtual file system and other file systems.
+ src/kerror_X  Kernel logger implementations.
+ src/sched_X   Thread scheduler implementations.
+ src/usrscope  User scope libary source files.
+ include/      User scope library headers.
+ lib/          C runtime libraries.
+ config/       Build config.
+ modmakefiles/ Per module makefiles.
+ test/         Unit tests.


Naming Conventions
------------------

### File names

+ `module.c|h`  Any module that implements some functionality
+ `kmodule.c|h` Kernel scope source module that provides some external
                sycallable functionality
+ `lowlevel.S`  Assembly source code; Note that capital S for user files as file
                names ending with small s are reserved for compilation time files
+ `dev/`        Dev subsys modules
+ `thscope/`    Functions that are called and excecuted in thread scope;
                This means mainly syscall wrappers

*Warning:* Assembly sources with small suffix (.s) might be removed by
`make clean` as those are considered as automatically generated files.

### Global variables

There should not be need for any global variables but since you'll be still
declaring global variable please atleast use descriptive names.

There has been both naming conventions used mixed case with underline between
module name and rest of the name and everyting writen small with underlines.
Third conventions is some very ugly that was inherited from CMSIS which we are
now trying to get rid of.

Following names are somewhat acceptable:

+ `module_feature_name`

*Note:* `module_featureName` is obsoleted.

### Function names

+ `module_comp_function` + module = name that also appears in filename
                        + comp   = component/functionality eg. thread
                                   components will change thread status

*Note:* `module_compFunction` is obsoleted.


Standard Data Types
-------------------

### Typedefs

Typedefs are used in zeke for most of the structs and for some portability
related things where we may want or have to change types for example between
platforms.

### Enums

Avoid using enums in kernel space, they are ugly and don't behave nicely.
Usually enums even seems to generate more code than using #defined values.

Enums might be ok in user space and in interfaces between user space and
kernel space. Some standard things may even require using enums.


Kernel Initialization 
----------------------

Kernel initialization order is defined as follows:

For Cortex-M:
+ `SystemInit` - the clock system intitialization and other mandatory hw inits
+ `__libc_init_array` - Static constructors
+ `kinit` - Kernel initialization and load user code

For ARM11:
+ `hw_preinit`
+ `constructors`
+ `hw_postinit`
+ `kinit`

After kinit scheduler will kick in and initialization continues in user space.

### Kernel module initializers

There is four kind of initializers supported at the moment:

+ *hw_preinit* for mainly hardware related initializers
+ *hw_postinit* for hardware related initializers
+ *constructor* (or init) for generic initialization

Optional kernel modules may use C static constructors and destructors to
generate init and fini code that will be run before any user code is loaded.
Constructors are also the preferred method to attach module with the kernel.

Currently as there is no support for module unloading any fini functions will
not be called in any case. Still this doesn't generate any compilation errors
and array of fini functions is still generated and maybe supported later.

Following example shows constructor/intializer notation supported by Zeke:

    void begin(void) __attribute__((constructor));
    void end(void) __attribute__((destructor));

    void begin(void)
    { ... }

    void end(void)
    { ... }

Constructor prioritizing is not supported for constructor pointers but however
linker uses SORT, which may or may not work as expected.

### hw_preinit and hw_postinit

hw_preinit and hw_postinit can be used by including kinit.h header file and
using following notation:

    HW_PREINIT_ENTRY(init1);
    HW_POSTINIT_ENTRY(init2);


Makefiles
---------

The main Makefile is responsible of parsing module makefiles and compiling the
whole kernel. Module makefiles are named as `<module>.mk` and are located in the
`modmakefiles` directory under the root.

Note that in context of Zeke there is two kinds of modules, the core/top level
modules that are packed as static library files and then there is kind of
submodules (often referred as modules too) that are optional compilation units
or compilation unit groups. Both are configured in the kernel configuration.

Disabling some submodules may disable the whole module while some are only a
part of bigged top modules. So if for example `configDEVSUBSYS` is set to zero
then only devsubsys related functionality/copilation units are dropped from the
base system, but if `configDEVNULL` is set to zero then the whole module is
dropped from the compilation.

Module makefiles are parsed as normal makefiles but care should be taken when
changing global variables in makefiles. Module makefiles are mainly allowed to
only apped IDIR variable and all other variables should be more or less specific
to the module itself and should begin with the name of the module.

Example of a module makefile (test.mk):

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

Main makefile will automatically discover `test-SRC-1` list and will compile a
new static library based on the compilation units in the list. Name of the
library is derived from the makefile's name and so should be the first word of
the source file list name.

### Target specific compilation options and special files

As we don't want to put anything target specific and possibly changing data to
main makefile we are using a special makefile called `target.mak`. This special
makefile won't be noticed by the module makefile search but it's included
separately. `target.mak` doesn't need to be changed if Zeke is compiled to a
different platform but it has to be updated if support for a new platform is to
be added.

`target.mak` should define at least following target specific variables:
+ `ASFLAGS`: Containing CPU architecture flags
+ `MEMMAP`: Specifying linker script for kernel image memory map
+ `STARTUP`: Specifying target specific startup assembly source code file
+ `CRT`: Specifying CRT library used with the target
and optionally:
+ `LLCFLAGS`: containing some target specific flags
