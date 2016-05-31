/**
 *******************************************************************************
 * @file    mempool.c
 * @author  Olli Vanhoja
 * @brief   A simple memory pooler.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <mempool.h>

struct mempool * mempool_init(size_t bsize, unsigned count)
{
    struct mempool * mp;
    const size_t pool_bsize = count * sizeof(void *);

    mp = kzalloc(sizeof(struct mempool) + pool_bsize);
    if (!mp)
        return NULL;

    mp->bsize = bsize;
    mp->head = queue_create(&mp->pool, sizeof(void *), pool_bsize);
    mtx_init(&mp->lock, MTX_TYPE_TICKET, 0);

    for (unsigned i = 0; i < count; i++) {
        void * elem = kzalloc(bsize);
        if (!elem) {
            mempool_destroy(&mp);
            return NULL;
        }

        queue_push(&mp->head, &elem);
    }

    return mp;
}

void mempool_destroy(struct mempool ** mp)
{
    void * elem;

    /* No need to lock */
    while (queue_pop(&(*mp)->head, &elem)) {
        kfree(elem);
    }
    kfree(*mp);
    *mp = NULL;
}

void * mempool_get(struct mempool * mp)
{
    void * elem;
    int retval;

    mtx_lock(&mp->lock);
    retval = queue_pop(&mp->head, &elem);
    mtx_unlock(&mp->lock);

    if (retval == 0)
        elem = kzalloc(mp->bsize);

    return elem;
}

void mempool_return(struct mempool * mp, void * p)
{
    int retval = 0;

    if (mp) {
        mtx_lock(&mp->lock);
        retval = queue_push(&mp->head, &p);
        mtx_unlock(&mp->lock);
    }
    if (retval == 0) {
        kfree(p);
    }
}
