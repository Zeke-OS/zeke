/**
 *******************************************************************************
 * @file    rpi_hw.c
 * @author  Olli Vanhoja
 * @brief   Raspberry Pi memory mapped hardware.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <autoconf.h>
#include <hal/mmu.h>
#include <ptmapper.h>

mmu_region_t mmu_region_rpihw = {
    .vaddr          = MMU_VADDR_RPIHW_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(MMU_VADDR_RPIHW_START, \
                        MMU_VADDR_RPIHW_END, MMU_PGSIZE_SECTION),
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_SDEV | MMU_CTRL_XN,
    .paddr          = MMU_VADDR_RPIHW_START,
    .pt             = &mmu_pagetable_master
};
PTMAPPER_FIXED_REGION(mmu_region_rpihw);
