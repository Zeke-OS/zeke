# Zeke - Main Makefile
#
# Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
# Copyright (c) 2012, 2013, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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
################################################################################

# Targets ######################################################################
# Help lines
# '# target: '		Generic target
# '# target_comp'	Generic target
# '# target_test'	Testing target
# '# target_clean'	Cleaning target
# '# target_doc'	Documentation target

# target_comp: all - Make config and compile kernel.
all: tools kernel.img world
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
tools: $(UNIFDEF) tools/exec_qemu
#

$(UNIFDEF):
	$(MAKE) -C tools/unifdef unifdef

tools/exec_qemu: tools/exec_qemu.c
	$(CC) tools/exec_qemu.c -o tools/exec_qemu
# End of Tools

$(AUTOCONF_H): config
	$(ROOT_DIR)/tools/aconf.sh $^ $@
	cd include && rm machine; ln -s sys/mach/$(MACHIDIR) machine

# target_comp: kernel.img - Compile kernel image.
kernel.img: | tools $(AUTOCONF_H) lib
	$(MAKE) -C kern all

# target_comp: world - Compile user space stuff.
world: lib bin etc sbin usr

bin: lib
	$(MAKE) -C bin all

etc:
	$(MAKE) -C etc all

lib: $(AUTOCONF_H)
	$(MAKE) -C lib prepare
	$(MAKE) -C lib all

sbin: lib
	$(MAKE) -C sbin all

usr: lib
	$(MAKE) -C usr all

# target_test: opttest - Target platform tests.
opttest: lib
	$(MAKE) -C opt/test all

# target_comp: rootfs - Create an rootfs image.
rootfs: all
	./tools/mkrootfs.sh zeke-rootfs.img boot/rpi.files

# target_doc: stats - Calculate some stats.
stats: clean
	cloc --exclude-dir=.git,doc,tools .

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
	$(MAKE) -C tools/unifdef clean

# target_clean: clean-doc - Clean documenation targets.
clean-doc:
	$(MAKE) -C doc/book clean

# target_clean: clean-man - Clean man pages.
clean-man:
	rm -rf doc/man/*
