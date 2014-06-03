# Base system
base-SRC-1 += $(wildcard kern/*.c)
base-SRC-$(configSCHED_TINY) += $(wildcard kern/sched_tiny/*.c)

# Generic Data Structures
base-SRC-1 += $(wildcard kern/generic/*.c)

# Kernel logging
base-SRC-$(configKLOGGER) += kern/kerror/kerror.c
base-SRC-$(configKLOGGER) += kern/kerror/kerror_buf.c
base-SRC-$(configKERROR_UART) += kern/kerror/kerror_uart.c
base-SRC-$(configKERROR_FB) += kern/kerror/kerror_fb.c

# Virtual file system
base-SRC-1 += $(wildcard kern/fs/*.c)
# devfs
#base-SRC-1 += kern/fs/dev/devfs.c
# ramfs
base-SRC-$(configRAMFS) += kern/fs/ramfs/*.c

