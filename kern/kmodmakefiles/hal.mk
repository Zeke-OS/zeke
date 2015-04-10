# Common to all platforms
hal-SRC-y += hal/sysinfo.c
hal-SRC-y += hal/hw_timers.c
hal-SRC-$(configATAG) += hal/atag.c
hal-SRC-$(configMMU) += hal/mmu.c
hal-SRC-$(configUART) += hal/uart.c
hal-SRC-$(configFB) += $(wildcard hal/fb/*.c)

# Target model specific modules
ifeq ($(configBCM2835),y)
	MEMMAP = memmap_bcm2835.ld
	STARTUP = hal/arm11/arm11_startup.S
	hal-ASRC-y += $(wildcard hal/bcm2835/*.S)
	hal-SRC-y += hal/bcm2835/bcm2835_mmio.c
	hal-SRC-y += hal/bcm2835/bcm2835_gpio.c
	hal-SRC-y += hal/bcm2835/bcm2835_timers.c
	hal-SRC-$(configBCM_MB) += hal/bcm2835/bcm2835_mailbox.c
	hal-SRC-$(configBCM_MB) += hal/bcm2835/bcm2835_prop.c
	hal-SRC-$(configUART) += hal/bcm2835/bcm2835_uart.c
	hal-SRC-$(configBCM_FB) += hal/bcm2835/bcm2835_fb.c
	hal-SRC-$(configBCM_JTAG) += hal/bcm2835/bcm2835_jtag.c
	hal-SRC-$(configBCM_PM) += hal/bcm2835/bcm2835_pm.c
	#should have rpi flag
	hal-SRC-$(configRPI_LEDS) += hal/rpi/rpi_leds.c
	hal-SRC-$(configRPI_EMMC) += hal/rpi/rpi_emmc.c
	hal-SRC-$(configRPI_HW)   += hal/rpi/rpi_hw.c
endif
ifeq ($(configJZ4780),y)
	MEMMAP = memmap_jz4780.ld
	STARTUP = hal/mips32/mips32_startup.S
endif

# Arhitecture and Profile specific sources
ifeq ($(configARCH_ARM),y)
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
endif

# Checks #######################################################################
# Check that MEMMAP and STARTUP are defined
ifndef MEMMAP
    $(error Missing kernel MEMMAP! Wrong configMCU_MODEL?)
endif
ifndef STARTUP
    $(error Missing kernel STARTUP! Wrong configMCU_MODEL?)
endif
