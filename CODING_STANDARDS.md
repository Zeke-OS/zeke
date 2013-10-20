Zeke Coding Standards & Generic Documentation
=============================================

Here is some misc documentation and generic guidelines on how to code for Zeke.


Naming Conventions
------------------

### File names

+ module.c|h    Any module that implements some functionality
+ kmodule.c|h   Kernel scope source module that provides some external
                sycallable functionality
+ dev/          Dev subsys modules
+ thscope/      Functions that are called and excecuted in thread scope;
                This means mainly syscall wrappers

### Global variables

There should not be need for any global variables but since you'll be still
declaring global variable please atleast use descriptive names.

There has been both naming conventions used mixed case with underline between
module name and rest of the name and everyting writen small with underlines.
Third conventions is some very ugly that was inherited from CMSIS which
we are now trying to get rid of.

Following names are somewhat acceptable:

+ module_featureName
+ module_feature_name

### Function names

+ module_compFunction   + module = name that also appears in filename
                        + comp   = component/functionality eg. thread
                                   components will change thread status
+ module_comp_function


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
kernel space.


Kernel Initialization 
----------------------

Kernel initialization order is defined as follows:

For Cortex-M:
+ SystemInit - the clock system intitialization and other mandatory hw inits
+ __libc_init_array - Static constructors
+ kinit - Kernel initialization and load user code

For ARM11:
+ hw_preinit
+ constructors
+ hw_postinit
+ kinit

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
linker uses SORT.

### hw_preinit and hw_postinit

hw_preinit and hw_postinit can be used by including kinit.h header file and
using following notation:

    SECT_HW_POSTINIT(init1);
    SECT_HW_POSTINIT(init2);


Makefiles
---------

The main Makefile is responsible of parsing module makefiles and compiling the
whole kernel. Module makefiles are named as `<module>.mk` in the root directory.

Note that in context of Zeke there is two kinds of modules, the core/top
level modules that are packed as static library files and then there is
kind of submodules (often referred as modules too) that are optional
compilation units or compilation unit groups. Latter are configured
in the kernel configuration file where as the preceding can't be completely
disabled (at least yet). However if all submodules of the top module are
disabled in the configuration file then the static library file of the top
module will be eventually empty.

Module makefiles are parsed as normal makefiles but care should be taken when
changing global variables. Mainly module makefiles are allowed to only apped
IDIR variable and all other variables should be more or less specific to the
module itself.

Example of a module makefile (test.mk):
    # Test module
    # Mandatory file
    test-SRC-1 += src/test/test.c
    test-SRC-$(configTEST_CONFIGURABLE) += src/test/source.c

Main makefile will automatically find `test-SRC-1` list and compile a new static
library containing all compilation units specified in the list. Name of the
library is from the makefile's name and so should be the first word of the source
file listing.

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
