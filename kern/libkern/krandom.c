/**
 *******************************************************************************
 * @file    krandom.h
 * @author  Olli Vanhoja
 * @brief   Kernel random.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1992, 1993
 *        The Regents of the University of California.  All rights reserved.
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

#include <limits.h>
#include <kinit.h>
#include <hal/core.h>
#include "libkern.h"

#define NSHUFF 50 /* to drop some "seed -> 1st value" linearity */
#define RAND_MAX 0x7fffffff

static unsigned long randseed = 937186357; /* after srandom(1), NSHUFF counted */

void ksrandom(unsigned long seed)
{
    int i;

    randseed = seed;
    for (i = 0; i < NSHUFF; i++)
        (void)krandom();
}

unsigned long krandom(void)
{
    long x, hi, lo, t;

    /*
     * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
     * From "Random number generators: good ones are hard to find",
     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
     * October 1988, p. 1195.
     */

    /* Can't be initialized with 0, so use another value. */
    if ((x = randseed) == 0)
        x = 123459876;

    hi = x / 127773;
    lo = x % 127773;
    t = 16807 * lo - 2836 * hi;
    if (t < 0)
        t += 0x7fffffff;

    randseed = t;

    return t;
}

long kunirand(long n)
{
    long partSize = (n == RAND_MAX) ? 1 : 1 + (RAND_MAX - n)/(n + 1);
    long maxUsefull = partSize * n + (partSize - 1);
    long draw;

    do {
        draw = krandom();
    } while (draw > maxUsefull);

    return draw / partSize;
}

int random_init(void) __attribute__((constructor));
int random_init(void)
{
    SUBSYS_INIT("krandom");

    ksrandom((uint32_t)(get_utime() % (uint64_t)0xffffffff));

    return 0;
}
