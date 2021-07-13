Build System
============

Prerequisites
-------------

To successfully compile Zeke, you'll need the following packages/tools:

**Zeke**

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

**Documentation**

- Doxygen
- `man` for viewing man pages

Using Docker for Build
----------------------

The easiest way to build and run Zeke is by using the ready built
[Docker image](https://hub.docker.com/r/olliv/zekedock/) and
[dmk](https://github.com/Zeke-OS/dmk).

The Docker image is built by using the this [Dockerfile](/docker/Dockerfile),
see [Zekedoc documentation](/docker/README.md) for more information.

When in the root directory of the Zeke repository:

```bash
$ dmk
you@abc:/build$ KBUILD_DEFCONFIG=qemu_rpi_headless_testing.defconfig ./configure defconfig
you@abc:/build$ make all -j5
you@abc:/build$ make qemu
```

After making the defconfig you can change the configuration knobs by running
`./configure menuconfig`. More information regarding `configure` script can be
found [here](https://github.com/Zeke-OS/zeke/wiki/configure-script).

The userland unit tests must be built by specifying a special target `opttest`
as `all` doesn't include tests. The tests can be found under `opt/test` when
running Zeke, which means the tests must be run inside Zeke as thebinaries are
compiled for Zeke using its libc.

The unit tests for the kernel (kunit) can be executed by writing the name of
each test to `/proc/kunit` and available tests can be listed by reading the
same file.

The Docker image comes with a GCC version specially built for debugging the
Zeke kernel. [Debugging with GDB](/doc/gdb.md) has some documentation about
how to debug Zeke.


Building
--------

### Build commands

+ `configure`- Set defaults for files in config dir
+ `make all rootfs` - Compile the kernel, user space and create a rootfs image
+ `make kernel.img` - Compile the kernel
+ `make world` - Compile only the user space without creating a rootfs image
+ `make opttest` - Compile on target tests
+ `make clean` - Clean all compilation targets

Run `make help` for full list of targets and compilation flags.


### Arch Specific Instructions

- [Running Zeke on ARM](/doc/arch/arm.md)
- [Running Zeke in QEMU](/doc/arch/qemu.md)
- [Running Zeke on MIPS](/doc/arch/mips.md)


### Build Config Tool

Configure script accepts following commands as an argument:

- `config`          Update current config utilising a line-oriented program
- `nconfig`         Update current config utilising a ncurses menu based program
- `menuconfig`      Update current config utilising a menu based program
- `xconfig`         Update current config utilising a QT based front-end
- `gconfig`         Update current config utilising a GTK based front-end
- `oldconfig`       Update current config utilising a provided .config as base
- `localmodconfig`  Update current config disabling modules not loaded
- `localyesconfig`  Update current config converting local mods to core
- `silentoldconfig` Same as oldconfig, but quietly, additionally update deps
- `defconfig`       New config with default from ARCH supplied defconfig
- `savedefconfig`   Save current config as ./defconfig (minimal config)
- `allnoconfig`     New config where all options are answered with no
- `allyesconfig`    New config where all options are accepted with yes
- `allmodconfig`    New config selecting modules when possible
- `alldefconfig`    New config with all symbols set to default
- `randconfig`      New config with random answer to all options
- `listnewconfig`   List new options
- `olddefconfig`    Same as silentoldconfig but sets new symbols to their default value

To use defconfig you must set `KBUILD_DEFCONFIG` environment variable to any of the
filenames seen under `defconfigs` directory.

For example:

```
$ KBUILD_DEFCONFIG=qemu_rpi_headless_testing.defconfig ./configure defconfig
```


How Everything is Compiled?
---------------------------

### Kernel Build

`kern/Makefile` is responsible of compiling the kernel and parsing kmod
makefiles. Module makefiles are following the naming convention
`<module>.mk` and are located under `kern/kmodmakefiles` directory.

Module makefiles are parsed like normal makefiles but care should be
taken when changing global variables in these makefiles. Module
makefiles are mainly allowed to only append `IDIR` variable and all
other variables should be more or less specific to the module makefile
itself and should begin with the name of the module.

A valid module makefile could look something like the following
listing:

```makefile
# Module example
# Mandatory file
# If any source files are declared like this the whole module becomes
# mandatory and won't be dropped even if its configuration is set to zero.
module-SRC-1 += src/module/test.c
# Optional file
# If all files are declared like this then the module is optional and can be
# enabled or disabled in the `config.mk`.
module-SRC-$(configMODULE_CONFIGURABLE) += src/module/source.c
# Assembly file
module-ASRC$(configMODULE_CONFIGURABLE) += src/module/lowlevel.S
```

The kernel makefile will automatically discover `test-SRC-1` list and
will compile a new static library based on the compilation units in the
list. Name of the library is derived from the makefile’s name and so
should be the first word of the source file list name.

### Libraries

TODO

### Applications aka Userland Build

Userland makefiles are constructed from `user_head.mk`, `user_tail.mk`
and the actual targets between includes. A good example of a user space
makefile is `bin/Makefile` that compiles tools under `/bin`. A manifest
file is automatically generated by the make system and it will be used
for creating a rootfs image with `tools/mkrootfs.sh` script.

#### Creating a new user space program

Userland program sources are mostly located under directories close to
the final location in rootfs image. Currently we only have `bin` and
`sbin` directories in the build but our intention is to follow FHS as
far as possible. So for example sources for init is located in
`/sbin/src/sinit` directory and the final elf binary is created to
`/sbin`. However if the final executable is compiled from one file
it's ok to omit the final level of the directory structure in source tree.

The build system for user programs is "fully" automated, meaning that
there shouldn't be need to write any new makefiles to build new binaries.

**Adding a new program to the build**

1. Assuming that you have your source code files in the correct directory
   go to the top directory of the build tree eg. `/bin` and open `Makefile`
   with your favorite editor.

2. Add the final binary name to the list of name in `BIN` variable.

3. Create a list of source file as a variable named `BINFILENAME-SRC-y`,
   where `y` can be replaced with `$(configSOMETHING)` if compilation of
   the binary should be controlled by a Kconfig option.

Libc is linked by default and any other needed libraries or linker arguments
can be passed by setting `BINFILENAME-LDFLAGS` in the makefile.

### Applications

TODO

### Tools Build

TODO
