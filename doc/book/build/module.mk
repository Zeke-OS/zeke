# Module example
# Mandatory file
# If any source files are declared like this the whole module becomes
# mandatory and won't be dropped even if its configuration is set to zero.
module-SRC-1 += src/module/test.c
# Optional file
# If all files are declared like this then the module is optional and can be
# enabled or disabled in the `config.mk`.
module-SRC-$(configMODULE_CONFIGURABLE) += src/module/source.c
# Assembly file
module-ASRC$(configMODULE_CONFIGURABLE) += src/module/lowlevel.S
