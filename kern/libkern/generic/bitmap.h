/**
 *******************************************************************************
 * @file    bitmap.h
 * @author  Olli Vanhoja
 * @brief   Bitmap allocation functions.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdint.h>
#include <stddef.h>

typedef uint32_t bitmap_t;

/**
 * Returns size of static bitmap in bits.
 * @param bmap Bitmap.
 * @return Size of given bitmap in bits.
 */
#define SIZEOF_BITMAP(bmap) (sizeof(bmap) * 8)

#define SIZEOF_BITMAP_T (sizeof(bitmap_t) * 8)

/**
 * Convert from number of entries to bitmap size.
 * Unfortunately SIZEOF_BITMAP_T must be hard coded in this macro to make it
 * work it with #ifs.
 * @param entries Number of entries needed.
 * @return Correct size for a bitmap_t array.
 */
#define E2BITMAP_SIZE(entries) ((entries) / (4*8))

int bitmap_block_search(size_t * retval, size_t block_len, bitmap_t * bitmap, size_t size);
void bitmap_block_update(bitmap_t * bitmap, unsigned int mark, size_t start, size_t len);
int bitmap_block_alloc(size_t * start, size_t len, bitmap_t * bitmap, size_t size);
int bitmap_block_align_alloc(size_t * start, size_t len,
        bitmap_t * bitmap, size_t size, size_t balign);
