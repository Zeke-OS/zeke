# This is a per test module makefile

# Test source file
TEST_SRC += test_ksprintf.c

# SRC files needed for this test
SRC-ksprintf += ../../src/libkern/kstring/ksprintf.c
SRC-ksprintf += ../../src/libkern/kstring/strlenn.c
SRC-ksprintf += ../../src/libkern/kstring/strnncat.c
SRC-ksprintf += ../../src/libkern/kstring/uitoa32.c
SRC-ksprintf += ../../src/libkern/kstring/uitoah32.c
