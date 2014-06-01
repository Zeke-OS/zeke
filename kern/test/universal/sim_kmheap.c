#include <string.h>
#include "../../src/kmalloc.h"
#include "sim_kmheap.h"

/**
 * Small simulated heap mainly for kmalloc testing.
 */
simheap_t simheap;

/**
 * Setup kmalloc heap simulation.
 */
void setup_kmalloc()
{
    simheap_t * p = &simheap;
    mblock_t * ma = (mblock_t *)(p->a);
    mblock_t * mb = (mblock_t *)(p->b);
    mblock_t * mc = (mblock_t *)(p->c);

    ma->size = sizeof(simheap.a) - MBLOCK_SIZE;
    ma->prev = 0;
    ma->next = 0;
    ma->signature = 0XBAADF00D;
    ma->ptr = ma->data;
    ma->refcount = 0;

    mb->size = sizeof(simheap.b) - MBLOCK_SIZE;
    mb->prev = 0;
    mb->next = 0;
    mb->signature = 0XBAADF00D;
    mb->ptr = mb->data;
    mb->refcount = 0;

    mc->size = sizeof(simheap.c) - MBLOCK_SIZE;
    mc->prev = 0;
    mc->next = 0;
    mc->signature = 0XBAADF00D;
    mc->ptr = mc->data;
    mc->refcount = 0;

    kmalloc_base = (void *)(simheap.a);
}

/**
 * Teardown kmalloc heap simulation.
 * @note This will leak a everything that was malloc'd.
 */
void teardown_kmalloc()
{
    memset(&simheap, 0, sizeof(simheap_t));
}

