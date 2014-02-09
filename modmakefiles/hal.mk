# Common to all platforms
hal-SRC-1 += src/hal/sysinfo.c
hal-SRC-$(configATAG) += src/hal/atag.c
hal-SRC-$(configMMU) += src/hal/mmu.c
hal-SRC-$(configUART) += src/hal/uart.c

# Target model specific modules
ifeq ($(configMCU_MODEL),MCU_MODEL_BCM2835)
	# NOTE: We don't want to include STARTUP here!
	hal-ASRC-1 += $(wildcard src/hal/bcm2835/*.S)
	hal-SRC-1 += $(wildcard src/hal/bcm2835/*.c)
endif

# Arhitecture and Profile specific sources
ifeq ($(configARM_PROFILE_M),1)
	hal-SRC-1 += $(wildcard src/hal/cortex_m/*.c)
else
	# ARM11
	ifneq (,$(filter $(configARCH),__ARM6__ __ARM6K__)) # Logical OR
		AIDIR += src/hal/arm11/
		hal-ASRC-1 += $(filter-out $(STARTUP), $(wildcard src/hal/arm11/*.S))
		hal-SRC-1 += $(wildcard src/hal/arm11/*.c)
	endif
endif
