# KUnit unit test framework.
base-SRC-$(configKUNIT) += $(wildcard kunit/*.c)
base-SRC-$(configKUNIT) += $(wildcard test/*.c)
base-SRC-$(configKUNIT_GENERIC) += $(wildcard test/generic/*.c)
base-SRC-$(configKUNIT_KSTRING) += $(wildcard test/kstring/*.c)
base-SRC-$(configKUNIT_FS) += $(wildcard test/fs/*.c)
base-SRC-$(configKUNIT_HAL) += $(wildcard test/hal/*.c)
