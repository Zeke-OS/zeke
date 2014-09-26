#libkern
libkern-SRC-y += $(wildcard libkern/*.c)
# kstring
libkern-SRC-y += $(wildcard libkern/kstring/*.c)
#subr_hash
libkern-SRC-$(configSUBR_HASH) += $(wildcard libkern/subr_hash/*.c)
