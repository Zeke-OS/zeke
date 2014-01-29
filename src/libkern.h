/**
 *******************************************************************************
 * @file    libkern.h
 * @author  Olli Vanhoja
 * @brief   Generic functions and macros for use in kernel.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * Get number of elements in an array that is known at compile time.
 * @param x         is an array.
 * @return The number of elements in array.
 */
#define num_elem(x) (sizeof(x) / sizeof(*(x)))

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

void ksrandom(unsigned long seed);
unsigned long krandom(void);
static long kunirand(long n);

#endif /* LIBKERN_H */

