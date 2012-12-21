/**
 *******************************************************************************
 * @file    heap.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for heap data structure (heap.c).
 *
 *******************************************************************************
 */

#pragma once
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

typedef struct {
    int * a;
    int size;
    size_t max;
} heap_t;

void heapify(heap_t * heap, int i);
int heap_del_max(heap_t * heap);
int heap_insert(heap_t * heap, int k);
void heap_inc_key(heap_t * heap, int i, int newk);

#endif /* HEAP_H */
