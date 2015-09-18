/**
 *******************************************************************************
 * @file    kobj.c
 * @author  Olli Vanhoja
 * @brief   Generic kernel object interface.
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

#include <errno.h>
#include <kobj.h>
#include <libkern.h>

void kobj_init(struct kobj * p, void (*ko_free)(struct kobj * p))
{
    p->ko_free = ko_free;
    p->ko_fast_lock = ATOMIC_INIT(0);
    p->ko_refcount = ATOMIC_INIT(1);
}

static int kobj_fast_lock(struct kobj * p)
{
    int prev;

    while ((prev = atomic_read(&p->ko_refcount)) > 0 &&
           atomic_test_and_set(&p->ko_fast_lock) == 1);
    if (prev <= 0)
        return -EIDRM;

    return 0;
}

static void kobj_fast_unlock(struct kobj * p)
{
    atomic_set(&p->ko_fast_lock, 0);
}

int kobj_refcnt(struct kobj * p)
{
    return atomic_read(&p->ko_refcount);
}

int kobj_ref(struct kobj * p)
{
    int prev;

    if (kobj_fast_lock(p))
        return -EIDRM;

    prev = atomic_inc(&p->ko_refcount);
    if (prev <= 0) {
        atomic_set(&p->ko_refcount, -1);
        return -EIDRM;
    }

    kobj_fast_unlock(p);

    return 0;
}

void kobj_unref(struct kobj * p)
{
    int prev;

    if (kobj_fast_lock(p))
        return;

    prev = atomic_dec(&p->ko_refcount);
    if (prev == 1) {
        atomic_set(&p->ko_refcount, -1);
        prev = atomic_read(&p->ko_refcount);
        p->ko_free(p);
    } else {
        kobj_fast_unlock(p);
    }
}
