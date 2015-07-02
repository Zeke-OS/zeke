/**
 *******************************************************************************
 * @file    dynmem.h
 * @author  Olli Vanhoja
 * @brief   Dynmem management headers.
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

/**
 * @addtogroup dynmem
 * @{
 */

#pragma once
#ifndef DYNMEM_H
#define DYNMEM_H

#include <stddef.h>
#include <sys/linker_set.h>

/**
 * Struct describing a reserved memory area that should not be used by
 * dynmem.
 */
struct dynmem_reserved_area {
    uintptr_t caddr_start;
    uintptr_t caddr_end;
};

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
 * @return  Address to the allocated region. Returns 0 if out of memory.
 */
void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t control);

/**
 * Forces a new memory region allocation from the given address even if it's
 * already reserved.
 * This function will never fail, and might be destructive and may even
 * corrupt the allocation table.
 * @param addr  Address.
 * @param size  Region size in 1MB blocks.
 * @param ap    Access permission.
 * @param control Control settings.
 * @return  address to the allocated region.
 */
void * dynmem_alloc_force(void * addr, size_t size, uint32_t ap,
                          uint32_t control);

/**
 * Add reference to the already allocated region.
 * @return Address to the allocated region; Otherwise 0.
 */
void * dynmem_ref(void * addr);

/**
 * Decrement dynmem region reference counter. If the final value of a reference
 * counter is zero then the dynmem region is freed and unmapped.
 * @param addr  Physical address of the dynmem region to be freed.
 */
void dynmem_free_region(void * addr);

/**
 * Clones a dynemem region.
 * Makes 1:1 copy of a given dynmem region to a new location in memory.
 * @param addr is the dynmem region address.
 * @return  Returns pointer to a clone of the dynmem area; Otherwise 0 in case
 *          of cloning failed.
 */
void * dynmem_clone(void * addr);

#define DYNMEM_XN       0x8
#define DYNMEM_AP_MASK  0x7

/**
 * Test for dynmem access.
 * Return value format:
 *   3 2   0
 * +--+----+
 * |XN| AP |
 * +--+----+
 * @param addr  is the physical base address.
 * @param len   is the size of block tested.
 * @return Returns 0 if addr is invalid; Otherwise returns ap flags + xn bit.
 */
uint32_t dynmem_acc(const void * addr, size_t len);

#endif /* DYNMEM_H */

/**
 * @}
 */
