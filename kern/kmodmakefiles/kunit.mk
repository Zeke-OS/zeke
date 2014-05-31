# KUnit unit test framework.
user-SRC-$(configKUNIT) += $(wildcard kern/kunit/*.c)
user-SRC-$(configKUNIT) += $(wildcard kern/test/*.c)
user-SRC-$(configKUNIT_GENERIC) += $(wildcard kern/test/generic/*.c)
user-SRC-$(configKUNIT_KSTRING) += $(wildcard kern/test/kstring/*.c)
user-SRC-$(configKUNIT_FS) += $(wildcard kern/test/fs/*.c)
