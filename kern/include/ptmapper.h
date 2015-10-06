/**
 *******************************************************************************
 * @file    ptmapper.h
 * @author  Olli Vanhoja
 * @brief   Page table mapper.
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
 * @addtogroup ptmapper
 * @{
 */

#pragma once
#ifndef PTMAPPER_H
#define PTMAPPER_H

#include <hal/mmu.h>

/**
 * Allocate memory for a page table.
 * Allocate memory for a page table from the page table region. This function
 * will not activate the page table or do anything besides updating necessary
 * varibles in pt and reserve a block of memory from the area.
 * @note master_pt_addr will be only set for a page table struct if allocated
 * page table is a master page table.
 * @param pt    is the page table struct without page table address pt_addr.
 * @return  Returns zero if succeed; Otherwise value other than zero indicating
 *          that the page table allocation has failed.
 */
int ptmapper_alloc(mmu_pagetable_t * pt);

/**
 * Free page table.
 * Frees a page table that has been previously allocated with ptmapper_alloc.
 * @note    Page table pt should be detached properly before calling this
 *          function and it is usually good idea to unmap any regions still
 *          mapped with the page table before removing it completely.
 * @param pt is the page table to be freed.
 */
void ptmapper_free(mmu_pagetable_t * pt);

#endif /* PTMAPPER_H */

/**
 * @}
 */
