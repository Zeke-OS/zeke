/*-
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <kmalloc.h>
#include <hal/core.h>
#include <kerror.h>
#include <subr_hash.h>

LIST_HEAD(generic, generic);

/**
 * General routine to allocate a hash table with control of memory flags.
 */
void * hashinit_flags(int elements, unsigned long * hashmask, int flags)
{
    long hashsize;
    struct generic * hashtbl;

    KASSERT(elements > 0, "bad elements");
    /* Exactly one of HASH_WAITOK and HASH_NOWAIT must be set. */
    KASSERT((flags & HASH_WAITOK) ^ (flags & HASH_NOWAIT),
            "Bad flags passed to hashinit_flags");

    for (hashsize = 1; hashsize <= elements; hashsize <<= 1) {
        continue;
    }
    hashsize >>= 1;

    /*
     * TODO No support for NOWAIT atm
     * if (flags & HASH_NOWAIT)
     */

    hashtbl = kmalloc((unsigned long)hashsize * sizeof(*hashtbl));

    if (hashtbl != NULL) {
        for (long i = 0; i < hashsize; i++) {
            LIST_INIT(&hashtbl[i]);
        }

        *hashmask = hashsize - 1;
    }

    return hashtbl;
}

/**
 * Allocate and initialize a hash table with default flag: may sleep.
 */
void * hashinit(int elements, unsigned long * hashmask)
{
    return hashinit_flags(elements, hashmask, HASH_WAITOK);
}

void hashdestroy(void * vhashtbl, unsigned long hashmask)
{
    struct generic * hashtbl;
    struct generic * hp;

    hashtbl = vhashtbl;
    for (hp = hashtbl; hp <= &hashtbl[hashmask]; hp++) {
        KASSERT(LIST_EMPTY(hp), "hash not empty");
    }
    kfree(hashtbl);
}

static const int primes[] = {
    1, 13, 31, 61, 127, 251, 509, 761, 1021, 1531,
    2039, 2557, 3067, 3583, 4093, 4603, 5119, 5623, 6143,
    6653, 7159, 7673, 8191, 12281, 16381, 24571, 32749
};
#define NPRIMES (sizeof(primes) / sizeof(primes[0]))

/**
 * General routine to allocate a prime number sized hash table.
 */
void * phashinit(int elements, unsigned long * nentries)
{
    long hashsize;
    struct generic * hashtbl;
    int i;

    KASSERT(elements > 0, "bad elements");

    for (i = 1, hashsize = primes[1]; hashsize <= elements;) {
        i++;
        if (i == NPRIMES)
            break;

        hashsize = primes[i];
    }

    hashsize = primes[i - 1];
    hashtbl = kmalloc((unsigned long)hashsize * sizeof(*hashtbl));

    for (i = 0; i < hashsize; i++) {
        LIST_INIT(&hashtbl[i]);
    }
    *nentries = hashsize;

    return hashtbl;
}
