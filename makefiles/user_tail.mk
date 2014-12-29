# C Obj files
BCS = $(patsubst %.c, %.bc, $(SRC-y))
OBJS = $(patsubst %.c, %.o, $(SRC-y))

all: $(BIN)
	echo "$(BIN)" > manifest

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	@echo "AS $@"
	@$(UNIFDEFALL) $(IDIR) $*.S | $(ARMGNU)-as -am $(IDIR) -o $@

$(OBJS): $(SRC-y) $(AUTOCONF_H)
	$(eval CUR_BC := $*.bc)
	$(eval CUR_OPT := $*.opt.bc)
	$(eval CUR_OPT_S := $*.opt.s)
	@echo "CC $@"
	@$(CC) $(CCFLAGS) $(IDIR) -c $*.c -o $(CUR_BC)
	@$(OPT) $(OFLAGS) $*.bc -o $(CUR_OPT)
	@$(LLC) $(LLCFLAGS) $(CUR_OPT) -o $(CUR_OPT_S)
	@$(ARMGNU)-as $(CUR_OPT_S) -o $@ $(ASFLAGS)

$(BIN): $(OBJS)
	$(eval CUR_BIN := $(basename $@))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_BIN)-ASRC-y)))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_BIN)-SRC-y)))
	@echo "LD $@"
	$(ARMGNU)-ld -o $@ -T $(ROOT_DIR)/$(ELFLD) $(LDFLAGS) $(ROOT_DIR)/lib/crt1.a $(LDIR) $(CUR_OBJS) -lc

clean:
	$(RM) $(ASOBJS) $(OBJS) $(BIN)
	$(RM) manifest

