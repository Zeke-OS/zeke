/**
 *******************************************************************************
 * @file    procfs_specinfo_pool.c
 * @author  Olli Vanhoja
 * @brief   Pooling for per process procfs specinfo structs.
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

#include <fs/procfs.h>
#include <kmalloc.h>
#include <queue_r.h>
#include <klocks.h>
#include "procfs_specinfo_pool.h"

#define SPECINFO_POOL_SIZE (configMAXPROC / 2)

static struct procfs_info * pool;
static struct queue_cb head;
static mtx_t pool_lock = MTX_INITIALIZER(MTX_TYPE_TICKET, 0);

void procfs_specinfo_pool_init(void)
{
    const size_t pool_bsize = SPECINFO_POOL_SIZE * sizeof(struct procfs_info *);

    pool = kzalloc_crit(pool_bsize);
    head = queue_create(pool, sizeof(struct procfs_info *), pool_bsize);

    for (int i = 0; i < SPECINFO_POOL_SIZE; i++) {
        struct procfs_info * info;

        info = kzalloc(sizeof(struct procfs_info));
        if (!info)
            return;

        queue_push(&head, &info);
    }
}

struct procfs_info * procfs_specinfo_pool_get(void)
{
    struct procfs_info * info;
    int retval;

    mtx_lock(&pool_lock);
    retval = queue_pop(&head, &info);
    mtx_unlock(&pool_lock);

    if (retval == 0)
        info = kzalloc(sizeof(struct procfs_info));

    return info;
}

void procfs_specinfo_pool_return(struct procfs_info * info)
{
    int retval;

    mtx_lock(&pool_lock);
    retval = queue_push(&head, &info);
    mtx_unlock(&pool_lock);
    if (retval == 0) {
        kfree(info);
    }
}
