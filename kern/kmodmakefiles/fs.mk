# File systems
base-SRC-1 += $(wildcard fs/*.c)

# devfs
base-SRC-1 += $(wildcard fs/devfs/*.c)
# mbr
base-SRC-$(configMBR) += $(wildcard fs/mbr/*.c)
# ramfs
base-SRC-$(configRAMFS) += $(wildcard fs/ramfs/*.c)