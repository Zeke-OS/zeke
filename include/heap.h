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

typedef struct {
    threadInfo_t * a[configSCHED_MAX_THREADS];
    int size;
} heap_t;

void heapify(heap_t * heap, int i);
threadInfo_t * heap_del_max(heap_t * heap);
int heap_insert(heap_t * heap, threadInfo_t * k);

#endif /* HEAP_H */
