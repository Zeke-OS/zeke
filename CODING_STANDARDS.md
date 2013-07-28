Zeke Coding Standards & Generic Documentation
=============================================

Naming Conventions
------------------

### File names

+ module.c|h    Any module that implements some functionality
+ kmodule.c|h   Kernel scope source module that provides some external
                sycallable functionality
+ dev/          Dev subsys modules
+ thscope/      Functions that are called and excecuted in thread scope;
                This means mainly syscall wrappers

### Function names

+ module_compFunction   + module = name that also appears in filename
                        + comp   = component/functionality eg. thread
                                   components will change thread status


Kernel Initialization 
----------------------

Kernel initialization order is defined as follows:

+ SystemInit - the clock system intitialization and other mandatory hw inits
+ __libc_init_array - Static constructors
+ kinit - Kernel initialization and load user code

### Kernel module initializers

Optional kernel modules may use C static constructors and destructors to
generate init and fini code that will be run before any user code is loaded.
Currently as there is no support for module unloading any fini functions will
not be called in any case. Still this doesn't generate any compilation errors
and array of fini functions is still generated and maybe supported later.

Following example shows notation is supported by Zeke:
void begin(void) __attribute__((constructor));
void end(void) __attribute__((destructor));

void begin(void)
{ ... }

void end(void)
{ ... }

