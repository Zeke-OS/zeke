/**
 *******************************************************************************
 * @file    atomic.h
 * @author  Olli Vanhoja
 * @brief   Atomic integer operations.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef HAL_ATOMIC_H_
#define HAL_ATOMIC_H_

typedef int atomic_t __aligned(4);

#define __ATOMIC_DOP_FN(fn_name, opcode_str)                    \
static inline int fn_name(atomic_t * v, int i) {                \
    int old, new, err = 0;                                      \
    __asm__ volatile ("try%=:\n\t"                              \
                      "LDREX    %[old], [%[addr]]\n\t"          \
                      opcode_str " %[new], %[old], %[i]\n\t"    \
                      "STREX    %[res], %[new], [%[addr]]\n\t"  \
                      "CMP      %[res], #0\n\t"                 \
                      "BNE      try%="                          \
                      : [old]"+r" (old), [new]"+r" (new),       \
                        [res]"+r" (err),                        \
                        [addr]"+r" (v), [i]"+r" (i)             \
    ); return old;                                              \
}

#define ATOMIC_INIT(i) (i)

static inline int atomic_read(atomic_t * v)
{
    int value;

    __asm__ volatile (
        "LDREX      %[val], [%[addr]]\n\t"
        "CLREX"
        : [val]"+r" (value)
        : [addr]"r" (v)
    );

    return value;
}

static inline int atomic_set(atomic_t * v, int i)
{
   int old, err = 0;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "STREX      %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #0\n\t"
        "BNE        try%="
        : [old]"+r" (old), [new]"+r" (i), [res]"+r" (err), [addr]"+r" (v)
    );

    return old;
}

static inline int atomic_test_and_set(atomic_t * lock)
{
    int err = 1;

    __asm__ volatile (
        "MOV        r1, #1\n\t"             /* locked value to r1 */
        "LDREX      r2, [%[addr]]\n\t"      /* load value of the lock */
        "CMP        r2, r1\n\t"             /* if already set */
        "STREXNE    %[res], r1, [%[addr]]\n\t" /* Sets err = 0
                                                * if store op ok */
        : [res]"+r" (err)
        : [addr]"r" (lock)
        : "r1", "r2"
    );

    return err;
}

__ATOMIC_DOP_FN(atomic_add, "ADD");

__ATOMIC_DOP_FN(atomic_sub, "SUB");

static inline int atomic_inc(atomic_t * v)
{
    return atomic_add(v, 1);
}

static inline int atomic_dec(atomic_t * v)
{
    return atomic_sub(v, 1);
}

__ATOMIC_DOP_FN(atomic_and, "AND");

__ATOMIC_DOP_FN(atomic_or, "ORR");

__ATOMIC_DOP_FN(atomic_xor, "EOR");

static inline int atomic_set_bit(atomic_t * v, int i)
{
    int mask = 1 << i;

    return atomic_or(v, mask);
}

static inline int atomic_clear_bit(atomic_t * v, int i)
{
    int mask = ~(1 << i);

    return atomic_and(v, mask);
}

static inline int atomic_cmpxchg(atomic_t * v, int expect, int new)
{
    int old, err = 0;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "TEQ        %[old], %[expect]\n\t"
        "STREXEQ    %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #0\n\t"
        "BNE        try%="
        : [old]"+r" (old), [expect]"+r" (expect), [new]"+r" (new),
          [res]"+r" (err), [addr]"+r" (v)
    );

    return old;
}

static inline void * atomic_read_ptr(void ** v)
{
    void * value;

    __asm__ volatile (
        "LDREX      %[val], [%[addr]]\n\t"
        "CLREX"
        : [val]"+r" (value)
        : [addr]"r" (v)
    );

    return value;
}

static inline void * atomic_set_ptr(void ** v, void * new)
{
   void * old;
   int err = 0;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "STREX      %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #0\n\t"
        "BNE        try%="
        : [old]"+r" (old), [new]"+r" (new), [res]"+r" (err), [addr]"+r" (v)
    );

    return old;
}
static inline void * atomic_cmpxchg_ptr(void ** ptr, void * expect, void * new)
{
    void * old;
    int err = 0;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "TEQ        %[old], %[expect]\n\t"
        "STREXEQ    %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #0\n\t"
        "BNE        try%="
        : [old]"+r" (old), [expect]"+r" (expect), [new]"+r" (new),
          [res]"+r" (err), [addr]"+r" (ptr)
    );

    return old;
}

#endif /* HAL_ATOMIC_H_ */
