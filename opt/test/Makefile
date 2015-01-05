DIRS := libc mman
CLEANDIRS := $(patsubst %,%.clean,$(subst :, ,$(DIRS)))

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

$(CLEANDIRS):
	$(MAKE) -C $(basename $@) clean

clean: $(CLEANDIRS)
	find . -type f -name "*.s" -exec rm -f {} \;
	find . -type f -name "*.d" -exec rm -f {} \;

.PHONY: $(DIRS) $(CLEANDIRS) clean
