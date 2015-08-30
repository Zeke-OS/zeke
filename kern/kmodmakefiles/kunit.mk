# KUnit unit test framework.
kunit-SRC-$(configKUNIT) += $(wildcard kunit/*.c)
kunit-SRC-$(configKUNIT) += $(wildcard test/*.c)
kunit-SRC-$(configKUNIT_BIO) += $(wildcard test/bio/*.c)
kunit-SRC-$(configKUNIT_FS) += $(wildcard test/fs/*.c)
kunit-SRC-$(configKUNIT_GENERIC) += $(wildcard test/generic/*.c)
kunit-SRC-$(configKUNIT_HAL) += $(wildcard test/hal/*.c)
kunit-SRC-$(configKUNIT_KSTRING) += $(wildcard test/kstring/*.c)
