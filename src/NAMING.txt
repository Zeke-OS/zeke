Naming Conventions
==================

File names
----------
+ module.c|h    Any module that implements some functionality
+ kmodule.c|h   Kernel scope source module that provides some external
                sycallable functionality
+ dev/          Dev subsys modules
+ thscope/      Functions that are called and excecuted in thread scope;
                This means mainly syscall wrappers

Naming of functions
-------------------
+ module_compFunction   + module = name that also appears in filename
                        + comp   = component/functionality eg. thread
                                   components will change thread status
