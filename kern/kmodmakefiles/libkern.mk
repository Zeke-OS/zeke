#libkern
libkern-SRC-y += $(wildcard libkern/*.c)
# kstring
libkern-SRC-y += $(wildcard libkern/kstring/*.c)
# gen min/max seg tree
libkern-SRC-$(configGENSEGTREE) += libkern/segtree/segtree.c
#subr_hash
libkern-SRC-$(configSUBR_HASH) += $(wildcard libkern/subr_hash/*.c)
