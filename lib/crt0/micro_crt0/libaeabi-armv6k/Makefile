ARMGNU   = arm-none-eabi
AS       = $(ARMGNU)-as
CC       = clang
CCFLAGS  = -c -std=c99 -Wall -ffreestanding -O2 -nostdlib -nostdinc
CCFLAGS += -Wno-shift-count-overflow
CCFLAGS += -m32 -target armv6k-none-eabi
ASFLAGS  = -march=armv6k -EL

SRC_S_FILES = $(wildcard *.S)
SRC_C_FILES = $(wildcard *.c)
OBJ_C       = $(SRC_C_FILES:%.c=%.o)
OBJ_S       = $(SRC_S_FILES:%.S=%.o)

all: libaeabi-armv6k.a

$(OBJ_C): %.o:%.c
	@echo "CC $@"
	@$(CC) $(CCFLAGS) $*.c -S -o $*.s
	@$(AS) $(ASFLAGS) $*.s -o $@

$(OBJ_S): %.o:%.S
	@echo "AS $@"
	@$(AS) $(ASFLAGS) $*.S -o $@

libaeabi-armv6k.a: $(OBJ_C) $(OBJ_S)
	ar rcs libaeabi-armv6k.a $(OBJ_C) $(OBJ_S)

.PHONY: all clean

clean:
	-rm -f *.o *.a *.s
