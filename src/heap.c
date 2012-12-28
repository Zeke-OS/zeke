/**
 *******************************************************************************
 * @file    heap.c
 * @author  Olli Vanhoja
 *
 * @brief   Heap data structure for threadInfo_t.
 *
 *******************************************************************************
 */

#include "sched.h"
#include "heap.h"

static int parent(int i);
static int left(int i);
static int right(int i);
static inline void swap(heap_t * heap, int i, int j);

static int parent(int i)
{
    return i / 2;
}

static int left(int i)
{
    return 2 * i;
}

static int right(int i)
{
    return 2 * i + 1;
}

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
void heapify(heap_t * heap, int i)
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

/** Removes the thread on top of a heap
  * @param heap Pointer to a heap_t struct.
  */
void heap_del_max(heap_t * heap)
{
    if (heap->size <= 0) {
        while (1); /* Catch */
    }
    heap->a[0] = heap->a[heap->size];
    heap->size--;
    heapify(heap, 0);
}

void heap_insert(heap_t * heap, threadInfo_t * k)
{
    heap->size++;
    if (heap->size > configSCHED_MAX_THREADS) {
        while (1); /* Catch */
    }
    int i = heap->size;
    while ((i > 0) && (heap->a[parent(i)]->priority < k->priority)) {
        heap->a[i] = heap->a[parent(i)];
        i = parent(i);
    }
    heap->a[i] = k;
}

/**
  * Heap increment key
  * @note Parameters are not asserted. If key is not actually biger than it
  * previously was this operation might not work as expected.
  * @param heap Pointer to a heap_t struct.
  * @param i Index of the changed key in heap array.
  */
void heap_inc_key(heap_t * heap, int i)
{
    while ((i > 0 && (heap->a[parent(i)]->priority) < heap->a[i]->priority)) {
        swap(heap, i, parent(i));
        i = parent(i);
    }
}

/**
  * Heap decrement key
  * @note Parameters are not asserted. If key is not actually smaller than it
  * previously was this operation might not work as expected.
  * @param heap Pointer to a heap_t struct.
  * @param i Index of the changed key in heap array.
  */
void heap_dec_key(heap_t * heap, int i)
{
    /* Only heapify is actually needed priority is already set
     * to its new value */
    heapify(heap, i);
}


