/**
 *******************************************************************************
 * @file    kmalloc.c
 * @author  Olli Vanhoja
 * @brief   Generic kernel memory allocator.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <machine/atomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/sysctl.h>
#include <dynmem.h>
#include <hal/core.h>
#include <hal/mmu.h>
#include <idle.h>
#include <kerror.h>
#include <klocks.h>
#include <kstring.h>
#include <libkern.h>
#include <queue_r.h>
#include <kmalloc.h>

/*
 * Signatures
 */
#define KM_SIGNATURE_VALID      0XBAADF00D /*!< a valid mblock entry. */
#define KM_SIGNATURE_INVALID    0xDEADF00D /*!< an invalid mblock entry. */

/**
 * kmalloc statistics strcut.
 */
struct kmalloc_stat {
    size_t kms_mem_res;     /*!< Amount of memory reserved for kmalloc. */
    size_t kms_mem_max;     /*!< Maximum amount of reserved memory. */
    size_t kms_mem_alloc;   /*!< Amount of currectly allocated memory. */
    size_t kms_mem_alloc_max; /*!< Maximum amount of allocated memory. */
};
struct kmalloc_stat kmalloc_stat;
#if 0
int fragm_ratio;
#endif

SYSCTL_DECL(_vm_kmalloc);
SYSCTL_NODE(_vm, OID_AUTO, kmalloc, CTLFLAG_RW, 0,
        "kmalloc stats");

SYSCTL_UINT(_vm_kmalloc, OID_AUTO, res, CTLFLAG_RD,
        ((unsigned int *)&(kmalloc_stat.kms_mem_res)), 0,
        "Amount of memory currently reserved for kmalloc.");
SYSCTL_UINT(_vm_kmalloc, OID_AUTO, max, CTLFLAG_RD,
        ((unsigned int *)&(kmalloc_stat.kms_mem_max)), 0,
        "Maximum peak amount of memory reserved for kmalloc.");
SYSCTL_UINT(_vm_kmalloc, OID_AUTO, alloc, CTLFLAG_RD,
        ((unsigned int *)&(kmalloc_stat.kms_mem_alloc)), 0,
        "Amount of memory currectly allocated with kmalloc.");
SYSCTL_UINT(_vm_kmalloc, OID_AUTO, alloc_max, CTLFLAG_RD,
        ((unsigned int *)&(kmalloc_stat.kms_mem_alloc_max)), 0,
        "Maximum peak amount of memory allocated with kmalloc");
#if 0
SYSCTL_INT(_vm_kmalloc, OID_AUTO, fragm_rat, CTLFLAG_RD,
        &fragm_ratio, 0, "Fragmentation percentage");
#endif

/**
 * Memory block descriptor.
 */
typedef struct mblock {
    unsigned signature;     /* Magic number for extra security. */
    size_t size;            /* Size of data area of this block. */
    struct mblock * next;   /* Pointer to the next memory block desc. */
    struct mblock * prev;   /* Pointer to the previous memory block desc. */
    atomic_t refcount;      /*!< Ref count. */
    void * ptr;             /*!< Memory block desc validatation. ptr should
                             * point to the data section of this mblock. */
    char data[];
} mblock_t;

/**
 * Size base MBLOCK entry.
 */
#define MBLOCK_SIZE (sizeof(mblock_t))

/**
 * kmalloc base address.
 */
void * kmalloc_base;

static mtx_t kmalloc_giant_lock;

/*
 * CB and data pointer array for lazy freeing data.
 * Lazy in this context means freeing data where there is no risk of deadlock.
 */
static struct queue_cb lazy_free_queue;
static uintptr_t lazy_free_queue_data[100];

/**
 * Get pointer to a memory block descriptor by memory block pointer.
 * @param p is the memory block address.
 * @return Address to the memory block descriptor of memory block p.
 */
#define get_mblock(p) ((mblock_t *)((uint8_t *)p - MBLOCK_SIZE))

/**
 * Convert MBytes to Bytes
 * @param v is a size in MBytes.
 * @return v in bytes.
 */
#define MB_TO_BYTES(v) ((v) * 1024 * 1024)

static mblock_t * extend(mblock_t * last, size_t size);
static mblock_t * find_mblock(mblock_t ** last, size_t size);
static void split_mblock(mblock_t * b, size_t s);
static mblock_t * merge(mblock_t * b);
static int valid_addr(void * p);
/* Stat functions */
static void update_stat_up(size_t * stat_act, size_t amount);
static void update_stat_down(size_t * stat_act, size_t amount);
#if 0
static void update_stat_set(size_t * stat_act, size_t value);
#endif

