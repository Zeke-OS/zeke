/**
 *******************************************************************************
 * @file    arm11_atomic.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL
#include <hal/atomic.h>

int atomic_read(atomic_t * v)
{
    int value;

    __asm__ volatile (
        "LDREX      %[val], [%[addr]]"
        : [val]"+r" (value)
        : [addr]"r" (v)
    );

    return value;
}

int atomic_set(atomic_t * v, int i)
{
   int old, err;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "STREX      %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #1\n\t"
        "BEQ        try%="
        : [old]"+r" (old), [new]"+r" (i), [res]"+r" (err), [addr]"+r" (v)
    );

    return old;
}

int atomic_add(atomic_t * v, int i)
{
    int old, new, err;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "ADD        %[new], %[old], %[i]\n\t"
        "STREX      %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #1\n\t"
        "BEQ        try%="
        : [old]"+r" (old), [new]"+r" (new), [res]"+r" (err),
          [addr]"+r" (v), [i]"+r" (i)
    );

    return old;
}

int atomic_sub(atomic_t * v, int i)
{
    int old, new, err;

    __asm__ volatile (
        "try%=:\n\t"
        "LDREX      %[old], [%[addr]]\n\t"
        "SUB        %[new], %[old], %[i]\n\t"
        "STREX      %[res], %[new], [%[addr]]\n\t"
        "CMP        %[res], #1\n\t"
        "BEQ        try%="
        : [old]"+r" (old), [new]"+r" (new), [res]"+r" (err),
          [addr]"+r" (v), [i]"+r" (i)
    );

    return old;
}

int atomic_inc(atomic_t * v)
{
    return atomic_add(v, 1);
}

int atomic_dec(atomic_t * v)
{
    return atomic_sub(v, 1);
}
