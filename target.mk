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
	CCFLAGS  += -target armv6k-none-eabi
	LLCFLAGS += -march=arm
	ASFLAGS  += -march=armv6k -EL
endif

# Floating point hardware
ifeq ($(configHFP),__HFP_VFP__)
	ASFLAGS  += -mfpu=vfp
endif
CCFLAGS += -m32

# Target specific CRT ##########################################################
# CRT is compiled per instruction set or per core model
ifeq ($(configARM_PROFILE_M),1)
	ifeq ($(configARCH),__ARM6M__)
		CRT = $(ROOT_DIR)/lib/crt/libaeabi-armv6-m/libaeabi-armv6-m.a
	endif
else
	ifeq ($(configARCH),__ARM6K__)
		CRT = $(ROOT_DIR)/lib/crt/libaeabi-armv6k/libaeabi-armv6k.a
	endif
endif
CRT_DIR = $(dir $(CRT))

# Checks #######################################################################
ifndef ASFLAGS
    $(error Missing ASFLAGS! Wrong configARCH? "$(configARCH)")
endif

# Check that CRT is defined
ifndef CRT
    $(error Missing CRT! Wrong configMCU_MODEL or configARCH? "$(configARCH)")
endif

