/**
 *******************************************************************************
 * @file    bitmap.h
 * @author  Olli Vanhoja
 * @brief   Bitmap allocation functions.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup libkern
 * @{
 */

/**
 * @addtogroup bitmap
 * @{
 */

#pragma once
#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t bitmap_t;

/**
 * Returns size of static bitmap in bits.
 * @param bmap Bitmap.
 * @return Size of given bitmap in bits.
 */
#define SIZEOF_BITMAP(bmap) (8 * sizeof(bmap))

/**
 * Convert from number of entries to bitmap size.
 * Unfortunately SIZEOF_BITMAP_T must be hard coded in this macro to make it
 * work it with #ifs.
 * @param entries Number of entries needed.
 * @return Correct size for a bitmap_t array.
 */
#define E2BITMAP_SIZE(entries) ((entries) / (4 * 8))

/**
 * Search for a contiguous block of zeroes of block_len in bitmap.
 * @param[out] retval   is the index of the first contiguous block of requested
 *                      length.
 * @param block_len     is the lenght of contiguous block searched for.
 * @param bitmap        is a bitmap of block reservations.
 * @param size          is the size of bitmap in bytes.
 * @return  Returns zero if a free block found; Value other than zero if there
 *          is no free contiguous block of requested length.
 */
int bitmap_block_search(size_t * retval, size_t block_len,
                        const bitmap_t * bitmap, const size_t size);

/**
 * Search for a contiguous block of zeroes of block_len in bitmap.
 * @param       start       is the index where lookup starts from.
 * @param[out] retval       is the index of the first contiguous block of
 *                          the requested length.
 * @param       block_len   is the lenght of contiguous block searched for.
 * @param       bitmap      is a bitmap of block reservations.
 * @param       size        is the size of bitmap in bytes.
 * @return  Returns zero if a free block found; Value other than zero if there
 *          is no free contiguous block of requested length.
 */
int bitmap_block_search_s(size_t start, size_t * retval, size_t block_len,
                          const bitmap_t * bitmap, size_t size);

/**
 * Check status of a bit in a bitmap pointed by bitmap.
 * @param bitmap            is a bitmap.
 * @param pos               is the bit position to be checked.
 * @param size              is the size of bitmap in bytes.
 * @return  Boolean value or -EINVAL.
 */
int bitmap_status(const bitmap_t * bitmap, size_t pos, size_t size);

/**
 * Set a bit in a bitmap pointed by bitmap.
 * @param bitmap            is a bitmap.
 * @param pos               is the bit position to be set.
 * @param size              is the size of bitmap in bytes.
 * @return  0 or -EINVAL.
 */
int bitmap_set(bitmap_t * bitmap, size_t pos, size_t size);

/**
 * Clear a bit in a bitmap pointed by bitmap.
 * @param bitmap            is a bitmap.
 * @param pos               is the bit position to be cleared.
 * @param size              is the size of bitmap in bytes.
 * @return  0 or -EINVAL.
 */
int bitmap_clear(bitmap_t * bitmap, size_t pos, size_t size);

/**
 * Set or clear contiguous block of bits in bitmap.
 * @param bitmap    is the bitmap being changed.
 * @param mark      0 = clear; 1 = set;
 * @param start     is the starting bit position in bitmap.
 * @param len       is the length of the block being updated.
 * @param size      is the size of bitmap in bytes.
 */
int bitmap_block_update(bitmap_t * bitmap, unsigned int mark, size_t start,
                        size_t len, size_t size);

/**
 * Set a contiguous block of zeroed bits to ones and return starting index.
 * @param[out] start    is the starting bit (index) of the newly allocated area.
 * @param len           is the length of zeroed area to be set.
 * @param bitmap        is the bitmap being changed.
 * @param size          is the size of the bitmap in bytes.
 * @return  Returns zero if a free block found; Value other than zero if there
 *          is no free contiguous block of requested length.
 */
int bitmap_block_alloc(size_t * start, size_t len, bitmap_t * bitmap,
                       size_t size);

/**
 * Allocate a contiguous aligned block of bits from bitmap.
 * @param[out] start    is the starting bit (index) of the newly allocated area.
 * @param len           is the length of zeroed area to be set.
 * @param bitmap        is the bitmap being changed.
 * @param size          is the size of the bitmap in bytes.
 * @return  Returns zero if a free block found; Value other than zero if there
 *          is no free contiguous block of requested length.
 */
int bitmap_block_align_alloc(size_t * start, size_t len,
        bitmap_t * bitmap, size_t size, size_t balign);

#endif /* BITMAP_H */

/**
 * @}
 */

/**
 * @}
 */
