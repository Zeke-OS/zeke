# This is a per test module makefile

# Test source file
TEST_SRC += test_testable.c

# Infile src dir variable 
sd-testable = ../testable

# SRC files needed for this test
SRC-testable += $(sd-testable)/testable.c
