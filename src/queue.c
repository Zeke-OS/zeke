/**
 *******************************************************************************
 * @file    queue.c
 * @author  Olli Vanhoja
 * @brief   Thread-safe queue
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <stdint.h>
#include "queue.h"

queue_cb_t queue_create(void * data_array, size_t block_size, size_t array_size)
{
    queue_cb_t cb = {
        .data = data_array,
        .b_size = block_size,
        .a_len = array_size / block_size,
        .m_read = 0,
        .m_write = 0
    };

    return cb;
}

int queue_push(queue_cb_t * cb, void * element)
{
    int nextElement = (cb->m_write + 1) % cb->a_len;
    int b_size = cb->b_size;

    /* Check that queue is not full */
    if(nextElement == cb->m_read)
        return 0;

    /* Copy element to the data array */
    memcpy(&((uint8_t *)(cb->data))[cb->m_write * b_size], element, b_size);

    cb->m_write = nextElement;
    return 1;
}

int queue_pop(queue_cb_t * cb, void * element)
{
    int read = cb->m_read;
    int write = cb->m_write;
    int b_size = cb->b_size;
    int nextElement;

    /* Check that queue is not empty */
    if(read == write)
        return 0;

    nextElement = (read + 1) % cb->a_len;

    /* Copy from data array to the element */
    memcpy(element, &((uint8_t *)(cb->data))[read * b_size], b_size);

    cb->m_read = nextElement;
    return 1;
}

void queue_clearFromPushEnd(queue_cb_t * cb)
{
    cb->m_write = cb->m_read;
}

void queue_clearFromPopEnd(queue_cb_t * cb)
{
    cb->m_read = cb->m_write;
}

int queue_isEmpty(queue_cb_t * cb)
{
    return cb->m_write == cb->m_read;
}

int queue_isFull(queue_cb_t * cb)
{
    return ((cb->m_write + 1) % cb->a_len) == cb->m_read;
}

int seek(queue_cb_t * cb, int i, void * element)
{
    int read = cb->m_read;
    int write = cb->m_write;

    /* Check that queue is not empty */
    if((read % cb->a_len) == write)
        return 0;

    int element_i = (read + i) % cb->a_len;

    /* Check that we don't hit write position */
    if(element_i == write)
        return 0;

    /* Copy from data array to the element */
    memcpy(element, &((uint8_t *)(cb->data))[element_i * cb->b_size], cb->b_size);

    return 1;
}
