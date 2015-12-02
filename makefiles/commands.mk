# Common user space build commands

define as-command
	@echo "AS $@"
	@$(CC) -E $(IDIR) $*.S | grep -Ev "^#" | \
		$(GNUARCH)-as $(ASFLAGS) -am $(IDIR) -o $@
endef

define cc-command
	@echo "CC $@"
	$(eval NAME := $(basename $(notdir $@)))
	$(eval CUR_BC := $*.bc)
	$(eval SP := s|\(^.*\)\.bc|$@|)
	@$(CC) $(CCFLAGS) $($(NAME)-CCFLAGS) $(IDIR) -c $*.c -o $(CUR_BC)
	@sed -i "$(SP)" "$*.d"
	@$(OPT) $(OFLAGS) $(CUR_BC) -o - | $(LLC) $(LLCFLAGS) - -o - | \
		$(GNUARCH)-as - -o $@ $(ASFLAGS)
endef
