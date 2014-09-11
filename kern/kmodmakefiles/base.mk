# Base system
base-SRC-y += $(wildcard ./*.c)
base-SRC-$(configSCHED_TINY) += $(wildcard sched_tiny/*.c)
base-SRC-$(configSCHED_CDS) += $(wildcard sched_cds/*.c)

# Kernel logging
base-SRC-$(configKLOGGER) += kerror/kerror.c
base-SRC-$(configKLOGGER) += kerror/kerror_buf.c
base-SRC-$(configKERROR_UART) += kerror/kerror_uart.c
base-SRC-$(configKERROR_FB) += kerror/kerror_fb.c
