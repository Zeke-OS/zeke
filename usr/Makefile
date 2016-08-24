# Zeke - /usr Makefile
#
# Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

include $(ROOT_DIR)/makefiles/user_head.mk

# Binaries #####################################################################
BIN-$(configUSR_EXAMPLES) := \
examples/daemon \
examples/eztrie \
examples/hugestack \
examples/linenoise
BIN-$(configUSR_GAMES) := games/banner games/fbdemo games/plasma

# Source Files #################################################################
examples/daemon-SRC-$(configUSR_EXAMPLES) := examples/daemon.c
examples/eztrie-SRC-$(configUSR_EXAMPLES) := examples/eztrie.c
examples/hugestack-SRC-$(configUSR_EXAMPLES) := examples/hugestack.c
examples/linenoise-SRC-$(configUSR_EXAMPLES) := examples/linenoise.c
games/banner-SRC-$(configUSR_GAMES) := games/banner.c
games/fbdemo-SRC-$(configUSR_GAMES) := games/fbdemo-src/main.c \
	games/fbdemo-src/bitmap.c games/fbdemo-src/fonteng.c
games/plasma-SRC-$(configUSR_GAMES) := games/plasma.c

# Other files ##################################################################
FILES-$(configUSR_GAMES) := games/ball.bmp

# CCFLAGS ######################################################################

# LDFLAGS ######################################################################
examples/linenoise-LDFLAGS := -llinenoise
examples/eztrie-LDFLAGS := -leztrie

# End

include $(ROOT_DIR)/makefiles/user_tail.mk
