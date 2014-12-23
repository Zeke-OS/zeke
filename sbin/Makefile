# Zeke - /sbin Makefile
#
# Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

# Configuration files ##########################################################
# Generic config
include $(ROOT_DIR)/genconfig/buildconf.mk
include $(ROOT_DIR)/genconfig/dist.mk
################################################################################
# We use suffixes because it's fun
.SUFFIXES:                  	# Delete the default suffixes
.SUFFIXES: .c .bc .o .h .S ._S	# Define our suffix list

SBIN = init

# Source Files #################################################################
init-SRC-y = $(wildcard src/init/*.c) $(wildcard src/init/tish/*.c)
################################################################################
# Available selections for source code files:
SRC-	=# C sources
SRC-n	=#
SRC-y	=#
ASRC-	=# Assembly sources
ASRC-n	=#
ASRC-y	=#
# (A)SRC- and (A)SRC-n meaning that submodule won't be compiled

# All sources
SRC-y += $(foreach var,$(SBIN), $($(var)-SRC-y))
ASRC-y += $(foreach var,$(SBIN), $($(var)-ASRC-y))

# Assembly Obj files
ASOBJS := $(patsubst %.S, %.o, $(ASRC-y))

# C Obj files
BCS := $(patsubst %.c, %.bc, $(SRC-y))
OBJS := $(patsubst %.c, %.o, $(SRC-y))

all: $(SBIN)
	echo "$(SBIN)" > manifest

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	@echo "AS $@"
	@$(UNIFDEFALL) $(IDIR) $*.S | $(ARMGNU)-as -am $(IDIR) -o $@

$(OBJS): $(SRC-y) $(AUTOCONF_H)
	$(eval CUR_BC := $*.bc)
	$(eval CUR_OPT := $*.opt.bc)
	$(eval CUR_OPT_S := $*.opt.s)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) $(IDIR) -c $*.c -o $(CUR_BC)
	@$(OPT) $(OFLAGS) $*.bc -o $(CUR_OPT)
	@$(LLC) $(LLCFLAGS) $(CUR_OPT) -o $(CUR_OPT_S)
	@$(ARMGNU)-as $(CUR_OPT_S) -o $@ $(ASFLAGS)

$(SBIN): $(OBJS)
	$(eval CUR_BIN := $(basename $@))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_BIN)-ASRC-y)))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_BIN)-SRC-y)))
	@echo "LD $@"
	$(ARMGNU)-ld -o $@ -T $(ROOT_DIR)/$(ELFLD) $(LDFLAGS) $(ROOT_DIR)/lib/crt1.a $(LDIR) $(CUR_OBJS) -lc

clean:
	$(RM) $(ASOBJS) $(OBJS) $(SBIN)
	$(RM) manifest
