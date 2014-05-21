/**
 *******************************************************************************
 * @file    heap.c
 * @author  Olli Vanhoja
 *
 * @brief   Heap data structure for threadInfo_t.
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

#include <sched.h>
#include "heap.h"

#if configDEBUG != 0
#define HEAP_BOUNDS_CHECK 1
#endif

static inline void swap(heap_t * heap, int i, int j);
static void heapify(heap_t * heap, int i);

/**
 * Returns index to the parent of the key i
 * @param i Index to a key.
 */
#define parent(i) (i / 2)

/**
 * Returns index to the key on the left side of the key i
 * @param i Index to a key.
 */
#define left(i) (2 * i)

/**
 * Returns index to the key on the right side of the key i
 * @param i Index to a key.
 */
#define right(i) (2 * i + 1)

/**
  * Swap two threadInfo_t pointers in a heap
  * @param heap Pointer to a heap struct.
  * @param i Index of a pointer in heap array.
  * @param j Index of a pointer in heap array.
  */
static inline void swap(heap_t * heap, int i, int j)
{
    threadInfo_t * temp = (threadInfo_t *)(heap->a[i]);
    heap->a[i] = heap->a[j];
    heap->a[j] = temp;
}

/**
  * Fix heap
  * @param heap Pointer to a heap struct.
  * @param i Current index in heap array.
  */
static void heapify(heap_t * heap, int i)
{
    int l = left(i);
    int r = right(i);

    if (r <= heap->size) {
        int largest;

        if (heap->a[i]->priority > heap->a[r]->priority)
            largest = l;
        else
            largest = r;

        if (heap->a[i]->priority < heap->a[largest]->priority) {
            swap(heap, i, largest);
            heapify(heap, largest);
        }
    } else if ((l == heap->size) && (heap->a[i]->priority < heap->a[l]->priority)) {
        swap(heap, i, l);
    }
}

void heap_del_max(heap_t * heap)
{
#ifdef HEAP_BOUNDS_CHECK
    if (heap->size <= 0) {
        while (1); /* Catch */
    }
#endif

    heap->a[0] = heap->a[heap->size];
    heap->size--;
    heapify(heap, 0);
}

void heap_insert(heap_t * heap, threadInfo_t * k)
{
    int i;

    heap->size++;

#ifdef HEAP_BOUNDS_CHECK
    if (heap->size > configSCHED_MAX_THREADS) {
        while (1); /* Catch */
    }
#endif

    i = heap->size;
    while ((i > 0) && (heap->a[parent(i)]->priority < k->priority)) {
        heap->a[i] = heap->a[parent(i)];
        i = parent(i);
    }
    heap->a[i] = k;
}

void heap_inc_key(heap_t * heap, int i)
{
    while ((i > 0) && ((heap->a[parent(i)]->priority) < heap->a[i]->priority)) {
        swap(heap, i, parent(i));
        i = parent(i);
    }
}

void heap_dec_key(heap_t * heap, int i)
{
    /* Only heapify is actually needed priority is already set
     * to its new value */
    heapify(heap, i);
}

void heap_reschedule_root(heap_t * heap, osPriority pri)
{
    int s = heap->size;

    /* Swap with the last one */
    heap->a[0]->priority = (int)osPriorityIdle - 1;
    swap(heap, 0, s);
    heapify(heap, 0);

    /* Move upwards */
    heap->a[s]->priority = pri;
    while ((s > 0) && ((heap->a[parent(s)]->priority) <= heap->a[s]->priority)) {
        swap(heap, s, parent(s));
        s = parent(s);
    }

}

int heap_find(heap_t * heap, pthread_t thread_id)
{
    int i;

    for (i = 0; i < heap->size; i++) {
        if (heap->a[i]->id == thread_id)
            return i;
    }

    return -1;
}
