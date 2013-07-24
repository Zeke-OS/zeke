/**
 *******************************************************************************
 * @file    memcpy.c
 * @brief   memcpy (part of string.h)
 *******************************************************************************
 */

#include "string.h"

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

/* Threshold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

void * memcpy(void * restrict destination, const void * source, ksize_t num)
{
    char * dst = destination;
    const char * src = source;
    long * aligned_dst;
    const long * aligned_src;

    /* If the size is small, or either SRC or DST is unaligned,
     * then punt into the byte copy loop.  This should be rare.  */
    if (!TOO_SMALL(num) && !UNALIGNED (src, dst))
    {
        aligned_dst = (long*)dst;
        aligned_src = (long*)src;

        /* Copy 4X long words at a time if possible.  */
        while (num >= BIGBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            num -= BIGBLOCKSIZE;
            }

        /* Copy one long word at a time if possible.  */
        while (num >= LITTLEBLOCKSIZE)
        {
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
}
