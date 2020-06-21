# File systems
fs-SRC-y += $(wildcard fs/*.c)
# inode pools
fs-SRC-$(configFS_INPOOL) += fs/libfs/inpool.c
# dehtable
fs-SRC-$(configFS_DEHTABLE) += fs/libfs/dehtable.c
# VFS hash
fs-SRC-$(configVFS_HASH) += fs/libfs/vfs_hash.c
# FS queue
fs-SRC-y += fs/libfs/fs_queue.c

# mbr
fs-SRC-$(configMBR) += $(wildcard fs/mbr/*.c)

# ramfs
fs-SRC-$(configRAMFS) += $(wildcard fs/ramfs/*.c)

# devfs
fs-SRC-$(configDEVFS) += $(wildcard fs/devfs/*.c)
# procfs
fs-SRC-$(configPROCFS) += $(wildcard fs/procfs/*.c)


# fatfs
fs-SRC-$(configFATFS) += $(wildcard fs/fatfs/*.c)
fs-SRC-$(configFATFS) += $(wildcard fs/fatfs/src/*.c)
fs-SRC-$(configFATFS) += $(wildcard fs/fatfs/src/utf/*.c)
