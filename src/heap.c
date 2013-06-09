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

#if configHEAP_BOUNDS_CHECK == 1
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

int heap_find(heap_t * heap, osThreadId thread_id)
{
    int i;

    for (i = 0; i < heap->size; i++) {
        if (heap->a[i]->id == thread_id)
            return i;
    }

    return -1;
}
