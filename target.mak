# Target specigic make options & special files

# Target Specific Compiler Options #############################################
# Arch specific flags
ifeq ($(configARCH),__ARM6M__)
	LLCFLAGS += -march=thumb
	ASFLAGS  += -mcpu=cortex-m0 -mthumb -EL
	# or more generic with
	#ASFLAGS  march=armv6-m -mthumb -EL
endif
ifeq ($(configARCH),__ARM6__)
	LLCFLAGS += -march=arm
	ASFLAGS  += -march=armv6 -EL
endif
# TODO Enable thumb?
ifeq ($(configARCH),__ARM6K__)
	LLCFLAGS += -march=arm
	ASFLAGS  += -march=armv6k -EL
endif

# MCU specific flags
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	CCFLAGS  += -DUSE_STDPERIPH_DRIVER -DSTM32F0XX
endif

# Floating point hardware
ifeq ($(configHFP),__HFP_VFP__)
	ASFLAGS  += -mfpu=vfp
endif

# Target specific Startup code & CRT ###########################################
# Memmap & vector table is set per MCU/CPU model
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	MEMMAP = config/memmap_stm32f051x8.ld
	STARTUP = src/hal/stm32f0/startup_stm32f0xx.S
endif
ifeq ($(configMCU_MODEL),MCU_MODEL_BCM2835)
	MEMMAP = config/memmap_bcm2835.ld
	STARTUP = src/hal/bcm2835/startup_bcm2835.S
endif
# Check that MEMMAP and STARTUP are defined
ifndef MEMMAP
	$(error Missing MEMMAP! Wrong configMCU_MODEL?)
endif
ifndef STARTUP
	$(error Missing STARTUP! Wrong configMCU_MODEL?)
endif

# CRT is compiled per instruction set or per core model
ifeq ($(configARM_PROFILE_M),1)
	# libgcc for ARMv6-M
	ifeq ($(configARCH),__ARM6M__)
		CRT = lib/crt/libaeabi-armv6-m/libaeabi-armv6-m.a
	endif
else
	ifeq ($(configARCH),__ARM6K__)
		CRT = lib/crt/libaeabi-armv6k/libaeabi-armv6k.a
	endif
endif
