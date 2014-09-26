# File systems
base-SRC-y += $(wildcard fs/*.c)
# inode pools
base-SRC-$(configFS_INPOOL) += fs/libfs/inpool.c
# dehtable
base-SRC-$(configFS_DEHTABLE) += fs/libfs/dehtable.c
# VFS hash
base-SRC-$(configVFS_HASH) += fs/libfs/vfs_hash.c

# mbr
base-SRC-$(configMBR) += $(wildcard fs/mbr/*.c)

# ramfs
base-SRC-$(configRAMFS) += $(wildcard fs/ramfs/*.c)

# devfs
base-SRC-$(configDEVFS) += $(wildcard fs/devfs/*.c)
# procfs
base-SRC-$(configPROCFS) += $(wildcard fs/procfs/*.c)


# fatfs
base-SRC-$(configFATFS) += $(wildcard fs/fatfs/*.c)
base-SRC-$(configFATFS) += $(wildcard fs/fatfs/src/*.c)
