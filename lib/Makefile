# Zeke - /lib Makefile
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

LIBS = crt1 libc

# AR flags #####################################################################

libc-ARFLAGS = $(wildcard crt0_o/*.o)

# Source Files #################################################################
crt1-SRC-y = \
$(wildcard crt1/*.c) \
$(wildcard crt1/arm/*.c)

# TODO arch specific selection
crt1-ASRC-y = \
$(wildcard crt1/arm/*.S)

libc-SRC-y = \
$(wildcard libc/_PDCLIB/*.c) \
$(wildcard libc/*.c) \
$(wildcard libc/basecodecs/*.c) \
$(wildcard libc/ctype/*.c) \
$(wildcard libc/fcntl/*.c) \
$(wildcard libc/gen/*.c) \
$(wildcard libc/inttypes/*.c) \
$(wildcard libc/locale/*.c) \
$(wildcard libc/malloc/*.c) \
$(wildcard libc/pthreads/*.c) \
$(wildcard libc/signal/*.c) \
$(wildcard libc/stat/*.c) \
$(wildcard libc/stdio/*.c) \
$(wildcard libc/stdlib/*.c) \
$(wildcard libc/string/*.c) \
$(wildcard libc/unistd/*.c) \
$(wildcard libc/uchar/*.c) \
$(wildcard libc/wchar/*.c) \
$(wildcard libc/wctype/*.c)
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
SRC-y += $(foreach var,$(LIBS), $($(var)-SRC-y))
ASRC-y += $(foreach var,$(LIBS), $($(var)-ASRC-y))

# Assembly Obj files
ASOBJS := $(patsubst %.S, %.o, $(ASRC-y))

# C Obj files
OBJS := $(patsubst %.c, %.o, $(SRC-y))

# a files for static libr
LIBSA = $(addsuffix .a, $(LIBS))


all: $(LIBSA)

$(ASOBJS): %.o: %.S $(AUTOCONF_H)
	@echo "AS $@"
	@$(UNIFDEFALL) $(IDIR) $*.S | $(ARMGNU)-as -am $(IDIR) -o $@

$(OBJS): %.o: %.c $(AUTOCONF_H)
	$(eval CUR_OPT := $*.opt.bc)
	$(eval CUR_OPT_S := $*.opt.s)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) $(IDIR) -c $*.c -o $*.bc
	@$(OPT) $(OFLAGS) $*.bc -o $(CUR_OPT)
	@$(LLC) $(LLCFLAGS) $(CUR_OPT) -o $(CUR_OPT_S)
	@$(ARMGNU)-as $(CUR_OPT_S) -o $@ $(ASFLAGS)

crt0_o: $(CRT)
	mkdir -p crt0_o
	cd crt0_o && \
		ar x $(CRT)

$(LIBSA): crt0_o $(OBJS) $(ASOBJS)
	$(eval CUR_LIB := $(basename $@))
	$(eval CUR_OBJS := $(patsubst %.S, %.o, $($(CUR_LIB)-ASRC-y)))
	$(eval CUR_OBJS += $(patsubst %.c, %.o, $($(CUR_LIB)-SRC-y)))
	$(eval CUR_ARFLAGS := $($(CUR_LIB)-ARFLAGS))
	@echo "AR $@"
	@ar rcs $@ $(CUR_OBJS) $(CUR_ARFLAGS)

clean:
	$(RM) $(ASOBJS) $(OBJS)
	rm -rf crt0_o
	$(RM) *.a
