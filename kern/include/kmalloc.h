/**
 *******************************************************************************
 * @file    kmalloc.h
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

/** @addtogroup kmalloc
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
 * Allocate memory block.
 * @param size is the size of memory block in bytes.
 * @return A pointer to the memory block allocated; 0 if failed to allocate.
 */
void * kmalloc(size_t size);

/**
 * Allocate and zero-intialize array.
 * @param nelem is the number of elements to allocate.
 * @param size  is the of each element.
 * @return A pointer to the memory block allocted; 0 if failed to allocate.
 */
void * kcalloc(size_t nelem, size_t elsize);

/**
 * Deallocate memory block.
 * Deallocates a memory block previously allocted with kmalloc, kcalloc or
 * krealloc.
 * @param p is a pointer to a previously allocated memory block.
 */
void kfree(void * p);

/**
 * Reallocate memory block.
 * Changes the size of the memory block pointed to by p.
 * @note This function behaves like C99 realloc.
 * @param p     is a pointer to a memory block previously allocated with
 *              kmalloc, kcalloc or krealloc.
 * @param size  is the new size for the memory block, in bytes.
 * @return  Returns a pointer to the reallocated memory block, which may be
 *          either the same as p or a new location. 0 indicates that the
 *          function failed to allocate memory, and p was not modified.
 */
void * krealloc(void * p, size_t size);

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
