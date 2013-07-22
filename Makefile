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
# IAR Compatibility crap
CCFLAGS += -D__ARM_PROFILE_M__=1 -D__ARM6M__=5 -D__CORE__=5 # TODO remove these
CCFLAGS += -nostdlib -nostdinc #-nobuiltininc
CCFLAGS += -m32 -ccc-host-triple $(ARMGNU)
OFLAGS   = -std-compile-opts
LLCFLAGS = -march=thumb -mtriple=$(ARMGNU)
################################################################################

# Dirs #########################################################################
IDIR = ./include ./config ./src

# Target specific libraries
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	IDIR += ./Libraries/CMSIS/Include ./Libraries/STM32F0xx/CMSIS
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
# Dev subsystem
SRC-$(CONF_DEVSUBSYS) += $(wildcard src/dev/*.c)
# PTTK91 VM
# TODO Include dir modification
#SRC-$(CONF_PTTK91_VM) += $(wildcard pttk91/src/*.c)
################################################################################
# Obj files
BCS  := $(patsubst %.c, %.bc, $(SRC-1))
OBJS := $(patsubst %.c, %.o, $(SRC-1))

# TODO use MCU_MODEL as a target specifier

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
	$(ARMGNU)-as $(CUR_OPT_S) -o $@

kernel.bin: $(MEMMAP) $(VECTABLE_O) $(OBJS)
	$(ARMGNU)-ld -o kernel.clang.thumb.opt.elf -T $^
	$(ARMGNU)-objdump -D kernel.clang.thumb.opt.elf > kernel.clang.thumb.opt.list
	$(ARMGNU)-objcopy kernel.clang.thumb.opt.elf kernel.clang.thumb.opt.bin -O binary

# target: help - Display callable targets.
help:
	@egrep "^# target:" [Mm]akefile|sed 's/^# target: //'

.PHONY: config kernel clean

clean:
	#rm -f *.bin
	#rm -f *.o
	#rm -f *.elf
	#rm -f *.list
	#rm -f *.bc
	#rm -f *.opt.s
	rm -f $(AUTOCONF_H)
