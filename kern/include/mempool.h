/**
 *******************************************************************************
 * @file    mempool.h
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

/**
 * @addtogroup mempool
 * @{
 */

#pragma once
#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>
#include <klocks.h>
#include <queue_r.h>

/**
 * Mempool type.
 */
enum mempool_type {
    MEMPOOL_TYPE_NONBLOCKING,   /*!< Non-blocing mempool. */
    MEMPOOL_TYPE_BLOCKING,      /*!< Blocking mempool. */
};

/**
 * Mempool object.
 */
struct mempool {
    size_t bsize;
    struct queue_cb head;
    enum mempool_type type;
    mtx_t lock;
    sema_t sema;
    void * data;
    uint8_t pool[0];
};

/**
 * Initialize a memory pool.
 * @param bsize is the size of an element in the pool in bytes.
 * @param type selects the type of the mempool.
 * @param count is the initial number of elements in the pool.
 * @return Returns a pointer to the newly initialized memory pool.
 */
struct mempool * mempool_init(enum mempool_type type, size_t bsize,
                              unsigned count);

/**
 * Destroy a memory pool and elements returned to it.
 * mp is set to NULL after it's destroyed.
 * @param mp is a pointer to the memory pool to be destroyed.
 */
void mempool_destroy(struct mempool ** mp);

/**
 * Get an elemenent from the memory pool.
 * @param mp is a pointer to the memory pool.
 */
void * mempool_get(struct mempool * mp);

/**
 * Return an element to the pool.
 * @param mp is a pointer to the memory pool.
 * @param p is a pointer to the element to be returned.
 */
void mempool_return(struct mempool * mp, void * p);

#endif /* MEMPOOL_H */

/**
 * @}
 */
