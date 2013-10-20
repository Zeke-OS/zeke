# Base system
base-SRC-1 += $(wildcard src/*.c)
base-SRC-$(configSCHED_TINY) += $(wildcard src/sched_tiny/*.c)
# Kernel logging
base-SRC-$(configKERROR_LAST) += $(wildcard  src/kerror_last/*.c)
# TODO thscope should be moved??
base-SRC-1 += $(wildcard src/thscope/*.c)
