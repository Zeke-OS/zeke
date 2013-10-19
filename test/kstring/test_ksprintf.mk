# This is a per test module makefile

# Test source file
TEST_SRC += test_ksprintf.c

# SRC files needed for this test
SRC-ksprintf += ../../src/kstring/ksprintf.c
SRC-ksprintf += ../../src/kstring/strlenn.c
SRC-ksprintf += ../../src/kstring/strnncat.c
SRC-ksprintf += ../../src/kstring/uitoa32.c
SRC-ksprintf += ../../src/kstring/uitoah32.c
