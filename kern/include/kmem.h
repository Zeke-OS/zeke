/**
 *******************************************************************************
 * @file    kmem.h
 * @author  Olli Vanhoja
 * @brief   Kernel memory mappings.
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
 * @addtogroup kmem
 * @{
 */

#pragma once
#ifndef KMEM_H
#define KMEM_H

#include <sys/linker_set.h>
#include <hal/mmu.h>
#include <vm/vm.h>

#define KMEM_FIXED_REGION(_region_name) \
    DATA_SET(kmem_fixed_regions, _region_name)

SET_DECLARE(kmem_fixed_regions, mmu_region_t);

#define KMEM_FOREACH(_regionp) \
        SET_FOREACH(_regionp, kmem_fixed_regions)

struct vm_pt;

extern mmu_pagetable_t mmu_pagetable_master;
extern struct vm_pt vm_pagetable_system;

extern const mmu_region_t mmu_region_kstack;
extern mmu_region_t mmu_region_kernel;
extern mmu_region_t mmu_region_kdata;

extern int _kmem_ready;

#endif /* KMEM_H */

/**
 * @}
 */
