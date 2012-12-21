/**
 *******************************************************************************
 * @file    heap.c
 * @author  Olli Vanhoja
 *
 * @brief   Heap data structure.
 *
 *******************************************************************************
 */

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
    int temp = heap->a[i];
    heap->a[j] = heap->a[i];
    heap->a[i] = temp;
}

void heapify(heap_t * heap, int i)
{
    int l = left(i);
    int r = right(i);

    if (r <= heap->size) {
        int largest;

        if (heap->a[i] > heap->a[r])
            largest = l;
        else
            largest = r;
        if (heap->a[i] < heap->a[largest]) {
            swap(heap, i, largest);
            heapify(heap, largest);
        }
    } else if ((l == heap->size) && (heap->a[i] < heap->a[l])) {
        swap(heap, i, l);
    }
}

int heap_del_max(heap_t * heap)
{
    int max = heap->a[0];

    heap->a[0] = heap->a[heap->size];
    heap->size--;
    heapify(heap, 0);

    return max;
}

int heap_insert(heap_t * heap, int k)
{
    if ((heap->size + 1) == heap->max)
        return -1;

    heap->size++;
    int i = heap->size;
    while ((i > 1) && (heap->a[parent(i)] < k)) {
        heap->a[i] = heap->a[parent(i)];
        i = parent(i);
    }
    heap->a[i] = k;

    return 0;
}

void heap_inc_key(heap_t * heap, int i, int newk)
{
    if (newk > heap->a[i] && (i < heap->size)) {
        heap->a[i] = newk;
        while (i > 1 && heap->a[parent(i)] < heap->a[i]) {
            swap(heap, i, heap->a[parent(i)]);
            i = parent(i);
        }
    }
}
