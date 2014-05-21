/**
 *******************************************************************************
 * @file    vralloc.h
 * @author  Olli Vanhoja
 * @brief   Virtual Region Allocator.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup vralloc
 * Virtual region memory allocator.
 * Vralloc is used to allocate memory regions that can be mapped into user
 * space as well as kernel space. This is usually done by using physical
 * memory layout in kernel mode and using vaddr as a user space mapping.
 * @sa kmalloc
 * @{
 */

#ifndef VRALLOC_H
#define VRALLOC_H
#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif
#include <stdint.h>
#include <vm/vm.h>

#define VRALLOC_ALLOCATOR_ID 0xBE57

/**
 * Initializes vregion allocator data structures.
 */
void vralloc_init(void) __attribute__((constructor));

/**
 * Allocate a virtual memory region.
 * Usr has a write permission by default.
 * @note Page table and virtual address is not set.
 * @param size is the size of new region in bytes.
 * @return  Returns vm_region struct if allocated was successful;
 *          Otherwise function return 0.
 */
vm_region_t * vralloc(size_t size);

/**
 * Clone a vregion.
 * @param old_region is the old region to be cloned.
 * @return  Returns pointer to the new vregion if operation was successful;
 *          Otherwise zero.
 */
struct vm_region * vr_rclone(struct vm_region * old_region);

/**
 * Free allocated vregion.
 * Dereferences a vregion.
 * @param region is a vregion to be derefenced/freed.
 */
void vrfree(struct vm_region * region);

#endif /* VRALLOC_H */

/**
 * @}
 */
