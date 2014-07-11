/**
 *******************************************************************************
 * @file    kmalloc.c
 * @author  Olli Vanhoja
 * @brief   Generic kernel memory allocator.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL
#include <stdint.h>
#include <stddef.h>
#include <libkern.h>
#include <kstring.h>
#include <kerror.h>
#include <sys/sysctl.h>
#include <dynmem.h>

#define KM_SIGNATURE_VALID      0XBAADF00D /*!< Signature for valid mblock
                                            *   entry. */
#define KM_SIGNATURE_INVALID    0xDEADF00D /*!< Signature for invalid mblock
                                            *   entry. */

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
int fragm_ratio;

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
SYSCTL_INT(_vm_kmalloc, OID_AUTO, fragm_rat, CTLFLAG_RD,
        &fragm_ratio, 0, "Fragmentation percentage");

/**
 * Memory block descriptor.
 */
typedef struct mblock {
    int signature;          /* Magic number for extra security. */
    size_t size;            /* Size of data area of this block. */
    struct mblock * next;   /* Pointer to the next memory block desc. */
    struct mblock * prev;   /* Pointer to the previous memory block desc. */
    int refcount;           /*!< Ref count. */
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

    b = dynmem_alloc_region(s_mbytes, MMU_AP_RWNA, MMU_CTRL_NG);
    if (b == 0) {
#if configDEBUG >= KERROR_WARN
        KERROR(KERROR_DEBUG, "dynmem returned null.");
#endif
        goto out;
    }

    update_stat_up(&(kmalloc_stat.kms_mem_res), MB_TO_BYTES(s_mbytes));

    /* First mblock in the newly allocated section.
     * Data section of the block will be returned by kmalloc.
     */
    b->size = s - MBLOCK_SIZE;
    b->next = 0;
    b->prev = last;
    if (last)
        last->next = b;
    b->signature = KM_SIGNATURE_VALID;
    b->ptr = b->data;
    b->refcount = 0;

    /* If there is still space left in the new region it has to be
     * marked as a block now. */
    memfree_b = (MB_TO_BYTES(s_mbytes)) - s;
    if (memfree_b) {
        mblock_t * bl;

        bl = (mblock_t *)((size_t)b + s); /* Get pointer to the new block. */
        bl->size = memfree_b - MBLOCK_SIZE;
        bl->signature = KM_SIGNATURE_VALID;
        bl->ptr = bl->data;
        bl->refcount = 0;
        bl->next = 0;
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
#if 0
        if (b->ptr == 0) {
            printf("Invalid mblock: p = %p sign = %x\n", b->ptr, b->signature);
            b = 0;
            break;
        }
#endif
        *last = b;
        if ((b->refcount == 0) && b->size >= size)
            break;
    } while ((b = b->next) != 0);

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
    nb->refcount = 0;
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
    if (b->next && (b->next->refcount == 0)) {
        /* Don't merge if these blocks are not in contiguous memory space.
         * If they aren't it means that they are from diffent areas of dynmem */
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
#define PRINT_VALID 0
#define ADDR_VALIDATION (p == (get_mblock(p)->ptr) \
        && get_mblock(p)->signature == KM_SIGNATURE_VALID)
    int retval = 0;

    /* TODO what if get_mblock returns invalid address? */
    if (kmalloc_base) { /* If base is not set it's impossible that we would have
                         * any allocated blocks. */
#if PRINT_VALID != 0
        if ((retval = ADDR_VALIDATION)) { /* Validation. */
            printf("VALID\n");
        } else {
            printf("INVALID %p\n", p);
        }
#else
        retval = ADDR_VALIDATION; /* Validation. */
#endif
    }

    return retval;
#undef ADDR_VALIDATION
#undef PRINT_VALID
}

void * kmalloc(size_t size)
{
    mblock_t * b;
    mblock_t * last;
    size_t s = memalign(size);

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
            if (!b)
                return 0;
        }
    } else { /* First kmalloc call or no pages allocated. */
        b = extend(0, s);
        if (!b)
            return 0;
        kmalloc_base = b;
    }

    update_stat_up(&(kmalloc_stat.kms_mem_alloc), b->size);

    b->refcount = 1;
    return b->data;
}

void * kcalloc(size_t nelem, size_t elsize)
{
    size_t * p;
    size_t s4;

    p = kmalloc(nelem * elsize);
    if (p) {
        s4 = memalign(nelem * elsize);
        memset(p, 0, s4);
    }

    return p;
}

void kfree(void * p)
{
    mblock_t * b;

    if (valid_addr(p)) {
        b = get_mblock(p);
        if (b->refcount <= 0) { /* Already freed. */
            b->refcount = 0;
            return;
        }

        b->refcount--;
        if (b->refcount > 0)
            return;

        update_stat_down(&(kmalloc_stat.kms_mem_alloc), b->size);

        /* Try merge with previous mblock if possible. */
        if (b->prev && (b->prev->refcount == 0)) {
            b = merge(b->prev);
        }

        /* Then try merge with next. */
        if (b->next) {
            merge(b);
        } else {
            /* Free the last block. */
            if (b->prev) {
                b->prev->next = 0;
            } else { /* All freed, no more memory allocated by kmalloc. */
                kmalloc_base = 0;
            }

            /* This should work as b should be pointing to a begining of
             * a region allocated with dynmem.
             *
             * Note: kfree is not bullet proof with non-contiguous dynmem
             * regions because it doesn't do any traversal to find older
             * allocations that are now free. Hopefully this doesn't matter
             * and it might even give some performance boost in certain
             * situations. */
            dynmem_free_region(b);
            /* TODO Update stat */
        }
    }
}

void * krealloc(void * p, size_t size)
{
    size_t s; /* Aligned size. */
    mblock_t * b; /* Old block. */
    mblock_t * nb; /* New block. */
    void * np; /* Pointer to the data section of the new block. */
    void * retval = 0;

    if (!p) {
        /* realloc is specified to call malloc(s) if p == 0. */
        retval = kmalloc(size);
        goto out;
    }

    if (valid_addr(p)) {
        s = memalign(size);
        b = get_mblock(p);

        if (b->size >= s) { /* Requested to shrink. */
            if (b->size - s >= (MBLOCK_SIZE + sizeof(void *))) {
                split_mblock(b, s);
            } /* else don't split. */
        } else { /* new size is larger. */
            /* Try to merge with next block. */
            if (b->next && (b->next->refcount == 0) &&
                    ((b->size + MBLOCK_SIZE + b->next->size) >= s)) {
                size_t old_size = b->size;

                merge(b);
                if (b->size < s) {
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
            } else { /* realloc with a new mblock.
                      * kmalloc & free will handle stat updates. */
alloc_new_block:
                np = kmalloc(s);
                if (!np) { /* Allocating new block failed,
                            * don't touch the old one. */
                    retval = 0;
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
    }
out:
    return retval;
}

void * kpalloc(void * p)
{
    if (valid_addr(p)) {
        get_mblock(p)->refcount += 1;
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

/**
 * kmalloc fragmentation percentage stats.
 */
static void stat_fragmentation(void)
{
    mblock_t * b = kmalloc_base;
    int blocks_free = 0;
    int blocks_total = 0;

    do {
        if (b->refcount == 0) {
            blocks_free++;
        }
        blocks_total++;
    } while ((b = b->next) != 0);

    fragm_ratio = (blocks_free * 100) / blocks_total;
}
DATA_SET(sched_idle_tasks, stat_fragmentation);