/*
 * This will be called before any other initializers.
 */
void kmalloc_init(void)
{
    mtx_init(&kmalloc_giant_lock, MTX_TYPE_TICKET, 0);
    lazy_free_queue = queue_create(lazy_free_queue_data, sizeof(uintptr_t),
                                   sizeof(lazy_free_queue_data));
}

/**
 * Allocate more memory for kmalloc.
 */
static mblock_t * extend(mblock_t * last, size_t s)
{
    mblock_t * b;
    size_t s_mbytes; /* Size rounded to MBytes */
    size_t memfree_b; /* Bytes free after allocating b. */

    s += MBLOCK_SIZE; /* Need some space for the header. */

    s_mbytes = s >> 20; /* Full Mbytes. */
    s_mbytes += (0xfffff & s) ? 1 : 0; /* Add one MB if there is any extra. */

    b = dynmem_alloc_region(s_mbytes, MMU_AP_RWNA, MMU_CTRL_MEMTYPE_WB);
    if (!b) {
#if configDEBUG >= KERROR_WARN
        KERROR(KERROR_DEBUG, "dynmem returned null.\n");
#endif
        goto out;
    }

    update_stat_up(&(kmalloc_stat.kms_mem_res), MB_TO_BYTES(s_mbytes));

    /*
     * First mblock in the newly allocated section.
     * Data section of the block will be returned by kmalloc.
     */
    b->size = s - MBLOCK_SIZE;
    b->next = NULL;
    b->prev = last;
    if (last)
        last->next = b;
    b->signature = KM_SIGNATURE_VALID;
    b->ptr = b->data;
    b->refcount = ATOMIC_INIT(0);

    /*
     * If there is still space left in the new region it has to be
     * marked as a block now.
     */
    memfree_b = (MB_TO_BYTES(s_mbytes)) - s;
    if (memfree_b) {
        mblock_t * bl;

        bl = (mblock_t *)((size_t)b + s); /* Get pointer to the new block. */
        bl->size = memfree_b - MBLOCK_SIZE;
        bl->signature = KM_SIGNATURE_VALID;
        bl->ptr = bl->data;
        bl->refcount = ATOMIC_INIT(0);
        bl->next = NULL;
        bl->prev = b;

        b->next = bl;
    }

out:
    return b;
}

/**
 * Find a free mblock.
 * @param[out] last is the last block in chain if no sufficient block found.
 * @param[in] size  is the minimum size of a block needed.
 * @return Returns a suffiently large block or null if not found.
 */
static mblock_t * find_mblock(mblock_t ** last, size_t size)
{
    mblock_t * b = kmalloc_base;

    do {
#ifdef configKMALLOC_DEBUG
        if (b->ptr == NULL) {
            printf("Invalid mblock: p = %p sign = %x\n", b->ptr, b->signature);
            b = NULL;
            break;
        }
#endif
        *last = b;
        if ((atomic_read(&b->refcount) == 0) && b->size >= size)
            break;
    } while ((b = b->next) != NULL);

    return b;
}

/**
 * Split a memory block in to two halves.
 * @param b is a pointer to the memory block descriptor.
 * @param s is the new size of a the block b.
 */
static void split_mblock(mblock_t * b, size_t s)
{
    mblock_t * nb = (mblock_t *)((size_t)b->data + s); /* New block. */

    nb->size = b->size - s - MBLOCK_SIZE;
    nb->next = b->next;
    nb->prev = b;
    nb->refcount = ATOMIC_INIT(0);
    nb->signature = KM_SIGNATURE_VALID;
    nb->ptr = nb->data;

    b->size = s;
    b->next = nb;

    if (nb->next)
        nb->next->prev = nb;
}

/**
 * Merge two blocks of memory.
 * @param b is the block on imaginary left side.
 * @return Pointer to a descriptor of the merged block.
 */
static mblock_t * merge(mblock_t * b)
{
    if (b->next && (atomic_read(&b->next->refcount) == 0)) {
        /*
         * Don't merge if these blocks are not in contiguous memory space.
         * If they aren't it means that they are from diffent areas of dynmem
         */
        if ((void *)((size_t)(b->data) + b->size) != (void *)(b->next)) {
            goto out;
        }

        /* Mark signature of the next block invalid. */
        b->next->signature = KM_SIGNATURE_INVALID;

        b->size += MBLOCK_SIZE + b->next->size;

        /* Update link pointers */
        b->next = b->next->next;
        if (b->next) {
            b->next->prev = b;
        }
    }
out:
    return b;
}

