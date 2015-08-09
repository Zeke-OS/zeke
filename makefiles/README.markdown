Makefiles
=========

Userland Makefiles
------------------

### Variables

- `BIN-y`: A list of binaries compiled by this Makefile.
- `<binary>-ASRC-y`: Assembly sources. <binary> should match with a name in
                     `BIN-y`list.
- `<binary>-SRC-y`: C sources.
- `<binary>-CCFLAGS`: CC flags for the binary.
- `<binary>-LDFLAGS`: Linker flags for the binary.
- `FILES` and `FILES-y`: Other files that should be included in the manifest and
                         thus copied to the rootfs image. These files are not
                         compiled but just copied.
- `CLEAN_FILES`: Files that should be removed when `make clean` is executed.
