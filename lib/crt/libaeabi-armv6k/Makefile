ARMGNU   = arm-none-eabi
AS       = $(ARMGNU)-as
CC       = clang
CCFLAGS  = -std=c99 -Wall -ffreestanding -O2 -nostdlib -nostdinc
CCFLAGS += -m32 -ccc-host-triple $(ARMGNU)

SRC_S_FILES = $(wildcard *.S)
SRC_C_FILES = $(wildcard *.c)
OBJ_FILES   = $(SRC_S_FILES:%.S=%.o) $(SRC_C_FILES:%.c=%.o)

%.o: %.S
	$(AS) $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) $< -o $@

libaeabi-armv6k.a: $(OBJ_FILES)
	ar rcs libaeabi-armv6k.a $(OBJ_FILES)

.PHONY: all clean

all: libaeabi-armv6k.a

clean:
	-rm -f *.o *.a
