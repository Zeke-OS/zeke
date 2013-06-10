/**
 *******************************************************************************
 * @file    heap.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for heap data structure for threadInfo_t (heap.c).
 *
 *******************************************************************************
 */

#pragma once
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include "sched.h"

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
void heap_reschedule_root(heap_t * heap, osPriority pri);

/**
 * Find thread from a heap array
 * @param heap a pointer to the heap_t struct.
 * @param thread_id thread id.
 * @return Id of the thread in heap array if thread was found; Otherwise -1.
 */
int heap_find(heap_t * heap, osThreadId thread_id);

#endif /* HEAP_H */
