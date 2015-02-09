/**
 *******************************************************************************
 * @file    memcpy.c
 * @brief   memcpy (part of kstring.h)
 *******************************************************************************
 */

#include <kstring.h>

/* It's ok to override thse functions by core specific optimized versions.
 * Eg. ARMv7 adds possibility to use hardware memcpy which is around 30 times
 * faster than this approach. */
void * memcpy(void * restrict destination, const void * source, size_t num) __attribute__ ((weak));
void __aeabi_memcpy(void *destination, const void *source, size_t num) __attribute__ ((weak));
void __aeabi_memcpy4(void *destination, const void *source, size_t num) __attribute__ ((weak));
void __aeabi_memcpy8(void *destination, const void *source, size_t num) __attribute__ ((weak));

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* TODO Check in which platforms this optimization is valid */
#if __ARM6K__
#define ARM_OPTIMIZATION 1
#define BIGBLOCKSIZE    (sizeof(size_t) << 2)
#else /* No ARM optimization */
/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof(long) << 2)
#endif

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof(long))

/* Threshold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

void * memcpy(void * restrict destination, const void * source, size_t num)
{
#ifdef __OPTIMIZE_SIZE__
    char * dst = (char *) destination;
    char * src = (char *) source;

    void * save = destination;

    while (num--) {
        *dst++ = *src++;
    }

    return save;
#else
    char * dst = destination;
    const char * src = source;
    long * aligned_dst;
    const long * aligned_src;

    /* If the size is small, or either SRC or DST is unaligned,
     * then punt into the byte copy loop.  This should be rare.  */
    if (!TOO_SMALL(num) && !UNALIGNED (src, dst)) {
        aligned_dst = (long*)dst;
        aligned_src = (long*)src;

#ifdef ARM_OPTIMIZATION
        while (num >= BIGBLOCKSIZE) {
            __asm__ volatile (
                "ldmia      %[src]!, {r4-r7}\n\t"
                "stmia      %[dst]!, {r4-r7}\n\t"
                : [src]"+r" (aligned_src), [dst]"+r" (aligned_dst)
                : : "r4", "r5", "r6", "r7", "memory");
            num -= BIGBLOCKSIZE;
        }
#else
        /* Copy 4X long words at a time if possible.  */
        while (num >= BIGBLOCKSIZE) {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            num -= BIGBLOCKSIZE;
        }
#endif

        /* Copy one long word at a time if possible.  */
        while (num >= LITTLEBLOCKSIZE) {
            *aligned_dst++ = *aligned_src++;
            num -= LITTLEBLOCKSIZE;
        }

        /* Pick up any residual with a byte copier.  */
        dst = (char*)aligned_dst;
        src = (char*)aligned_src;
    }

    while (num--)
        *dst++ = *src++;

    return destination;
#endif
}

void __aeabi_memcpy(void *destination, const void *source, size_t num)
{
    memcpy(destination, source, num);
}

void __aeabi_memcpy4(void *destination, const void *source, size_t num)
{
    memcpy(destination, source, num);
}

void __aeabi_memcpy8(void *destination, const void *source, size_t num)
{
    memcpy(destination, source, num);
}
