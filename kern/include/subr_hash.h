/*
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2004 Joseph Koshy
 * Copyright (c) 1982, 1986, 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @addtogroup libkern
 * @{
 */

/**
 * @addtogroup subr_hash Subr_hash
 * The hashinit(), hashinit_flags() and phashinit() functions allocate space
 * for hash tables of size given by the argument nelements.
 *
 * The hashinit() function allocates hash tables that are sized to largest power
 * of two less than or equal to argument nelements. The phashinit() function
 * allocates hash tables that are sized to the largest prime number less than or
 * equal to argument nelements. The hashinit_flags() function operates like
 * hashinit() but also accepts an additional argument flags which control
 * various options during allocation. Allocated hash tables are contiguous
 * arrays of LIST_HEAD(3) entries, allocated using malloc(9), and initialized
 * using LIST_INIT(3). The malloc arena to be used for allocation is pointed to
 * by argument type.
 *
 * The hashdestroy() function frees the space occupied by the hash table pointed
 * to by argument hashtbl. Argument type determines the malloc arena to use when
 * freeing space. The argument hashmask should be the bit mask returned by the
 * call to hashinit() that allocated the hash table. The argument flags must be
 * used with one of the following values.
 *
 * @warning The hashinit() and phashinit() functions will panic if argument
 *          nelements is less than or equal to zero.
 * @warning The hashdestroy() function will panic if the hash table pointed
 *          to by hashtbl is not empty.
 * @bug     There is no phashdestroy() function, and using hashdestroy() to
 *          free a hash table allocated by phashinit() usually has grave
 *          consequences.
 *
 * Examples
 * ========
 *
 * A typical example is shown below:
 *
 * @code{.c}
 * ...
 * static LIST_HEAD(foo, foo) *footable;
 * static u_long foomask;
 * ...
 * footable = hashinit(32, M_FOO, &foomask);
 * @endcode
 *
 * Here we allocate a hash table with 32 entries from the malloc arena pointed
 * to by M_FOO. The mask for the allocated hash table is returned in foomask.
 * A subsequent call to hashdestroy() uses the value in foomask:
 *
 * @code{.c}
 * ...
 * hashdestroy(footable, M_FOO, foomask);
 * @endcode
 * @{
 */

#pragma once
#ifndef SUBR_HASH_H
#define SUBR_HASH_H

/**
 * Init a hash table.
 * The hashinit() function allocates hash tables that are sized to largest
 * power of two less than or equal to  argument nelements.
 *
 * The hashinit() function returns a pointer to an allocated hash table and
 * sets the location pointed to by hashmask to the bit mask to be used for
 * computing the correct slot in the hash table.
 */
void * hashinit(int count, unsigned long * hashmask);
void * hashinit_flags(int count, unsigned long * hashmask, int flags);

/**
 * Any malloc performed by  the hashinit_flags() function
 * will not be allowed to wait, and therefore may fail.
 */
#define HASH_NOWAIT     0x00000001

/**
 * Any malloc performed by  the hashinit_flags() function
 * is allowed to wait for memory.
 */
#define HASH_WAITOK     0x00000002

/**
 * Destroy a hash table.
 * The hashdestroy() function frees the space occupied by the hash table
 * pointed to by argument hashtbl.
 *
 * The hashdestroy() function will panic if the hash table pointed to by
 * hashtbl is not empty.
 */
void hashdestroy(void *, unsigned long);

/**
 * The phashinit() function returns a pointer to an allocated hash table
 * and sets the location pointed to by nentries to the number of rows in
 * the hash table.
 */
void * phashinit(int count, unsigned long * nentries);

#endif /* SUBR_HASH_H */

/**
 * @}
 */

/**
 * @}
 */
