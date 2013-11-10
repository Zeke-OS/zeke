/**
 *******************************************************************************
 * @file    kmalloc.c
 * @author  Olli Vanhoja
 * @brief   Generic kernel memory allocator.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdint.h>
#include <stddef.h>
#include <kstring.h>
#ifndef PU_TEST_BUILD
#include <dynmem.h>
#endif

/**
 * Memory block descriptor.
 */
typedef struct mblock {
    size_t size;            /* Size of data area of this block. */
    struct mblock * next;   /* Pointer to the next memory block desc. */
    struct mblock * prev;   /* Pointer to the previous memory block desc. */
    int refcount;           /*!< Ref count. */
    void * ptr;         /*!< Memory block desc validatation. p should be data */
    char data[1];
} mblock_t;

#define MBLOCK_SIZE (sizeof(mblock_t) - 4)

void * kmalloc_base = 0;

#define align4(x) (((((x)-1)>>2)<<2)+4)

static mblock_t * extend(mblock_t * last, size_t size);
static mblock_t * get_mblock(void * p);
static mblock_t * find_mblock(mblock_t ** last, size_t size);
static void split_mblock(mblock_t * b, size_t s);
static mblock_t * merge(mblock_t * b);
static int valid_addr(void * p);


/**
 * Allocate more memory for kmalloc.
 */
static mblock_t * extend(mblock_t * last, size_t s)
{
    mblock_t * b;
    mblock_t * bl;
    size_t s_m; /* Size floored to MBytes */
    size_t memfree_b; /* Bytes free after allocating b. */

    s += MBLOCK_SIZE; /* Need some space for the header. */

    s_m = s >> 20; /* Full Mbytes. */
    s_m += (0xfffff & s) ? 1 : 0; /* Add one MB if there is any extra. */

#ifndef PU_TEST_BUILD
    b = dynmem_alloc_region(s_m, MMU_AP_RWNA, MMU_CTRL_NG);
#endif

    if (!b) return 0;

    /* First mblock in the newly allocated section.
     * Data section of block will be returned by kmalloc.
     */
    b->size = s - MBLOCK_SIZE;
    b->next = 0;
    b->prev = last;
    if (last) {
        last->next = b;
    }
    b->ptr = b->data;
    b->refcount = 1;

    /* If there is still space left in the new region it has to be
     * mapped now. */
    memfree_b = (1024*1024 * s_m) - s;
    if (memfree_b) {
        bl = (mblock_t *)((size_t)b + s);
        bl->size = memfree_b;
        bl->next = 0;
        bl->ptr = bl->data;
        bl->refcount = 0;
        bl->prev = b;
        b->next = bl;
    }

    return b;
}

/**
 * Get pointer to a memory block descriptor by memory block pointer.
 * @param p is the memory block address.
 * @areturn Address to the memory block descriptor of memory block p.
 */
static mblock_t * get_mblock(void * p)
{
    return (p - MBLOCK_SIZE);
}

static mblock_t * find_mblock(mblock_t ** last, size_t size)
{
    mblock_t * b = kmalloc_base;

    while (b && !((b->refcount == 0) && b->size >= size)) {
        *last = b;
        b = b->next;
    }

    return b;
}

/**
 * Split a memory block in to two halves.
 * @param b is a pointer to the memory block descriptor.
 * @param s is the size of a new block of left side.
 */
static void split_mblock(mblock_t * b, size_t s)
{
    mblock_t * nb = (mblock_t *)(b->data + s);

    nb->size = b->size - s - MBLOCK_SIZE;
    nb->next = b->next;
    nb->prev = b;
    nb->refcount = 0;
    nb->ptr = nb->data;

    b->size = s;
    b->next = nb;

    if (nb->next) {
        nb->next->prev = nb;
    }
}

/**
 * Merge two blocks of memory.
 * @param b is the block on imaginary left side.
 * @return Pointer to a descriptor of the merged block.
 */
