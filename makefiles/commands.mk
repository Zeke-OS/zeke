# Common build commands

# AS
# AIDIR := Assembler include dirs in -I format
# ASFLAGS := Assembler flags
define as-command
	@echo "AS $@"
	@$(CC) -E $(AIDIR) $*.S | grep -Ev "^#" | \
		$(GNUARCH)-as $(ASFLAGS) -am $(AIDIR) -o $@
endef

# CC
# CCFLAGS := Common cflags
# NAME-CCFLAGS := Per c file cflags
# IDIR := Include dirs in -I format
# OFLAGS := Optimizer flags
# LLCFLAGS := Linker flags
# ASFLAGS := Assembler flags
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

define anal-command
	@echo "anal $@"
	$(eval NAME := $(basename $(notdir $@)))
	$(eval CUR_XML := $*.xml)
	@$(CC) --analyze $(CCFLAGS) $($(NAME)-CCFLAGS) $(IDIR) -c $*.c -o $(CUR_XML)
endef
