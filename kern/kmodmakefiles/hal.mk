# Common to all platforms
hal-SRC-1 += hal/sysinfo.c
hal-SRC-1 += hal/hw_timers.c
hal-SRC-$(configATAG) += hal/atag.c
hal-SRC-$(configMMU) += hal/mmu.c
hal-SRC-$(configUART) += hal/uart.c
hal-SRC-$(configFB) += $(wildcard hal/fb/*.c)

# Target model specific modules
ifeq ($(configMCU_MODEL),MCU_MODEL_BCM2835)
	MEMMAP = memmap_bcm2835.ld
	STARTUP = hal/arm11/arm11_startup.S
	hal-ASRC-1 += $(wildcard hal/bcm2835/*.S)
	hal-SRC-1 += hal/bcm2835/bcm2835_mmio.c
	hal-SRC-1 += hal/bcm2835/bcm2835_gpio.c
	hal-SRC-1 += hal/bcm2835/bcm2835_timers.c
	hal-SRC-$(configBCM_MB) += hal/bcm2835/bcm2835_mailbox.c
	hal-SRC-$(configUART) += hal/bcm2835/bcm2835_uart.c
	hal-SRC-$(configFB) += hal/bcm2835/bcm2835_fb.c
	hal-SRC-$(configBCM_JTAG) += hal/bcm2835/bcm2835_jtag.c
	#should have rpi flag
	hal-SRC-$(configRPI_LEDS) += hal/rpi/rpi_leds.c
	hal-SRC-$(configRPI_EMMC) += hal/rpi/rpi_emmc.c
endif

# Arhitecture and Profile specific sources
ifeq ($(configARM_PROFILE_M),1)
	hal-SRC-1 += $(wildcard hal/cortex_m/*.c)
else
	# ARM11
	ifneq (,$(filter $(configARCH),__ARM6__ __ARM6K__)) # Logical OR
		AIDIR += hal/arm11/
		hal-ASRC-1 += $(filter-out $(STARTUP), $(wildcard hal/arm11/*.S))
		hal-SRC-1 += $(wildcard hal/arm11/*.c)
	endif
endif

# Checks #######################################################################
# Check that MEMMAP and STARTUP are defined
ifndef MEMMAP
    $(error Missing MEMMAP! Wrong configMCU_MODEL?)
endif
ifndef STARTUP
    $(error Missing STARTUP! Wrong configMCU_MODEL?)
endif
