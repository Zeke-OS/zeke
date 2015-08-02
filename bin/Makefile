# Zeke - /bin Makefile
#
# Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
BIN-y := basename cat chgrp chmod chown cksum cmp cp dirname du echo env \
	hostname id kill login logname ls mount sh stty umount uptime wc xargs
BIN-$(configMINISED) += minised

SBASELUTIL := src/sbase/libutil
SBASELUTF  := src/sbase/libutf

# Source Files #################################################################
basename-SRC-y := src/sbase/basename.c $(SBASELUTIL)/eprintf.c \
	$(SBASELUTIL)/fshut.c
cat-SRC-y := src/cat.c
chgrp-SRC-y := src/sbase/chgrp.c $(SBASELUTIL)/eprintf.c \
	$(SBASELUTIL)/recurse.c $(SBASELUTIL)/ealloc.c $(SBASELUTIL)/strlcat.c \
	$(SBASELUTIL)/strlcpy.c
chmod-SRC-y := src/sbase/chmod.c \
	$(SBASELUTIL)/eprintf.c $(SBASELUTIL)/mode.c $(SBASELUTIL)/recurse.c \
	$(SBASELUTIL)/ealloc.c $(SBASELUTIL)/strlcat.c $(SBASELUTIL)/strlcpy.c
chown-SRC-y := src/sbase/chown.c \
	$(SBASELUTIL)/eprintf.c $(SBASELUTIL)/mode.c $(SBASELUTIL)/recurse.c \
	$(SBASELUTIL)/ealloc.c $(SBASELUTIL)/strlcat.c $(SBASELUTIL)/strlcpy.c \
	$(SBASELUTIL)/strtonum.c
cksum-SRC-y := src/sbase/cksum.c $(SBASELUTIL)/eprintf.c $(SBASELUTIL)/fshut.c
cmp-SRC-y := src/sbase/cmp.c $(SBASELUTIL)/eprintf.c $(SBASELUTIL)/fshut.c
cp-SRC-y := src/cp.c
dirname-SRC-y := src/sbase/dirname.c $(SBASELUTIL)/eprintf.c \
	$(SBASELUTIL)/fshut.c
du-SRC-y := src/sbase/du.c $(SBASELUTIL)/eprintf.c $(SBASELUTIL)/fshut.c \
	$(SBASELUTIL)/strtonum.c $(SBASELUTIL)/recurse.c $(SBASELUTIL)/strlcat.c \
	$(SBASELUTIL)/strlcpy.c $(SBASELUTIL)/ealloc.c $(SBASELUTIL)/human.c
echo-SRC-y := src/echo.c
env-SRC-y := src/env.c
hostname-SRC-y := src/sbase/hostname.c $(SBASELUTIL)/eprintf.c \
	$(SBASELUTIL)/fshut.c
id-SRC-y := src/id.c
kill-SRC-y := src/sbase/kill.c $(SBASELUTIL)/eprintf.c $(SBASELUTIL)/fshut.c \
	$(SBASELUTIL)/strtonum.c
login-SRC-y := src/login.c
logname-SRC-y := src/sbase/logname.c $(SBASELUTIL)/eprintf.c \
	$(SBASELUTIL)/fshut.c
ls-SRC-y := src/ls.c
minised-SRC-$(configMINISED) := src/minised/sedcomp.c src/minised/sedexec.c
mount-SRC-y := src/mount.c src/utils/opt.c
sh-SRC-y := $(wildcard src/sh/*.c) src/utils/gline.c
stty-SRC-y := src/stty.c
umount-SRC-y := src/umount.c src/utils/opt.c
uptime-SRC-y := src/uptime.c
wc-SRC-y := src/wc.c
#xargs-SRC-y := src/xargs.c
xargs-SRC-y := src/sbase/xargs.c $(SBASELUTIL)/eprintf.c $(SBASELUTIL)/fshut.c \
	$(SBASELUTIL)/strtonum.c $(SBASELUTIL)/strlcat.c $(SBASELUTIL)/strlcpy.c \
	$(SBASELUTIL)/ealloc.c

# Other files ##################################################################
FILES := whoami

# CCFLAGS ######################################################################

# LDFLAGS ######################################################################
sh-LDFLAGS := -llinenoise
login-LDFLAGS := -lcrypt

# End

include $(ROOT_DIR)/makefiles/user_tail.mk
