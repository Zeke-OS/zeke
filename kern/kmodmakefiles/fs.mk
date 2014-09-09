# File systems
base-SRC-1 += $(wildcard fs/*.c)
# VFS hash
base-SRC-$(configVFS_HASH) += $(wildcard fs/vfs_hash/*.c)

# devfs
base-SRC-1 += $(wildcard fs/devfs/*.c)
# mbr
base-SRC-$(configMBR) += $(wildcard fs/mbr/*.c)
# ramfs
base-SRC-$(configRAMFS) += $(wildcard fs/ramfs/*.c)

# msdosfs
base-SRC-$(configMSDOSFS) += $(wildcard fs/msdosfs/*.c)
