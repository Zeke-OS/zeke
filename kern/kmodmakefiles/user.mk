# User space code
# TODO Temporarily here, in the future we may not want to compile this with the
# kernel.
user-SRC-1 += $(wildcard lib/libc/*.c)
user-SRC-1 += $(wildcard sbin/init/*.c)
