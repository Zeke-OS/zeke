# Copyright (c) 2012, 2013, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met: 
#
#1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Include kernel config
include ./config/config.mk

# Configuration files ##########################################################
CONFIG_MK  = ./config/config.mk
AUTOCONF_H = ./config/autoconf.h

# Memmap & vector table is set per mcu/cpu model
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	MEMMAP = config/memmap_stm32f051x8
	VECTABLE = src/vectable.s
endif
# TODO rpi
################################################################################
VECTABLE_O = $(patsubst %.s, %.o, $(VECTABLE))

# Generic Compiler Options #####################################################
ARMGNU   = arm-none-eabi
CCFLAGS  = -std=c99 -emit-llvm -Wall -ffreestanding -O2
CCFLAGS += -nostdlib -nostdinc
CCFLAGS += -m32 -ccc-host-triple $(ARMGNU)
OFLAGS   = -std-compile-opts
LLCFLAGS = -mtriple=$(ARMGNU)
ASFLAGS  =#


# Target Specific Compiler Options #############################################
# Arch specific flags
ifeq ($(configARCH),__ARM6M__)
	LLCFLAGS += -march=thumb
	ASFLAGS  += -mcpu=cortex-m0 -mthumb -EL
	# or more generic with
	#ASFLAGS  march=armv6-m -mthumb -EL
endif
ifeq ($(configARCH),__ARM6__)
	LLCFLAGS += -march=thumb
	ASFLAGS  += -march=armv6 -mthumb -EL
endif

# MCU specific flags
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	CCFLAGS  += -DUSE_STDPERIPH_DRIVER -DSTM32F0XX
endif

# Floating point hardware
ifeq ($(configHFP),__HFP_VFP__)
	ASFLAGS  += -mfpu=vfp
endif
################################################################################

# Dirs #########################################################################
IDIR = ./include ./config ./src

# Target specific libraries
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	IDIR += ./Libraries/CMSIS/Include ./Libraries/STM32F0xx/Drivers/inc
	IDIR += ./Libraries/STM32F0xx/CMSIS
	IDIR += ./Libraries/STM32F0xx/Drivers/inc
	# TODO not always Discovery
	IDIR += ./Libraries/Discovery
endif
################################################################################
IDIR := $(patsubst %,-I%,$(subst :, ,$(IDIR)))

# Source modules ###############################################################
SRC-  =# Init
SRC-0 =# Init
SRC-1 =# Init
# SRC- and SRC-0 meaning module will not be compiled

# Base system
SRC-1 += $(wildcard src/*.c)
SRC-1 += $(wildcard src/string/*.c)
SRC-$(configSCHED_TINY) += $(wildcard src/sched_tiny/*.c)
# TODO thscope should be moved??
SRC-1 += $(wildcard src/thscope/*.c)

# Target specific modules
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	SRC-1 += $(wildcard Libraries/STM32F0xx/CMSIS/*.c)
	#SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_dbgmcu.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_exti.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_gpio.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_misc.c
	#SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_pwr.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_rcc.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_syscfg.c
	#SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_tim.c
	# TODO not always Discovery
	SRC-1 += $(wildcard Libraries/Discovery/*.c)
	SRC-1 += src/hal/stm32f0_interrupt.c
endif
# Select HAL
ifeq ($(configARM_PROFILE_M),1)
	SRC-1 += src/hal/cortex_m.c
#else
#	ifeq ($(configARCH),__ARM4T__) # ARM9
#	endif
endif

# Dev subsystem
SRC-$(configDEVSUBSYS) += $(wildcard src/dev/*.c)

# PTTK91 VM
# TODO should be built separately
################################################################################
# Obj files
BCS  := $(patsubst %.c, %.bc, $(SRC-1))
OBJS := $(patsubst %.c, %.o, $(SRC-1))

# Target specific CRT ##########################################################
CRT =# Init
ifeq ($(configARM_PROFILE_M),1)
	# TODO which one is the best choice
	# libaeabi-cortexm0
	CRT := Libraries/crt/libaebi-cortexm0/libaeabi-cortexm0.a
	# libgcc for ARMv6-M
	#ifeq ($(configARCH),__ARM6M__)
	#CRT := Libraries/crt/armv6-m-libgcc/libgcc.a
	#endif
else
	ifeq ($(configARCH),__ARM6__)
		 CRT := Libraries/crt/rpi-libgcc/libgcc.a
	endif
endif
################################################################################

# We use suffixes because those are funny
.SUFFIXES:					# Delete the default suffixes
.SUFFIXES: .c .bc .o .h		# Define our suffix list


# target: all - Make config and compile kernel
all: config kernel

# target: config - Update configuration from $(CONFIG_MK)
config: $(AUTOCONF_H)

# target: test - Run alla universal unit tests
test:
	cd test\universal \
	make

$(AUTOCONF_H): $(CONFIG_MK)
	./tools/aconf.sh $(CONFIG_MK) $(AUTOCONF_H)

$(CONFIG_MK):
# End of config

kernel: kernel.bin

$(VECTABLE_O): $(VECTABLE)
	$(ARMGNU)-as $< -o $(VECTABLE_O)

$(BCS): $(SRC-1)
	clang $(CCFLAGS) $(IDIR) -c $*.c -o $@

$(OBJS): $(BCS)
	$(eval CUR_OPT := $*.opt.bc)
	$(eval CUR_OPT_S := $*.opt.s)
	opt-3.0 $(OFLAGS) $*.bc -o $(CUR_OPT)
	llc-3.0 $(LLCFLAGS) $(CUR_OPT) -o $(CUR_OPT_S)
	$(ARMGNU)-as $(CUR_OPT_S) -o $@ $(ASFLAGS)

#--sysroot=/usr/lib/gcc/arm-none-eabi/4.7.4/armv6-m/
kernel.bin: $(MEMMAP) $(VECTABLE_O) $(OBJS) $(CRT)
	$(ARMGNU)-ld -o kernel.clang.thumb.opt.elf -T $^ $(CRT)
	$(ARMGNU)-objdump -D kernel.clang.thumb.opt.elf > kernel.clang.thumb.opt.list
	$(ARMGNU)-objcopy kernel.clang.thumb.opt.elf kernel.clang.thumb.opt.bin -O binary

# target: help - Display callable targets.
help:
	@egrep "^# target:" [Mm]akefile|sed 's/^# target: //'

.PHONY: config kernel clean

clean:
	rm -f $(AUTOCONF_H)
	rm -f $(OBJS) $(VECTABLE_O)
	rm -f $(BCS)
	find . -type f -name "*.bc" -exec rm -f {} \;
	find . -type f -name "*.opt.bc" -exec rm -f {} \;
	find . -type f -name "*.opt.s" -exec rm -f {} \;
	rm -f *.bin
	rm -f *.elf
	rm -f *.list
