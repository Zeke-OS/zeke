include $(ROOT_DIR)/genconfig/buildconf.mk

DIRS =#
ifeq ($(configTESTS_LIBC),y)
DIRS += libc
endif
ifeq ($(configTESTS_MMAN),y)
DIRS += mman
endif
ifeq ($(configTESTS_TTY),y)
DIRS += tty
endif
ifeq ($(configTESTS_PTHREADS),y)
DIRS += pthreads
endif
ifeq ($(configTESTS_UNISTD),y)
DIRS += unistd
endif

CLEANDIRS := $(patsubst %,%.clean,$(subst :, ,$(DIRS)))

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

$(CLEANDIRS):
	$(MAKE) -C $(basename $@) clean

clean: $(CLEANDIRS)
	find . -type f -name "*.bc" -exec rm -f {} \;
	find . -type f -name "*.d" -exec rm -f {} \;
	$(RM) libc/manifest
	$(RM) mman/manifest
	$(RM) tty/manifest
	$(RM) unistd/manifest

.PHONY: $(DIRS) $(CLEANDIRS) clean
