# C Obj files
OBJS = $(patsubst %.c, %.o, $(SRC-y))

all: $(BIN) manifest

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	@echo "AS $@"
	@$(UNIFDEFALL) $(IDIR) $*.S | $(ARMGNU)-as -am $(IDIR) -o $@

$(OBJS): $(SRC-y) $(AUTOCONF_H)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) $(IDIR) -c $*.c -o /dev/stdout | $(OPT) $(OFLAGS) - -o - | $(LLC) $(LLCFLAGS) - -o - | $(ARMGNU)-as - -o $@ $(ASFLAGS)

$(BIN): $(OBJS)
	$(eval CUR_BIN := $(basename $@))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_BIN)-ASRC-y)))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_BIN)-SRC-y)))
	@echo "LD $@"
	$(ARMGNU)-ld -o $@ -T $(ROOT_DIR)/$(ELFLD) $(LDFLAGS) $(ROOT_DIR)/lib/crt1.a $(LDIR) $(CUR_OBJS) -lc

manifest: $(BIN)
	echo "$(BIN)" > manifest

clean:
	$(RM) $(ASOBJS) $(OBJS) $(BIN)
	$(RM) manifest

