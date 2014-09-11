# User space code
# TODO Temporarily here, in the future we may not want to compile this with the
# kernel.
user-SRC-y += $(wildcard $(ROOT_DIR)/lib/libc/*.c)
user-SRC-y += $(wildcard $(ROOT_DIR)/lib/libc/stdlib/*.c)
user-SRC-y += $(wildcard $(ROOT_DIR)/sbin/init/*.c)
user-SRC-$(configTISH) += $(wildcard $(ROOT_DIR)/sbin/init/tish/*.c)
