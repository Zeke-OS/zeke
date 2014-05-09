# KUnit unit test framework.
user-SRC-$(configKUNIT) += $(wildcard src/kunit/*.c)
user-SRC-$(configKUNIT) += $(wildcard src/test/*.c)
user-SRC-$(configKUNIT) += $(wildcard src/test/generic/*.c)
user-SRC-$(configKUNIT) += $(wildcard src/test/kstring/*.c)
user-SRC-$(configKUNIT) += $(wildcard src/test/fs/*.c)
