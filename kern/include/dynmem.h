/**
 *******************************************************************************
 * @file    dynmem.h
 * @author  Olli Vanhoja
 * @brief   Dynmem management headers.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup dynmem
 * @{
 */

#pragma once
#ifndef DYNMEM_H
#define DYNMEM_H

#include <stddef.h>
#include <stdint.h>
#include <sys/linker_set.h>
#include <hal/mmu.h>

/**
 * Dynmem page/region size in bytes.
 * In practice this should be always 1 MB.
 */
#define DYNMEM_PAGE_SIZE    MMU_PGSIZE_SECTION

/**
 * Struct describing a reserved memory area that should not be used by
 * dynmem.
 */
struct dynmem_reserved_area {
    uintptr_t caddr_start;
    uintptr_t caddr_end;
};

/**
 * Mark a physical memory range as reserved.
 * A memory region marked as reserved will not be used by dynmem for any
 * memory allocations.
 * @param _name_ is the name of the allocation.
 * @param _caddr_start_ is the start address of the reserved memory region.
 * @param _caddr_end_ is the end address of the reserved memory region.
 */
#define DYNMEM_RESERVED_AREA(_name_, _caddr_start_, _caddr_end_)        \
    static struct dynmem_reserved_area _dynmem_reserved_##_name_ = {    \
        .caddr_start = _caddr_start_,                                   \
        .caddr_end = _caddr_end_,                                       \
    };                                                                  \
    DATA_SET(dynmem_reserved, _dynmem_reserved_##_name_)

/**
 * Allocate a contiguous memory region from dynmem area.
 * @param size      Region size in 1MB blocks.
 * @param ap        Access permission.
 * @param control   Control settings.
 * @returns Address to the allocated region. Returns NULL if out of memory.
 */
void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t control);

/**
 * Get a reference to an already allocated region.
 * @param addr is the address of the already allocated dynmem region.
 * @returns 0 if succeed.
 */
int dynmem_ref(void * addr);

/**
 * Decrement dynmem region reference counter. If the final value of a reference
 * counter is zero then the dynmem region is freed and unmapped.
 * @param addr  Physical address of the dynmem region to be freed.
 */
void dynmem_free_region(void * addr);

/**
 * Clone a dynemem region.
 * Makes 1:1 copy of a given dynmem region to a new location in memory.
 * @param addr is the dynmem region address.
 * @returns Returns pointer to a clone of the dynmem area; Otherwise NULL in case
 *          of cloning failed.
 */
void * dynmem_clone(void * addr);

/*
 * Bit fields of the return value of dynmem_acc().
 */
#define DYNMEM_XN       0x8 /*!< Dynmem execute never bit. */
#define DYNMEM_AP_MASK  0x7 /*!< Dynmem mask for MMU AP bits. */

/**
 * Test for dynmem access.
 * Return value format:
 *
 *     3 2   0
 *   +--+----+
 *   |XN| AP |
 *   +--+----+
 *
 * AP is in same format as in mmu.h and XN is DYNMEM_XN.
 * @param addr  is the physical base address.
 * @param len   is the size of block tested.
 * @returns Returns 0 if addr is invalid; Otherwise returns ap flags + xn bit.
 */
uint32_t dynmem_acc(const void * addr, size_t len);

#endif /* DYNMEM_H */

/**
 * @}
 */
