# Common to all platforms
hal-SRC-y += hal/sysinfo.c
hal-SRC-y += hal/hw_timers.c
hal-SRC-$(configATAG) += hal/atag.c
hal-SRC-$(configMMU) += hal/mmu.c
hal-SRC-$(configUART) += hal/uart.c
hal-SRC-$(configFB) += $(wildcard hal/fb/*.c)

# Target model specific modules
ifdef configBCM2835
	MEMMAP = memmap_bcm2835.ld
	STARTUP = hal/arm11/arm11_startup.S
	hal-ASRC-y += $(wildcard hal/bcm2835/*.S)
	hal-SRC-y += hal/bcm2835/bcm2835_mmio.c
	hal-SRC-y += hal/bcm2835/bcm2835_gpio.c
	hal-SRC-y += hal/bcm2835/bcm2835_timers.c
	hal-SRC-$(configBCM_MB) += hal/bcm2835/bcm2835_mailbox.c
	hal-SRC-$(configUART) += hal/bcm2835/bcm2835_uart.c
	hal-SRC-$(configBCM_FB) += hal/bcm2835/bcm2835_fb.c
	hal-SRC-$(configBCM_JTAG) += hal/bcm2835/bcm2835_jtag.c
	#should have rpi flag
	hal-SRC-$(configRPI_LEDS) += hal/rpi/rpi_leds.c
	hal-SRC-$(configRPI_EMMC) += hal/rpi/rpi_emmc.c
	hal-SRC-$(configRPI_HW)   += hal/rpi/rpi_hw.c
endif

# Arhitecture and Profile specific sources
ifeq ($(configARM_PROFILE_M),y)
	hal-SRC-y += $(wildcard hal/cortex_m/*.c)
else
	# ARM11
	ifneq "$(strip $(__ARM6__), $(__ARM6K__))" ""
		AIDIR += hal/arm11/
		hal-ASRC-y += $(filter-out $(STARTUP), $(wildcard hal/arm11/*.S))
		hal-SRC-y += $(wildcard hal/arm11/*.c)
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
