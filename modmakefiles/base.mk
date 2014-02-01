# Base system
base-SRC-1 += $(wildcard src/*.c)
base-SRC-$(configSCHED_TINY) += $(wildcard src/sched_tiny/*.c)

# Generic Data Structures
base-SRC-1 += $(wildcard src/generic/*.c)

# Kernel logging
base-SRC-$(configKERROR_LAST) += $(wildcard src/kerror_lastlog/*.c)
base-SRC-$(configKERROR_TTYS) += $(wildcard src/kerror_ttys/*.c)

# Virtual file system
base-SRC-1 += $(wildcard src/fs/*.c)
# devfs
#base-SRC-1 += src/fs/dev/devfs.c
# ramfs
base-SRC-$(configRAMFS) += src/fs/ramfs/*.c