/**
 * Validate a given memory block address.
 * @param p is a pointer to a memory block.
 * @return Returns value other than 0 if given pointer is valid.
 */
static int valid_addr(void * p)
{
    if (!p)
        return 0;

    /* RFE what if get_mblock returns invalid address? */
    /*
     * If the base is not set it's impossible that we would have any allocated
     * blocks.
     */
    if (!kmalloc_base)
        return 0;

    /* Validation */
    return (p == (get_mblock(p)->ptr) &&
            get_mblock(p)->signature == KM_SIGNATURE_VALID);
}

void * kmalloc(size_t size)
{
    mblock_t * b;
    mblock_t * last;
    size_t s = memalign(size);

    mtx_lock(&kmalloc_giant_lock);
    if (kmalloc_base) {
        /* Find a mblock. */
        last = kmalloc_base;
        b = find_mblock(&last, s);
        if (b) {
            /* Can we split this mblock.
             * Note that b->size >= s
             */
            if ((b->size - s) >= (MBLOCK_SIZE + sizeof(void *))) {
                split_mblock(b, s);
            }
        } else {
            /* No fitting block, allocate more memory. */
            b = extend(last, s);
            if (!b) {
                mtx_unlock(&kmalloc_giant_lock);
                return NULL;
            }
        }
    } else { /* First kmalloc call or no pages allocated. */
        b = extend(NULL, s);
        if (!b) {
            mtx_unlock(&kmalloc_giant_lock);
            return NULL;
        }
        kmalloc_base = b;
    }

    update_stat_up(&(kmalloc_stat.kms_mem_alloc), b->size);

    atomic_set(&b->refcount, 1);
    mtx_unlock(&kmalloc_giant_lock);

    return b->data;
}

void * kcalloc(size_t nelem, size_t elsize)
{
    size_t * p;

    p = kmalloc(nelem * elsize);
    if (p) {
        memset(p, 0, memalign(nelem * elsize));
    }

    return p;
}

void * kzalloc(size_t size)
{
    size_t * p;

    p = kmalloc(size);
    if (p) {
        memset(p, 0, memalign(size));
    }
    return p;
}

void kfree(void * p)
{
    mblock_t * b;

    if (!valid_addr(p))
        return;

    b = get_mblock(p);
    if (atomic_read(&b->refcount) <= 0) { /* Already freed. */
        return;
    }

    atomic_dec(&b->refcount);
    if (atomic_read(&b->refcount) > 0)
        return;

    mtx_lock(&kmalloc_giant_lock);

    update_stat_down(&(kmalloc_stat.kms_mem_alloc), b->size);

    /* Try merge with previous mblock if possible. */
    if (b->prev && (atomic_read(&b->prev->refcount) == 0)) {
        b = merge(b->prev);
    }

    /* Then try merge with next. */
    if (b->next) {
        merge(b);
    } else {
        /* Free the last block. */
        if (b->prev) {
            b->prev->next = NULL;
        } else { /* All freed, no more memory allocated by kmalloc. */
            kmalloc_base = NULL;
        }

        /*
         * This should work as b should be pointing to a begining of
         * a region allocated with dynmem.
         *
         * Note: kfree is not bullet proof with non-contiguous dynmem
         * regions because it doesn't do any traversal to find older
         * allocations that are now free. Hopefully this doesn't matter
         * and it might even give some performance boost in certain
         * situations.
         */
        mtx_unlock(&kmalloc_giant_lock);
        dynmem_free_region(b);
        /* TODO Update stat */
        return;
    }

    mtx_unlock(&kmalloc_giant_lock);
}

void kfree_lazy(void * p)
{
    istate_t istate;

    /*
     * This is not a complete protection against concurrent access but we trust
     * the caller knows how this works.
     */
    istate = get_interrupt_state();
    disable_interrupt();

    if (!queue_push(&lazy_free_queue, &p)) {
        mblock_t * b = get_mblock(p);
        KERROR(KERROR_WARN, "kfree lazy queue full, leaked %u bytes\n",
               (uint32_t)b->size);
    }

    set_interrupt_state(istate);
}

/*
 * TODO We should take cpu as an argument and have this lazy free for each core.
 */
