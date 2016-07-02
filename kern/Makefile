# Zeke - Kernel Makefile
#
# Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
# Copyright (c) 2012, 2013 Olli Vanhoja <olli.vanhoja@ninjaware.fi>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met: 
#
# 1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Configuration files ##########################################################
# Generic config
include $(ROOT_DIR)/genconfig/buildconf.mk
include $(ROOT_DIR)/makefiles/commands.mk
################################################################################
# We use suffixes because it's fun
.SUFFIXES:                      # Delete the default suffixes
.SUFFIXES: .c .bc .o .h .S ._S  # Define our suffix list

# Makefiles for modules ########################################################
# Atm for sake of simplicity we keep all makefiles on single level
MODMKFILES = $(wildcard ./kmodmakefiles/*.mk)
include $(MODMKFILES)
# Following will generate a list of module names
ALLMODULES = $(basename $(notdir $(MODMKFILES)))
# Remove modules that do not contain any compilation units
MODULES += $(foreach var, $(ALLMODULES), $(if $(strip $($(var)-SRC-y) $($(var)-ASRC-y)), $(var),))
################################################################################

# Compiler Options #############################################################
#CC         = SET IN BUILDCONF.MK
CCFLAGS    += -emit-llvm -ffreestanding -nostdlib -nostdinc \
$(subst $\",,$(configKERNEL_CCFLAGS))
CCFLAGS    += -include autoconf.h -DKERNEL_INTERNAL
#OPT        = SET IN BUILDCONF.MK
OFLAGS     += -std-compile-opts
#LLC        = SET IN BUILDCONF.MK
LLCFLAGS   += -mtriple=$(GNUARCH)
ASFLAGS    +=#
LDFLAGS    +=#
################################################################################

# Include Dirs #################################################################
IDIR   = ./include ../include ../genconfig
AIDIR += ../genconfig
################################################################################
IDIR  := $(patsubst %,-I%,$(subst :, ,$(IDIR)))
AIDIR := $(patsubst %,-I%,$(subst :, ,$(AIDIR)))

# Select & Include Modules #####################################################
# Available selections for source code files:
SRC-	=# C sources
SRC-n  	=#
SRC-y  	=#
ASRC-  	=# Assembly sources
ASRC-n 	=#
ASRC-y	=#
# (A)SRC- and (A)SRC-n meaning that submodule won't be compiled

# Include sources from top modules
SRC-y += $(foreach var,$(MODULES), $($(var)-SRC-y))
ASRC-y += $(foreach var,$(MODULES), $($(var)-ASRC-y))
################################################################################
# Parse file names

# Assembly Obj files
ASOBJS 	:= $(patsubst %.S, %.o, $(ASRC-y))

# C Obj files
OBJS 	:= $(patsubst %.c, %.o, $(SRC-y))

STARTUP_O := $(patsubst %.S, %.o, $(STARTUP))

# Deps
DEPS := $(patsubst %.c, %.d, $(SRC-y))

# a files for modules
MODAS := $(addsuffix .a, $(MODULES))

KERNEL_IMG := $(ROOT_DIR)/kernel.img

# Targets ######################################################################

all: kernel

# Kernel
kernel: $(KERNEL_IMG)

$(STARTUP_O): $(STARTUP) $(AUTOCONF_H)
	@echo "AS $@"
	@$(CC) -E $(AIDIR) $< | grep -Ev "^#" | $(GNUARCH)-as -o $(STARTUP_O)

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	$(as-command)

$(OBJS): %.o: %.c $(AUTOCONF_H)
	$(cc-command)
ifeq ($(ANAL),y)
	$(anal-command)
endif

-include $(DEPS)

$(MODAS): $(ASOBJS) $(OBJS)
	@echo "AR $@"
	$(eval CUR_MODULE := $(basename $@))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_MODULE)-SRC-y)))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_MODULE)-ASRC-y)))
	@ar rcs $@ $(CUR_OBJS)

$(KERNEL_IMG): $(MEMMAP) $(STARTUP_O) $(MODAS)
	$(GNUARCH)-ld -o $(ROOT_DIR)/kernel.elf -T $(MEMMAP) $(LDFLAGS) \
		$(STARTUP_O) --whole-archive $(MODAS) --no-whole-archive $(CRT)
	$(GNUARCH)-objcopy -O binary $(ROOT_DIR)/kernel.elf $(KERNEL_IMG)
# End of Kernel

$(ROOT_DIR)/kernel.list: $(KERNEL_IMG)
	$(GNUARCH)-objdump -D $(ROOT_DIR)/kernel.elf > $(ROOT_DIR)/kernel.list

.PHONY: kernel clean

clean:
	$(RM) $(ASOBJS) $(OBJS) $(DEPS) $(STARTUP_O)
	rm -rf anal
	$(RM) *.bin
	$(RM) *.img
	$(RM) *.elf
	$(RM) *.list
	$(RM) *.a
