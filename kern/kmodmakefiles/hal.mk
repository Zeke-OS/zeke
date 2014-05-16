# Common to all platforms
hal-SRC-1 += kern/hal/sysinfo.c
hal-SRC-$(configATAG) += kern/hal/atag.c
hal-SRC-$(configMMU) += kern/hal/mmu.c
hal-SRC-$(configUART) += kern/hal/uart.c

# Target model specific modules
ifeq ($(configMCU_MODEL),MCU_MODEL_BCM2835)
	# NOTE: We don't want to include STARTUP here!
	hal-ASRC-1 += $(wildcard kern/hal/bcm2835/*.S)
	hal-SRC-1 += $(wildcard kern/hal/bcm2835/*.c)
	#should have raspi flag
	hal-SRC-1 += $(wildcard kern/hal/raspi/*.c)
endif

# Arhitecture and Profile specific sources
ifeq ($(configARM_PROFILE_M),1)
	hal-SRC-1 += $(wildcard kern/hal/cortex_m/*.c)
else
	# ARM11
	ifneq (,$(filter $(configARCH),__ARM6__ __ARM6K__)) # Logical OR
		AIDIR += kern/hal/arm11/
		hal-ASRC-1 += $(filter-out $(STARTUP), $(wildcard kern/hal/arm11/*.S))
		hal-SRC-1 += $(wildcard kern/hal/arm11/*.c)
	endif
endif