static mblock_t * merge(mblock_t * b) {
    if (b->next && (b->next->refcount == 0)) {
        /* Don't merge if these blocks are not in contiguous memory space.
         * If they aren't it means that they are from diffent areas of dynmem */
        if ((void *)(b->next->data) != (void *)((size_t)(b->data) + b->size))
            goto out;

        b->size += MBLOCK_SIZE + b->next->size;
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
 * @return value other than 0 if given pointer is valid.
 */
static int valid_addr(void * p)
{
    int retval = 0;

    if (kmalloc_base) { /* If base is not set it's impossible that we would have
                         * any allocated blocks. */
        retval = (p == (get_mblock(p)->ptr)); /* Validation. */
    }

    return retval;
}

/**
 * Allocate memory block
 * @param size is the size of memory block in bytes.
 * @return A pointer to the memory block allocated; 0 if failed to allocate.
 */
void * kmalloc(size_t size)
{
    mblock_t * b;
    mblock_t * last;
    size_t s = align4(size);

    if (kmalloc_base) {
        /* Find a mblock */
        last = kmalloc_base;
        b = find_mblock(&last, s);

        if (b) {
            /* Can we split this mblock */
            if ((b->size - s) >= (MBLOCK_SIZE + 4)) {
                split_mblock(b, s);
            }
            b->refcount = 1;
        } else {
            /* No fitting block, allocate more memory */
            b = extend(last, s);
            if (!b) {
                return 0;
            }
        }
    } else { /* First kmalloc call or no pages allocated */
        b = extend(0, s);
        if (!b) {
            return 0;
        }
        kmalloc_base = b;
    }

    return b->data;
}

/**
 * Allocate and zero-intialize array
 * @param nelem is the number of elements to allocate.
 * @param size is the of each element.
 * @return A pointer to the memory block allocted; 0 if failed to allocate.
 */
void * kcalloc(size_t nelem, size_t elsize)
{
    size_t * p;
    size_t s4;

    p = kmalloc(nelem * elsize);
    if (p) {
        s4 = align4(nelem * elsize);
        memset(p, 0, s4);
    }

    return p;
}

/**
 * Deallocate memory block
 *
 * Deallocates a memory block previously allocted with kmalloc, kcalloc or
 * krealloc.
 * @param p is a pointer to a previously allocated memory block.
 */
void kfree(void * p)
{
    mblock_t * b;

    if (valid_addr(p)) {
        b = get_mblock(p);
        if (b->refcount <= 0) {
            b->refcount = 0;
            return;
        }
        b->refcount--;
        if (b->refcount > 0)
            return;

        /* Try merge with previous mblock if possible. */
        if (b->prev &&  (b->prev->refcount == 0)) {
            b = merge(b->prev);
        }
        /* Then try merge with next. */
        if (b->next) {
            merge(b);
        } else {
            /* Free the last block. */
            if (b->prev)
                b->prev->next = 0;
            else /* All freed, no more memory allocated by kmalloc. */
                kmalloc_base = 0;
            /* This should work as b should be pointing to a begining of
             * a region allocated with dynmem.
             *
             * Note: kfree is not bullet proof with non-contiguous dynmem
             * regions because it doesn't do any traversal to find older
             * allocations that are now free. Hopefully this doesn't matter
             * and it might even give some performance boost in certain
             * situations. */
            dynmem_free_region(b);
        }
    }
}

/**
 * Reallocate memory block
 *
 * Changes the size of the memory block pointed to by p.
 * @note This function behaves like C99 realloc.
 * @param p is a pointer to a memory block previously allocated with kmalloc,
 *          kcalloc or krealloc.
 * @param size is the new size for the memory block, in bytes.
 * @return  A pointer to the reallocated memory block, which may be either the
 *          same as p or a new location. 0 indicates that the function failed to
 *          allocate memory, and p was not modified.
 */
void * krealloc(void * p, size_t size)
{
    size_t s;
    mblock_t * b;
    mblock_t * nb;
    void * np;
    void * retval = 0;

    if(!p) {
        /* realloc is specified to call malloc(s) if p == 0. */
        retval = kmalloc(size);
        goto out;
    }

    if (valid_addr(p)) {
        s = align4(size);
        b = get_mblock(p);

        if (b->size >= s) {
            if (b->size - s >= (MBLOCK_SIZE + 4)) {
                split_mblock(b, s);
            }
        } else {
            /* Try merge with next block */
            if (b->next && (b->next->refcount == 0) &&
                    ((b->size + MBLOCK_SIZE + b->next->size) >= s)) {
                merge(b);
                if (b->size - s >= (MBLOCK_SIZE + 4)) {
                    split_mblock(b, s);
                }
            } else { /* realloc with a new mblock. */
                np = kmalloc(s);
                if (!np) {
                    retval = 0;
                    goto out;
                }

                nb = get_mblock(np);
                memcpy(b->data, nb->data, b->size);
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

/**
 * Memory block reference
 *
 * Pass kmalloc'd pointer to a block to somewhere else and increment refcount.
 * @param p is a pointer to the kmalloc'd block of data.
 */
void * kpalloc(void * p)
{
    get_mblock(p)->refcount += 1;
    return p;
}
