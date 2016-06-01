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

struct mempool * mempool_init(enum mempool_type type, size_t bsize,
                              unsigned count)
{
    struct mempool * mp;
    uint8_t * elem;
    const size_t  data_bsize = count * bsize;
    const size_t pool_bsize = count * sizeof(void *);

    mp = kzalloc(sizeof(struct mempool) + pool_bsize);
    elem = kzalloc(data_bsize);
    if (!(mp && elem))
        return NULL;

    mp->bsize = bsize;
    mp->head = queue_create(&mp->pool, sizeof(void *), pool_bsize);

    mtx_init(&mp->lock, MTX_TYPE_TICKET, 0);
    if (type == MEMPOOL_TYPE_BLOCKING) {
        sema_init(&mp->sema, count);
    }

    mp->data = elem;
    uint8_t * data_end = elem + data_bsize;
    do {
        queue_push(&mp->head, &elem);
        elem += bsize;
    } while (elem != data_end);

    return mp;
}

void mempool_destroy(struct mempool ** mp)
{
    struct mempool * p = *mp;

    /* No need to lock */
    kfree(p->data);
    kfree(p);
    *mp = NULL;
}

void * mempool_get(struct mempool * mp)
{
    void * elem = NULL;

    if (mp->type == MEMPOOL_TYPE_BLOCKING)
        sema_down(&mp->sema);

    mtx_lock(&mp->lock);
    (void)queue_pop(&mp->head, &elem);
    mtx_unlock(&mp->lock);

    return elem;
}

void mempool_return(struct mempool * mp, void * p)
{
    mtx_lock(&mp->lock);
    (void)queue_push(&mp->head, &p);
    mtx_unlock(&mp->lock);
    if (mp->type == MEMPOOL_TYPE_BLOCKING)
        sema_up(&mp->sema);
}
