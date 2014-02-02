/**
 *******************************************************************************
 * @file    dynmem.h
 * @author  Olli Vanhoja
 * @brief   Dynmem management headers.
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

#pragma once
#ifndef DYNMEM_H
#define DYNMEM_H
#include <stddef.h>
#include <ptmapper.h>
#include <hal/mmu.h>

/* TODO
 * - Manage memmap arrays
 * - Reference counting for dynmem allocations & mappings
 * - Offer high level memory mapping & allocation functions
 * - Macros to incr/decr ref count on dynamic memory region
 * - Should we have 2 GB dynmem and map it as 1:1 to phys only when possible?
 */

/**
 * Dynmem area starts
 * TODO check if this is ok?
 */
#define DYNMEM_START    MMU_VADDR_DYNMEM_START

/**
 * Dynmem area end
 * TODO should match end of physical memory at least
 * (or higher if paging is allowed later)
 */
#define DYNMEM_END      MMU_VADDR_DYNMEM_END

/**
 * Dynmemmap size.
 * Dynmem memory space is allocated in 1MB sections.
 */
#define DYNMEM_MAPSIZE  ((DYNMEM_END - DYNMEM_START) / 1048576)

/**
 * Size of dynmem page table in pt region.
 */
#define DYNMEM_PT_SIZE  MMU_PTSZ_COARSE

extern uint32_t dynmemmap[];

void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t control);
void * dynmem_alloc_force(void * addr, size_t size, uint32_t ap, uint32_t control);
void dynmem_free_region(void * addr);
uint32_t dynmem_acc(void * addr, size_t len);

#endif /* DYNMEM_H */
