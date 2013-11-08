/**
 *******************************************************************************
 * @file    mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control.
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

/** @addtogroup HAL
  * @{
  */

#include <kinit.h>
#include <ptmapper.h>
#include "mmu.h"

extern void ptmapper_init(void); /* form ptmapper */
static void mmu_init(void);
HW_PREINIT_ENTRY(mmu_init);

/**
 * Initialize the MMU and static regions.
 * @note This is called from startup.
 */
static void mmu_init(void)
{
    uint32_t value, mask;

    ptmapper_init();

    /* Set MMU_DOM_KERNEL as client and others to generate error. */
    value = MMU_DOMAC_TO(MMU_DOM_KERNEL, MMU_DOMAC_CL);
    mask = MMU_DOMAC_ALL;
    mmu_domain_access_set(value, mask);

    value = MMU_ZEKE_C1_DEFAULTS;
    mask = MMU_ZEKE_C1_DEFAULTS;
    mmu_control_set(value, mask);
}

/**
  * @}
  */
