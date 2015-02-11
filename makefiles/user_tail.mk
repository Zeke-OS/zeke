# C Obj files
OBJS = $(patsubst %.c, %.o, $(SRC-y))

all: $(BIN) manifest

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	@echo "AS $@"
	@$(UNIFDEFALL) $(IDIR) $*.S | $(GNUARCH)-as -am $(IDIR) -o $@

$(OBJS): $(SRC-y) $(AUTOCONF_H)
	@echo "CC $@"
	$(eval CUR_BC := $*.bc)
	@$(CC) $(CCFLAGS) $(IDIR) -c $*.c -o $(CUR_BC)
	@$(OPT) $(OFLAGS) $(CUR_BC) -o - | $(LLC) $(LLCFLAGS) - -o - | \
		$(GNUARCH)-as - -o $@ $(ASFLAGS)

$(BIN): $(OBJS)
	$(eval CUR_BIN := $(basename $@))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_BIN)-ASRC-y)))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_BIN)-SRC-y)))
	@echo "LD $@"
	$(GNUARCH)-ld -o $@ -T $(ROOT_DIR)/$(ELFLD) $(LDFLAGS) $(ROOT_DIR)/lib/crt1.a $(LDIR) $(CUR_OBJS) -lc

manifest: $(BIN)
	echo "$(BIN)" > manifest

clean:
	$(RM) $(ASOBJS) $(OBJS) $(BIN)
	$(RM) manifest

