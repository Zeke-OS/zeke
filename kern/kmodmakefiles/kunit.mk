# KUnit unit test framework.
user-SRC-$(configKUNIT) += $(wildcard kunit/*.c)
user-SRC-$(configKUNIT) += $(wildcard test/*.c)
user-SRC-$(configKUNIT_GENERIC) += $(wildcard test/generic/*.c)
user-SRC-$(configKUNIT_KSTRING) += $(wildcard test/kstring/*.c)
user-SRC-$(configKUNIT_FS) += $(wildcard test/fs/*.c)
