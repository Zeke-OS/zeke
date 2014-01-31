# This is a per test module makefile

# Test source file
TEST_SRC += test_ramfs.c

# SRC files needed for this test
SRC-ramfs += ../../src/fs/ramfs/ramfs.c
SRC-ramfs += ../../src/libkern/kstring/strlenn.c
SRC-ramfs += ../../src/libkern/kstring/kstrdup.c
SRC-ramfs += ../../src/kmalloc.c
SRC-ramfs += ../../src/fs/inpool.c
SRC-ramfs += ../../src/fs/dehtable.c
