# Zeke - Target specigic make options & special files
#
# Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

