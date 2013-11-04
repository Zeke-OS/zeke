/**
 *******************************************************************************
 * @file    bitmap.c
 * @author  Olli Vanhoja
 * @brief   bitmap allocation functions.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <bitmap.h>

/**
 * Search for a contiguous block of block_len in bitmap.
 * @param retval[out]   Index of the first contiguous block of requested length.
 * @param block_len     is the lenght of contiguous block searched for.
 * @param bitmap        is a bitmap of block reservations.
 * @param size          is the size of bitmap in bytes.
 * @return  Returns zero if a free block found; Value other than zero if there
 *          is no free contiguous block of requested length.
 */
int bitmap_block_search(size_t * retval, size_t block_len, bitmap_t * bitmap, size_t size)
{
    size_t i, j;
    bitmap_t * cur;
    size_t start = 0, end = 0;

    block_len--;
    cur = &start;
    for (i = 0; i < (size / sizeof(bitmap_t)); i++) {
        for(j = 0; j < SIZEOF_BITMAP_T; j++) {
            if ((bitmap[i] & (1 << j)) == 0) {
                *cur = i * SIZEOF_BITMAP_T + j;
                cur = &end;

                if ((end - start >= block_len) && (end >= start)) {
                    *retval = start;
                    return 0;
                }
            } else {
                start = 0;
                end = 0;
                cur = &start;
            }
        }
    }

    return 1;
}

/**
 * Set or clear contiguous block of bits in bitmap.
 * @param bitmap    is the bitmap being changed.
 * @param mark      0 = clear; 1 = set;
 * @param start     is the starting bit position in bitmap.
 * @param len       is the length of the block being updated.
 */
void bitmap_block_update(bitmap_t * bitmap, unsigned int mark, size_t start, size_t len)
{
    size_t i, j, n;
    bitmap_t  tmp;
    size_t k = (start - (start & (SIZEOF_BITMAP_T - 1))) / SIZEOF_BITMAP_T;
    size_t max = k + len / SIZEOF_BITMAP_T + 1;

    mark &= 1;

    /* start mod size of bitmap_t in bits */
    n = start & (SIZEOF_BITMAP_T - 1);

    j = 0;
    for (i = k; i <= max; i++) {
        while (n < SIZEOF_BITMAP_T) {
            tmp = mark << n;
            bitmap[i]  &= ~(1 << n);
            bitmap[i] |= tmp;
            n++;
            if (++j >= len)
                return;
        }
        n = 0;
    }
}

