#pragma once
#ifndef SIM_HEAP_H
#define SIM_HEAP_H

typedef struct mblock {
    int signature;          /* Magic number for extra security. */
    size_t size;            /* Size of data area of this block. */
    struct mblock * next;   /* Pointer to the next memory block desc. */
    struct mblock * prev;   /* Pointer to the previous memory block desc. */
    int refcount;           /*!< Ref count. */
    void * ptr;             /*!< Memory block desc validatation. ptr should
                             * point to the data section of this mblock. */
    char data[1];
} mblock_t;
#define MBLOCK_SIZE (sizeof(mblock_t) - sizeof(void *))
extern void * kmalloc_base;

typedef struct {
    char a[1024];
    char b[1024];
    char c[1024];
} simheap_t;
extern simheap_t simheap;

#define block(x, offset) ((mblock_t *)(((size_t)simheap.x) + offset))

void setup_kmalloc();
void teardown_kmalloc();

#endif /* SIM_HEAP_H */
