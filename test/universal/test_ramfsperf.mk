# This is a per test module makefile

# Test source file
TEST_SRC += test_ramfsperf.c

# SRC files needed for this test
SRC-ramfsperf += ../../src/fs/ramfs/ramfs.c
SRC-ramfsperf += ../../src/kstring/strlenn.c
SRC-ramfsperf += ../../src/kstring/kstrdup.c
SRC-ramfsperf += ../../src/kmalloc.c
SRC-ramfsperf += ../../src/fs/inpool.c
SRC-ramfsperf += ../../src/fs/dehtable.c
