# Sort and remove duplicates
SRC-y := $(sort $(SRC-y))

# Obj files
OBJS := $(patsubst %.c, %.o, $(SRC-y))
DEPS := $(patsubst %.c, %.d, $(SRC-y))

FILES += $(FILES-y)

include $(ROOT_DIR)/makefiles/commands.mk

all: $(BIN-y) manifest

$(ASOBJS): $(ASRC-y) $(AUTOCONF_H)
	$(as-command)

$(OBJS): %.o: %.c $(AUTOCONF_H)
	$(cc-command)

$(BIN-y): $(OBJS)
	@echo "LD $@"
	$(eval CUR_BIN := $(basename $@))
	$(eval CUR_OBJS += $(patsubst %.S, %.o, $($(CUR_BIN)-ASRC-y)))
	$(eval CUR_OBJS := $(patsubst %.c, %.o, $($(CUR_BIN)-SRC-y)))
	@$(GNUARCH)-ld -o $@ -T $(ROOT_DIR)/$(ELFLD) $(LDFLAGS) \
		$(ROOT_DIR)/lib/crt1.a $(LDIR) $(CUR_OBJS) $($(CUR_BIN)-LDFLAGS) -lc

-include $(DEPS)

manifest: $(BIN-y) $(FILES)
	echo "$(strip $(BIN-y) $(FILES))" > manifest

clean:
	$(RM) $(ASOBJS) $(OBJS) $(DEPS) $(BIN-y) $(CLEAN_FILES)
	$(RM) manifest

