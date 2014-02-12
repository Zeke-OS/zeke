AS=arm-none-eabi-as -mthumb

SRC_FILES=$(wildcard *.S) 

%.o: %.S
	@echo "AS $@"
	@$(AS) $< -o $@

libaeabi-armv6-m.a: $(SRC_FILES:%.S=%.o)
	ar rcs libaeabi-armv6-m.a $(SRC_FILES:%.S=%.o)

.PHONY: all clean

all: libaeabi-armv6-m.a

clean:
	-rm -f *.o *.a