static void idle_lazy_free(uintptr_t arg)
{
    void * addr;

    /*
     * Free only one allocation per call to allow other tasks run as well.
     * Locking shouldn't be a problem since no other process should have lock
     * to our giant lock.
     */
    if (queue_pop(&lazy_free_queue, &addr)) {
        kfree(addr);
    }
}
IDLE_TASK(idle_lazy_free, 0);

void * krealloc(void * p, size_t size)
{
    size_t s; /* Aligned size. */
    mblock_t * b; /* Old block. */
    mblock_t * nb; /* New block. */
    void * np; /* Pointer to the data section of the new block. */
    void * retval = NULL;

    if (!p) {
        /* realloc is specified to call malloc(s) if p == NULL. */
        retval = kmalloc(size);
        goto out;
    }

    if (!valid_addr(p))
        return NULL;

    s = memalign(size);
    b = get_mblock(p);

    if (b->size >= s) { /* Requested to shrink. */
        if (b->size - s >= (MBLOCK_SIZE + sizeof(void *))) {
            mtx_lock(&kmalloc_giant_lock);
            split_mblock(b, s);
            mtx_unlock(&kmalloc_giant_lock);
        } /* else don't split. */
    } else { /* new size is larger. */
        /* Try to merge with next block. */
        if (b->next && (atomic_read(&b->next->refcount) == 0) &&
                ((b->size + MBLOCK_SIZE + b->next->size) >= s)) {
            size_t old_size = b->size;

            mtx_lock(&kmalloc_giant_lock);

            merge(b);
            if (b->size < s) {
                mtx_unlock(&kmalloc_giant_lock);
                /* Goto alloc_new_block if merge failed. */
                goto alloc_new_block;
            }

            /* Substract from stat */
            update_stat_down(&(kmalloc_stat.kms_mem_alloc), old_size);

            /* Split new block if it's larger than needed */
            if (b->size - s >= (MBLOCK_SIZE + sizeof(void *))) {
                split_mblock(b, s);
            }

            /* Add new size to stat */
            update_stat_up(&(kmalloc_stat.kms_mem_alloc), b->size);
            mtx_unlock(&kmalloc_giant_lock);
        } else { /* realloc with a new mblock.
                  * kmalloc & free will handle stat updates. */
alloc_new_block:
            np = kmalloc(s);
            if (!np) { /* Allocating new block failed,
                        * don't touch the old one. */
                retval = NULL;
                goto out;
            }

            nb = get_mblock(np);
            memcpy(nb->data, b->data, b->size);
            /* Free the old mblock. */
            kfree(p);
            retval = np;
            goto out;
        }
    }
    retval = p;
out:
    return retval;
}

void * kpalloc(void * p)
{
    if (valid_addr(p)) {
        atomic_inc(&(get_mblock(p)->refcount));
    }
    return p;
}

/**
 * Updates stat actual value by adding amount to it.
 * This function will also update related max value.
 * @param stat_act  is a pointer to a stat actual value.
 * @param amount    is the value to be added.
 */
static void update_stat_up(size_t * stat_act, size_t amount)
{
    size_t * stat_max = stat_act + 1;

    *stat_act += amount;
    if (*stat_act > *stat_max)
        *stat_max = *stat_act;
}

/**
 * Updates stat current value by substracting amount from it.
 * @param stat_act  is a pointer to a stat actual value.
 * @param amount    is the value to be substracted.
 */
static void update_stat_down(size_t * stat_act, size_t amount)
{
    *stat_act -= amount;
}

#if 0
/**
 * Set stat current value.
 * This function will also update related max value.
 * @param stat_act  is a pointer to a stat actual value.
 * @param value     is the to value to be set to selected stat.
 */
static void update_stat_set(size_t * stat_act, size_t value)
{
    size_t * stat_max = stat_act + sizeof(size_t);

    *stat_act = value;
    if (*stat_act > *stat_max)
        *stat_max = *stat_act;
}
#endif

/*
 * There is currently no way of making this safe and efficiently.
 */
#if 0
/**
 * kmalloc fragmentation percentage stats.
 */
static void stat_fragmentation(uintptr_t arg)
{
    mblock_t * b = kmalloc_base;
    int blocks_free = 0;
    int blocks_total = 0;

    if (!b)
        return;

    do {
        if (atomic_read(&b->refcount) == 0) {
            blocks_free++;
        }
        blocks_total++;
    } while ((b = b->next));

    fragm_ratio = (blocks_free * 100) / blocks_total;
}
IDLE_TASK(stat_fragmentation, 0);
#endif
