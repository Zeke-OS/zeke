# Add userland include dirs to IDIR

-include $(ROOT_DIR)/lib/lib_idir.gen.mk

IDIR += $(patsubst %,-I$(ROOT_DIR)/lib/%,$(LIB_IDIR))
