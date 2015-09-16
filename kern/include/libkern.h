/**
 *******************************************************************************
 * @file    libkern.h
 * @author  Olli Vanhoja
 * @brief   Generic functions and macros for use in kernel.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 * Generic functions and macros for use in kernel.
 * @{
 */

#pragma once
#ifndef LIBKERN_H
#define LIBKERN_H

#include <stddef.h>
#include <sys/types.h>

/**
 * Returns a container of ptr, which is a element in some struct.
 * @param ptr       is a pointer to a element in struct.
 * @param type      is the type of the container struct.
 * @param member    is the name of the ptr in container struct.
 * @return Pointer to the container of ptr.
 */
#define container_of(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

/**
 * Returns the size of a member of type.
 * @param type      is a type.
 * @param member    is a member in type.
 * @return The size of the member.
 */
#define member_size(type, member) (sizeof(((type *)0)->member))

#ifndef num_elem
/**
 * Get number of elements in an array that is known at compile time.
 * @param x         is an array.
 * @return The number of elements in array.
 */
#define num_elem(x) (sizeof(x) / sizeof(*(x)))
#endif

/* log_2 */
#define NBITS2(n) ((n & 2) ? 1 : 0)
#define NBITS4(n) ((n & (0xC)) ? (2 + NBITS2(n >> 2)) : (NBITS2(n)))
#define NBITS8(n) ((n & 0xF0) ? (4 + NBITS4(n >> 4)) : (NBITS4(n)))
#define NBITS16(n) ((n & 0xFF00) ? (8 + NBITS8(n >> 8)) : (NBITS8(n)))
#define NBITS32(n) ((n & 0xFFFF0000) ? (16 + NBITS16(n >> 16)) : (NBITS16(n)))
#define NBITS(n) (n == 0 ? 0 : NBITS32(n) + 1) /*!< log_2 2 of n. */

static inline int imax(int a, int b)
{ return (a > b ? a : b); }

static inline int imin(int a, int b)
{ return (a < b ? a : b); }

static inline long lmax(long a, long b)
{ return (a > b ? a : b); }

static inline long lmin(long a, long b)
{ return (a < b ? a : b); }

static inline unsigned int max(unsigned int a, unsigned int b)
{ return (a > b ? a : b); }

static inline unsigned int min(unsigned int a, unsigned int b)
{ return (a < b ? a : b); }

static inline unsigned long ulmax(unsigned long a, unsigned long b)
{ return (a > b ? a : b); }

static inline unsigned long ulmin(unsigned long a, unsigned long b)
{ return (a < b ? a : b); }

static inline off_t omax(off_t a, off_t b)
{ return (a > b ? a : b); }

static inline off_t omin(off_t a, off_t b)
{ return (a < b ? a : b); }

static inline int abs(int a) { return (a < 0 ? -a : a); }
static inline long labs(long a) { return (a < 0 ? -a : a); }

/**
 * Return word aligned size of size.
 * @param size is a size of a memory block requested.
 * @returns Returns size aligned to the word size of the current system.
 */
size_t memalign(size_t size);

/**
 * Return page aligned size of size.
 * @param size is a size of a memory block requested.
 * @param bytes align to bytes.
 * @returns Returns size aligned to the word size of the current system.
 */
size_t memalign_size(size_t size, size_t bytes);

/**
 * @addtogroup krandom krandom, ksrandom, kunirandom
 * @{
 */

/**
 * Seed the pseudo-random number generator.
 * @param seed is the seed to be used.
 */
void ksrandom(unsigned long seed);

/**
 * Pseudo-random number generator.
 * Pseudo-random number generator for randomizing the profiling clock,
 * and whatever else we might use it for.  The result is uniform on
 * [0, 2^31 - 1].
 * @return Returns a pseudo-random number.
 */
uint32_t krandom(void);
uint32_t kunirand(unsigned long n);

/**
 * @}
 */

static inline void write_word(uint32_t val, uint8_t *buf, int offset)
{
    buf[offset + 0] = val & 0xff;
    buf[offset + 1] = (val >> 8) & 0xff;
    buf[offset + 2] = (val >> 16) & 0xff;
    buf[offset + 3] = (val >> 24) & 0xff;
}

static inline void write_halfword(uint16_t val, uint8_t *buf, int offset)
{
    buf[offset + 0] = val & 0xff;
    buf[offset + 1] = (val >> 8) & 0xff;
}

static inline void write_byte(uint8_t byte, uint8_t *buf, int offset)
{
    buf[offset] = byte;
}

static inline uint32_t read_word(uint8_t * buf, int offset)
{
    uint32_t b0 = buf[offset + 0] & 0xff;
    uint32_t b1 = buf[offset + 1] & 0xff;
    uint32_t b2 = buf[offset + 2] & 0xff;
    uint32_t b3 = buf[offset + 3] & 0xff;

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

static inline uint16_t read_halfword(uint8_t * buf, int offset)
{
    uint16_t b0 = buf[offset + 0] & 0xff;
    uint16_t b1 = buf[offset + 1] & 0xff;

    return b0 | (b1 << 8);
}

static inline uint8_t read_byte(uint8_t * buf, int offset)
{
    return buf[offset];
}

int sizetto(size_t value, void * p, size_t size);

/**
 * @addtogroup ffs ffs, ffsl, ffsll, fls, flsl, flsll
 * @{
 */

/**
 * Find first bit set in a word.
 * @return the position of the first bit set, or 0 if no bits are set in mask.
 */
static inline int ffs(int mask)
{
    return __builtin_ffs(mask);
}

/**
 * Find first bit set in a word.
 * @return the position of the first bit set, or 0 if no bits are set in mask.
 */
static inline int ffsl(long mask)
{
    return __builtin_ffsl(mask);
}

/**
 * Find first bit set in a word.
 * @return the position of the first bit set, or 0 if no bits are set in mask.
 */
static inline int ffsll(long long mask)
{
    return __builtin_ffsll(mask);
}

/**
 * Find the last bit set in a word.
 */
int fls(int mask);

/**
 * Find the last bit set in a word.
 */
int flsl(long mask);

/**
 * Find the last bit set in a word.
 */
int flsll(long long mask);

/**
 * @}
 */

/**
 * Parse path and file name from a complete path.
 * <path>/<name>
 * Out arguments are kmalloc'd and should be freed by the caller after use;
 * Out arguments can be NULL.
 * @param pathname  is a complete path to a file or directory.
 * @param path[out] is the expected dirtory part of the path.
 * @param name[out] is the file or directory name parsed from the full path.
 * @return 0 if succeed; Otherwise a negative errno is returned.
 */
int parsenames(const char * pathname, char ** path, char ** name);

#endif /* LIBKERN_H */

/**
 * @}
 */
