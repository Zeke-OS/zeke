# Base system
base-SRC-1 += $(wildcard src/*.c)
base-SRC-$(configSCHED_TINY) += $(wildcard src/sched_tiny/*.c)
base-SRC-$(configPROCESSSCHED) += $(wildcard src/process/*.c)
# Kernel logging
base-SRC-$(configKERROR_LAST) += $(wildcard src/kerror_lastlog/*.c)
#dev subsystem
base-SRC-$(configDEVSUBSYS) += src/dev/dev.c
# TODO thscope should be moved??
base-SRC-1 += $(wildcard src/thscope/*.c)
