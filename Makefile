# Zeke - Main Makefile
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
export ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include $(ROOT_DIR)/genconfig/buildconf.mk
include $(ROOT_DIR)/boot/boot.mk
################################################################################

# Generate a list of all libraries
ALL_LIBS := $(patsubst lib/%/,%,$(shell ls -d lib/*/))

# Targets ######################################################################
# Help lines
# '# target: '		Generic target
# '# target_comp'	Generic target
# '# target_test'	Testing target
# '# target_clean'	Cleaning target
# '# target_doc'	Documentation target

# target_comp: all - Make config and compile kernel.
all: kernel.img world
.PHONY: $(DIRS_TARGETS) kernel.img world \
	bin sbin etc lib tools \
	clean-all clean clean-doc clean-tools clean-man

# target_doc: doc - Compile all documentation.
doc: doc-book doc-man

# target_doc: doc-man - Generate man pages.
doc-man: clean-man
	doxygen Doxyfile.libc
	doxygen Doxyfile.kern

# target_doc: doc-book - Compile Zeke book.
doc-book:
	$(MAKE) -C doc/book all
# End of Doc


# target_comp: tools - Build tools.
tools: tools/exec_qemu
#

tools/exec_qemu: tools/exec_qemu.c
	$(CC) tools/exec_qemu.c -o tools/exec_qemu
# End of Tools

$(AUTOCONF_H): config | tools
	$(ROOT_DIR)/tools/aconf.sh $^ $@
	cd include && rm -f machine; ln -s sys/mach/$(MACHIDIR) machine

# target_comp: kernel.img - Compile kernel image.
kernel.img: | $(AUTOCONF_H) crt0
	$(MAKE) -C kern all

# target_comp: world - Compile user space stuff.
world: lib bin etc sbin usr

bin: lib
	$(MAKE) -C bin all

etc:
	$(MAKE) -C etc all

# For compiling invidual libraries
$(ALL_LIBS): $(AUTOCONF_H)
	$(if $(filter $@,libc),$(eval export LIBS := crt0 libc),$(eval export LIBS := $@))
	@$(MAKE) -C lib
	@echo MAKE $@

lib: $(ALL_LIBS)
	$(eval export LIBS := $(ALL_LIBS))
	$(MAKE) -C lib gen_idir_mkfile

sbin: lib
	$(MAKE) -C sbin all

usr: lib
	$(MAKE) -C usr all

# target_test: opttest - Target platform tests.
opttest: lib
	$(MAKE) -C opt/test all

# target_comp: rootfs - Create an rootfs image.
rootfs: all
	./tools/mkrootfs.sh zeke-rootfs.img $(MKROOTFS_BOOTFILES)

# target: qemu - Run Zeke in QEMU.
qemu: rootfs
	qemu-system-arm -kernel kernel.elf -sd zeke-rootfs.img \
		-cpu arm1176 -m 256 -M raspi -nographic -serial stdio \
		-monitor telnet::4444,server,nowait -d unimp,guest_errors -s

# target_doc: stats - Calculate some stats.
stats: clean
	cloc --exclude-dir=.git,doc,tools .

cppcheck-kern:
	cppcheck --rule-file=tools/cppcheck_zeke.cfg --force ./kern

cppcheck-lib:
	cppcheck --rule-file=tools/cppcheck_zeke.cfg --force ./lib

# target: help - Display callable targets.
help:
	@./tools/help.sh

# target_clean: clean-all - Clean all targets.
clean-all: clean clean-tools clean-doc

# target_clean: clean - Clean.
clean:
	$(RM) $(AUTOCONF_H)
	$(MAKE) -C bin clean
	$(MAKE) -C etc clean
	$(MAKE) -C kern clean
	$(MAKE) -C lib clean
	$(MAKE) -C opt/test clean
	$(MAKE) -C sbin clean
	$(MAKE) -C usr clean
	find . -type f -name "*.o" -exec rm -f {} \;
	find . -type f -name "*.bc" -exec rm -f {} \;
	$(RM) *.bin
	$(RM) *.img
	$(RM) *.elf
	$(RM) *.list
	$(RM) *.a
	$(RM) zeke-rootfs.img
	$(RM) include/machine

# target_clean: clean-tools - Clean all tools.
clean-tools:
	$(RM) tools/exec_qemu

# target_clean: clean-doc - Clean documenation targets.
clean-doc:
	$(MAKE) -C doc/book clean

# target_clean: clean-man - Clean man pages.
clean-man:
	$(RM) doc/man/*
