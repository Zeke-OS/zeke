Libraries
=========

All libraries that are linkable for the user space are compiled by
Makefiles under this directory. The following files are needed per
library:

    libX/Makefile (optional)    Generates any files needed for the actual
                                compilation.
    libX/lib.mk                 Declares compilation options, headers files, and
                                source files needed by the library.

lib.mk
------

### Compiler options
- `libX-ARFLAGS` is passed to ar.

### Sources
- `libX-ASRC-y` Assembly sources for the library.
-  `libX-SRC-y` C sources for the library.
