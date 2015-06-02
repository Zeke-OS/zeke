/**
 *******************************************************************************
 * @file    segtree.c
 * @author  Olli Vanhoja
 * @brief   Generic min/max segment tree.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kmalloc.h>
#include <segtree.h>

struct segt * segt_init(size_t n, segtcmp_t cmp)
{
    struct segt * s;

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    s = kzalloc(sizeof(struct segt) + 2 * n * sizeof(void *));
    s->cmp = cmp;
    s->n = n;

    return s;
}

void segt_free(struct segt * s)
{
    kfree(s);
}

void segt_alt(struct segt * s, size_t k, void * x)
{
    k += s->n;
    s->arr[k] = x;
    for (k >>= 1; k >= 1; k >>= 1) {
        size_t j = k << 1;

        s->arr[k] = s->cmp(s->arr[j], s->arr[j + 1]);
    }
}

void * segt_find(struct segt * s, size_t a, size_t b)
{
    void * q;

    a += s->n;
    b += s->n;

    q = s->arr[a];
    while (a <= b) {
        if (a & 1) {
            void * pa = s->arr[a];
            void * p;

            p = s->cmp(q, pa);
            if (p)
                q = p;
            a++;
        }
        if (!(b & 1)) {
            void * pb = s->arr[b];
            void * p;

            p = s->cmp(q, pb);
            if (p)
                q = p;
            b--;
        }
        a >>= 1;
        b >>= 1;
    }

    return q;
}
