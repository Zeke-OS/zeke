# Sort and remove duplicates
SRC-y := $(sort $(SRC-y))

# Obj files
OBJS := $(patsubst %.c, %.o, $(SRC-y))
DEPS := $(patsubst %.c, %.d, $(SRC-y))

all: $(BIN) manifest

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	@echo "AS $@"
	@$(CC) -E $(IDIR) $*.S | grep -Ev "^#" | $(GNUARCH)-as -am $(IDIR) -o $@

$(OBJS): %.o: %.c $(AUTOCONF_H)
	@echo "CC $@"
	$(eval NAME := $(basename $(notdir $@)))
	$(eval CUR_BC := $*.bc)
	$(eval SP := s|\(^.*\)\.bc|$@|)
	@$(CC) $(CCFLAGS) $($(NAME)-CCFLAGS) $(IDIR) -c $*.c -o $(CUR_BC)
	@sed -i "$(SP)" "$*.d"
	@$(OPT) $(OFLAGS) $(CUR_BC) -o - | $(LLC) $(LLCFLAGS) - -o - | \
		$(GNUARCH)-as - -o $@ $(ASFLAGS)

$(BIN): $(OBJS)
	@echo "LD $@"
	$(eval CUR_BIN := $(basename $@))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_BIN)-ASRC-y)))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_BIN)-SRC-y)))
	@$(GNUARCH)-ld -o $@ -T $(ROOT_DIR)/$(ELFLD) $(LDFLAGS) \
		$(ROOT_DIR)/lib/crt1.a $(LDIR) $(CUR_OBJS) $($(CUR_BIN)-LDFLAGS) -lc

-include $(DEPS)

manifest: $(BIN)
	echo "$(BIN)" > manifest

clean:
	$(RM) $(ASOBJS) $(OBJS) $(DEPS) $(BIN)
	$(RM) manifest

