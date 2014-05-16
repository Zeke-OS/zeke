# KUnit unit test framework.
user-SRC-$(configKUNIT) += $(wildcard kern/kunit/*.c)
user-SRC-$(configKUNIT) += $(wildcard kern/test/*.c)
user-SRC-$(configKUNIT) += $(wildcard kern/test/generic/*.c)
user-SRC-$(configKUNIT) += $(wildcard kern/test/kstring/*.c)
user-SRC-$(configKUNIT) += $(wildcard kern/test/fs/*.c)
