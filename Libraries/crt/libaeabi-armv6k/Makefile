AS=arm-none-eabi-as

SRC_FILES=$(wildcard *.S) 

%.o: %.S
	$(AS) $< -o $@

libaeabi-armv6k.a: $(SRC_FILES:%.S=%.o)
	ar rcs libaeabi-armv6k.a $(SRC_FILES:%.S=%.o)

.PHONY: all clean

all: libaeabi-armv6k.a

clean:
	-rm -f *.o *.a
