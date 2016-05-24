/**
 *******************************************************************************
 * @file    queue_r.h
 * @author  Olli Vanhoja
 * @brief   Thread-safe queue
 * @section LICENSE
 * Copyright (c) 2013, 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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
 * @addtogroup libkern
 * @{
 */

/**
 * @addtogroup queue_r
 * @{
 */

#pragma once
#ifndef QUEUE_R_H
#define QUEUE_R_H

#include <stddef.h>

/**
 * Queue control block.
 */
typedef struct queue_cb {
    void * data;    /*!< Pointer to the data array used for queue. */
    size_t b_size;  /*!< Block size in bytes. */
    size_t a_len;   /*!< Array length. */
    size_t m_write; /*!< Write index. */
    size_t m_read;  /*!< Read index. */
} queue_cb_t;

#define QUEUE_INITIALIZER(data_array, block_size, array_size)   \
    (struct queue_cb){                                          \
        .data = (data_array),                                   \
        .b_size = (block_size),                                 \
        .a_len = (array_size / block_size),                     \
        .m_write = 0,                                           \
        .m_read = 0,                                            \
        }

/**
 * Create a new queue control block.
 * Initializes a new queue control block and returns it as a value.
 * @param data_array an array where the actual queue data is stored.
 * @param block_size the size of single data block/struct/data type in
 *                   data_array in bytes.
 * @param arra_size the size of the data_array in bytes.
 * @return a new queue_cb_t queue control block structure.
 */
queue_cb_t queue_create(void * data_array, size_t block_size,
                        size_t array_size);

/**
 * Push an element to the queue.
 * @param cb is a pointer to the queue control block.
 * @param element the new element to be copied.
 * @return 0 if queue is already full; otherwise operation was succeed.
 * @note element is always copied to the queue, so it is safe to remove the
 * original data after a push.
 */
int queue_push(queue_cb_t * cb, const void * element);

/**
 * Allocate an element from the queue.
 * @param cb is a pointer to the queue control block.
 * @return A pointer to the element in the queue.
 */
void * queue_alloc_get(queue_cb_t * cb);

/**
 * Commit previous allocation from the queue.
 * @note queue_push() should not be called between calls to queue_alloc_get()
 *       and queue_alloc_commit().
 */
void queue_alloc_commit(queue_cb_t * cb);

/**
 * Pop an element from the queue.
 * @param cb is a pointer to the queue control block.
 * @param element is the location where element is copied to from the queue.
 * @return 0 if queue is empty; otherwise operation was succeed.
 */
int queue_pop(queue_cb_t * cb, void * element);

/**
 * Peek an element from the queue.
 * @param cb is a pointer to the queue control block.
 * @param element is the location where element is copied to from the queue.
 * @return 0 if queue is empty; otherwise operation was succeed.
 */
int queue_peek(queue_cb_t * cb, void ** element);

/**
 * Skip n number of elements in the queue.
 * @param cb is a pointer to the queue control block.
 * @return Returns the number of elements skipped.
 */
int queue_skip(queue_cb_t * cb, size_t n);

/**
 * Clear the queue.
 * This operation is considered safe when committed from the push end thread.
 * @param cb is a pointer to the queue control block.
 */
void queue_clear_from_push_end(queue_cb_t * cb);

/**
 * Clear the queue.
 * This operation is considered safe when committed from the pop end thread.
 * @param cb is a pointer to the queue control block.
 */
void queue_clear_from_pop_end(queue_cb_t * cb);

/**
 * Check if the queue is empty.
 * @param cb is a pointer to the queue control block.
 * @return 0 if the queue is not empty.
 */
int queue_isempty(queue_cb_t * cb);

/**
 * Check if the queue is full.
 * @param cb is a pointer to the queue control block.
 * @return 0 if the queue is not full.
 */
int queue_isfull(queue_cb_t * cb);

/**
 * Seek queue.
 * @param cb is a pointer to the queue control block.
 * @param i index.
 * @param[in] element returned element.
 * @return 0 if failed; otherwise succeed.
 */
int seek(queue_cb_t * cb, size_t i, void * element);

#endif /* QUEUE_R_H */

/**
 * @}
 */

/**
 * @}
 */
