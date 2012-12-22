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

static inline void swap(heap_t * heap, int i, int j)
{
    threadInfo_t * temp = (threadInfo_t *)(heap->a[i]);
    heap->a[i] = heap->a[j];
    heap->a[j] = temp;
}

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

threadInfo_t * heap_del_max(heap_t * heap)
{
    threadInfo_t * max = *heap->a;

    heap->a[0] = heap->a[heap->size];
    heap->size--;
    heapify(heap, 0);

    return max;
}

int heap_insert(heap_t * heap, threadInfo_t * k)
{
    //if ((heap->size + 1) == configSCHED_MAX_THREADS)
    //    return -1;

    heap->size++;
    int i = heap->size;
    while ((i > 1) && (heap->a[parent(i)]->priority < k->priority)) {
        heap->a[i] = heap->a[parent(i)];
        i = parent(i);
    }
    heap->a[i] = k;

    return 0;
}
