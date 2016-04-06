/**
 *******************************************************************************
 * @file    kmalloc.h
 * @author  Olli Vanhoja
 * @brief   Generic kernel memory allocator.
 * @section LICENSE
 * Copyright (c) 2013, 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup kmalloc
 * Malloc for kernel's internal use.
 * Kmalloc should be used for in-kernel dynamic memory allocations that doesn't
 * need to be directly accessible from used space at any point. This includes
 * eg. process control blocks, file system control blocks and cached data,
 * thread control etc.
 * @}
 */

#pragma once
#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

/**
 * Automatically free a kmalloc'd buffer when a pointer variable to the buffer
 * goes out of scope.
 * This is used as an attribute for a pointer variable.
 */
#define kmalloc_autofree \
    __attribute__((cleanup(_kfree_cleanup)))

/**
 * Allocate a memory block.
 * @param size is the size of the new memory block in bytes.
 * @return A pointer to the memory block allocated; NULL if failed to allocate.
 */
void * kmalloc(size_t size);

/**
 * Allocate and zero-intialize an array.
 * @param nelem is the number of elements to allocate.
 * @param size  is the of each element.
 * @return A pointer to the memory block allocted; NULL if failed to allocate.
 */
void * kcalloc(size_t nelem, size_t elsize);

/**
 * Allocate and zero-initialize a block.
 * @param size is the size of the new memory block in bytes.
 * @return A pointer to the memory block allocated; NULL if failed to allocate.
 */
void * kzalloc(size_t size);

/**
 * Deallocate a memory block.
 * Deallocates a memory block previously allocted with kmalloc, kcalloc or
 * krealloc.
 * @param p is a pointer to a previously allocated memory block.
 */
void kfree(void * p);

/**
 * Deallocate a memory block when idling.
 * This function is mainly useful for situation where a deadlock could occur,
 * especially when a thread calling any of kmalloc functions was interrutped and
 * call to kfree() must be done in interrupt handler (or scheduler).
 */
void kfree_lazy(void * p);

static inline void _kfree_cleanup(void * p)
{
    void ** pp = (void **)p;

    kfree(*pp);
}

/**
 * Reallocate a memory block.
 * Changes the size of the memory block pointed to by p.
 * @note This function behaves like C99 realloc.
 * @param p     is a pointer to a memory block previously allocated with
 *              kmalloc, kcalloc or krealloc.
 * @param size  is the new size for the memory block, in bytes.
 * @return  Returns a pointer to the reallocated memory block, which may be
 *          either the same as p or a new location. NULL indicates that the
 *          function failed to allocate memory, and p was not modified.
 */
void * krealloc(void * p, size_t size) __attribute__((warn_unused_result));

/**
 * New memory block reference.
 * Pass kmalloc'd pointer to a block and increment refcount.
 * @param p is a pointer to a kmalloc'd block of data.
 * @return Same as p.
 */
void * kpalloc(void * p);

#endif /* KMALLOC_H */

/**
 * @}
 */
