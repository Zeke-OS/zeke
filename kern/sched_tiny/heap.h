/**
 *******************************************************************************
 * @file    heap.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for heap data structure for threadInfo_t (heap.c).
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <tsched.h>

#define HEAP_NEW_EMPTY {{NULL}, -1}

/** Heap data struct */
typedef struct {
    threadInfo_t * a[configSCHED_MAX_THREADS];  /*!< Heap array */
    int size;                                   /*!< Current heap size */
} heap_t;

/**
 * Removes the thread on top of a heap
 * @param heap a pointer to the heap_t struct.
 */
void heap_del_max(heap_t * heap);

void heap_insert(heap_t * heap, threadInfo_t * k);

/**
 * Heap increment key
 * @note Parameters are not asserted. If key is not actually biger than it
 * previously was this operation might not work as expected.
 * @param heap a pointer to the heap_t struct.
 * @param i Index of the changed key in heap array.
 */
void heap_inc_key(heap_t * heap, int i);

/**
 * Heap decrement key
 * @note Parameters are not asserted. If key is not actually smaller than it
 * previously was this operation might not work as expected.
 * @param heap Pointer to a heap_t struct.
 * @param i Index of the changed key in heap array.
 */
void heap_dec_key(heap_t * heap, int i);

/**
 * Reschedule root to some other level with a given priority.
 * @param heap a pointer to the heap_t struct.
 * @param pri the new priority.
 */
void heap_reschedule_root(heap_t * heap, int pri);

/**
 * Find thread from a heap array
 * @param heap a pointer to the heap_t struct.
 * @param thread_id thread id.
 * @return Id of the thread in heap array if thread was found; Otherwise -1.
 */
int heap_find(heap_t * heap, pthread_t thread_id);

#endif /* HEAP_H */
