libc-ARFLAGS = \
$(patsubst %.c,%.o,$(crt0-SRC-y)) \
$(patsubst %.S,%.o,$(crt0-ASRC-y))

libc-ASRC-$(configARCH_ARM) := \
$(wildcard libc/setjmp/arm/*.S)

libc-SRC-y := \
$(wildcard libc/*.c) \
$(wildcard libc/_PDCLIB/*.c) \
$(wildcard libc/basecodecs/*.c) \
$(wildcard libc/ctype/*.c) \
$(wildcard libc/dirent/*.c) \
$(wildcard libc/fcntl/*.c) \
$(wildcard libc/grp/*.c) \
$(wildcard libc/inttypes/*.c) \
$(wildcard libc/libgen/*.c) \
$(wildcard libc/locale/*.c) \
$(wildcard libc/malloc/*.c) \
$(wildcard libc/math/*.c) \
$(wildcard libc/mman/*.c) \
$(wildcard libc/priv/*.c) \
$(wildcard libc/pthreads/*.c) \
$(wildcard libc/pwd/*.c) \
$(wildcard libc/regexp/*.c) \
$(wildcard libc/resource/*.c) \
$(wildcard libc/sched/*.c) \
$(wildcard libc/setjmp/*.c) \
$(wildcard libc/signal/*.c) \
$(wildcard libc/stat/*.c) \
$(wildcard libc/stdio/*.c) \
$(wildcard libc/stdlib/*.c) \
$(wildcard libc/string/*.c) \
$(wildcard libc/termios/*.c) \
$(wildcard libc/time/*.c) \
$(wildcard libc/uchar/*.c) \
$(wildcard libc/unistd/*.c) \
$(wildcard libc/wchar/*.c) \
$(wildcard libc/wctype/*.c) \
$(wildcard libc/zeke/*.c)

