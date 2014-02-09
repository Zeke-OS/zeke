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

# Floating point hardware
ifeq ($(configHFP),__HFP_VFP__)
	ASFLAGS  += -mfpu=vfp
endif

# Target specific Startup code & CRT ###########################################
# Memmap & vector table is set per MCU/CPU model
ifeq ($(configMCU_MODEL),MCU_MODEL_BCM2835)
	MEMMAP = config/memmap_bcm2835.ld
	# We may wan't to move this line to some where else
	STARTUP = src/hal/arm11/arm11_startup.S
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
