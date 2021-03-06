# Base system
base-SRC-y += $(wildcard ./*.c)
base-SRC-y += $(wildcard sched/*.c)

# Kernel logging
base-SRC-$(configKLOGGER) += kerror/kerror.c
base-SRC-$(configKLOGGER) += kerror/kerror_buf.c
base-SRC-$(configKERROR_UART) += kerror/kerror_uart.c
base-SRC-$(configKERROR_FB) += kerror/kerror_fb.c
base-SRC-$(configDYNDEBUG) += kerror/dyndebug.c
base-SRC-$(configCORE_DUMPS) += $(wildcard coredump/*.c)
