include $(ROOT_DIR)/genconfig/buildconf.mk

DIRS =#
DIRS-$(configTESTS_FB) += fb
DIRS-$(configTESTS_LIBC) += libc
DIRS-$(configTESTS_MMAN) += mman
DIRS-$(configTESTS_TTY) += tty
DIRS-$(configTESTS_PTHREADS) += pthreads
DIRS-$(configTESTS_UNISTD) += unistd

CLEANDIRS := $(patsubst %,%.clean,$(subst :, ,$(DIRS-y)))

all: $(DIRS-y) run.sh manifest

$(DIRS-y):
	$(MAKE) -C $@

run.sh: $(DIRS-y)
	@echo '#!/bin/sh' > run.sh
	@for dir in $(DIRS-y); \
		do for ex in `cat $$dir/manifest`; \
			do echo "$$dir/$$ex" >> run.sh; \
		done; \
	done

manifest:
	@echo "run.sh" > manifest

$(CLEANDIRS):
	$(MAKE) -C $(basename $@) clean

clean: $(CLEANDIRS)
	find . -type f -name "*.bc" -exec rm -f {} \;
	find . -type f -name "*.d" -exec rm -f {} \;
	$(RM) fb/manifest
	$(RM) libc/manifest
	$(RM) mman/manifest
	$(RM) tty/manifest
	$(RM) pthreads/manifest
	$(RM) unistd/manifest
	$(RM) manifest

.PHONY: $(DIRS-y) $(CLEANDIRS) clean
