# Base system
base-SRC-1 += $(wildcard kern/*.c)
base-SRC-$(configSCHED_TINY) += $(wildcard kern/sched_tiny/*.c)

# Generic Data Structures
base-SRC-1 += $(wildcard kern/generic/*.c)

# Kernel logging
base-SRC-$(configKERROR_LAST) += $(wildcard kern/kerror_lastlog/*.c)
base-SRC-$(configKERROR_TTYS) += $(wildcard kern/kerror_ttys/*.c)

# Virtual file system
base-SRC-1 += $(wildcard kern/fs/*.c)
# devfs
#base-SRC-1 += kern/fs/dev/devfs.c
# ramfs
base-SRC-$(configRAMFS) += kern/fs/ramfs/*.c

