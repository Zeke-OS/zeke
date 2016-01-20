/**
 *******************************************************************************
 * @file    hal_mib.c
 * @author  Olli Vanhoja
 * @brief   Kernel HW Management Information Base (MIB).
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <machine/endian.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/sysctl.h>
#include <hal/core.h>
#include <hal/mmu.h> /* to get MMU_PGSIZE_COARSE */

SYSCTL_NODE(, CTL_HW, hw, CTLFLAG_RW, 0,
            "hardware");

SYSCTL_NODE(_hw, OID_AUTO, pm, CTLFLAG_RW, 0,
            "pm");

/* TODO HW_MACHINE */

static char hw_model[16];
SYSCTL_STRING(_hw, HW_MODEL, model, CTLFLAG_RD | CTLFLAG_KERWR,
              hw_model, sizeof(hw_model),
              "HW model");

SYSCTL_INT(_hw, HW_BYTEORDER, byteorder, CTLFLAG_RD, NULL, _BYTE_ORDER,
           "Byte order");

/* TODO HW_NCPU */

size_t physmem_start;
SYSCTL_UINT(_hw, HW_PHYSMEM_START, physmem_start,
            CTLFLAG_RD | CTLFLAG_KERWR | CTLFLAG_SKIP, &physmem_start, 0,
            "");

size_t physmem_size = configDYNMEM_SAFE_SIZE;
SYSCTL_UINT(_hw, HW_PHYSMEM, physmem, CTLFLAG_RD | CTLFLAG_KERWR,
            &physmem_size, 0,
            "Total memory");

/* TODO HW_USERMEM */

SYSCTL_UINT(_hw, HW_PAGESIZE, pagesize, CTLFLAG_RD, NULL, MMU_PGSIZE_COARSE,
            "Page size");

SYSCTL_UINT(_hw, HW_FLOATINGPT, floatingpt, CTLFLAG_RD, NULL, IS_HFP_PLAT,
            "Hardware floating point");

/* TODO HW_MACHINE_ARCH */

/* TODO HW_REALMEM but maybe not here? */
