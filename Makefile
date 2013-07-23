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
MEMMAP = config/memmap_stm32f051x8
VECTABLE = src/vectable.s
################################################################################
VECTABLE_O = $(patsubst %.s, %.o, $(VECTABLE))

# Compiler options #############################################################
ARMGNU   = arm-none-eabi
CCFLAGS  = -std=c99 -emit-llvm -Wall -ffreestanding -O2
CCFLAGS += -nostdlib -nostdinc
CCFLAGS += -m32 -ccc-host-triple $(ARMGNU)
OFLAGS   = -std-compile-opts
LLCFLAGS = -march=thumb -mtriple=$(ARMGNU)
ASFLAGS  = -mcpu=cortex-m0 -mthumb -EL
################################################################################

# Dirs #########################################################################
IDIR = ./include ./config ./src

# Target specific libraries
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	IDIR += ./Libraries/CMSIS/Include ./Libraries/STM32F0xx/Drivers/inc
	IDIR += ./Libraries/STM32F0xx/CMSIS/
	IDIR += ./Libraries/STM32F0xx/Drivers/inc
	#IDIR += ./Libraries/Discovery # TODO not always Discovery
endif
################################################################################
IDIR := $(patsubst %,-I%,$(subst :, ,$(IDIR)))

# Source moduleus ##############################################################
SRC-1 :=# Init
SRC-0 :=# Init
SRC-  :=# Init
# SRC-0 and SRC - meaning module will not be compiled

# Base system
SRC-1 += $(wildcard src/*.c)
SRC-$(configSCHED_TINY) += $(wildcard src/sched_tiny/*.c)
#SRC-1 += $(wildcard src/thscope/*.c) # TODO thscope should be moved??
SRC-1 += src/thscope/kernel.c
# Target specific modules
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	SRC-1 += $(wildcard Libraries/STM32F0xx/CMSIS/*.c)
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_cec.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_dbgmcu.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_exti.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_gpio.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_misc.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_pwr.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_rcc.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_syscfg.c
	SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_tim.c
	#SRC-1 += $(wildcard Libraries/Discovery/*.c)
	SRC-1 += src/hal/cortex_m.c # TODO This is actually more generic
endif
# HAL
ifeq ($(configARM_PROFILE_M),1)
	SRC-1 += src/hal/cortex_m.c
#else
#	ifeq ($(configCORE),__ARM4T__) # ARM9
#	endif
endif
# Dev subsystem
SRC-$(CONF_DEVSUBSYS) += $(wildcard src/dev/*.c)
# PTTK91 VM
# TODO Include dir modification
#SRC-$(CONF_PTTK91_VM) += $(wildcard pttk91/src/*.c)
################################################################################
# Obj files
BCS  := $(patsubst %.c, %.bc, $(SRC-1))
OBJS := $(patsubst %.c, %.o, $(SRC-1))

# Target specific CRT ##########################################################
CRT =# Init
ifeq ($(configARM_PROFILE_M),1)
	CRT := Libraries/crt/libaebi-cortexm0/libaeabi-cortexm0.a
else
	ifeq ($(configCORE),__ARM6__)
		 CRT := Libraries/crt/rpi-libgcc/libgcc.a
	endif
endif
################################################################################

.SUFFIXES:					# Delete the default suffixes
.SUFFIXES: .c .bc .o .h		# Define our suffix list


# target: all - Make config and compile kernel
all: config kernel

# target: config - Update configuration from $(CONFIG_MK)
config: $(AUTOCONF_H)

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

kernel.bin: $(MEMMAP) $(VECTABLE_O) $(OBJS) $(CRT)
	$(ARMGNU)-ld -o kernel.clang.thumb.opt.elf -T $^
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
