/**
 *******************************************************************************
 * @file    arm11_perf.c
 * @author  Olli Vanhoja
 * @brief   ARM11 performance counters.
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

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM11
  * @{
  */

/* Performance monitoring events */
#define EVT_ICACHE_MISS     0x00 /*!< Instruction cache miss. */
#define EVT_STALL_INSTR     0x01
#define EVT_DATA_STALL      0x02
#define EVT_ITLB_MISS       0x03
#define EVT_DTLB_MISS       0x04
#define EVT_BRANCH          0x05
#define EVT_BRANCH_MISS     0x06
#define EVT_INSTRUCTION     0x07
#define EVT_DCACHE_ACC      0x09
#define EVT_DCACHE_ACC_ALL  0x0A
#define EVT_DCACHE_MISS     0x0B
#define EVT_DCACE_WB        0x0C
#define EVT_PC_CHANGED      0x0D
#define EVT_TLB_MISS        0x0F
#define EVT_EXT_REQ         0x10
#define EVT_EXT_FULL        0x11
#define EVT_EXT_DRAIN       0x12
#define EVT_ETMEXTOT0       0x20 /*!< ETMEXTOUT[0] signal was asserted. */
#define EVT_ETMEXTOT1       0x21 /*!< ETMEXTOUT[1] signal was asserted. */
#define EVT_ETMEXTOT        0x22 /*!< EVT_ETMEXTOT0 || EVT_ETMEXTOT1 */
#define EVT_PROCEDCALL      0x23 /*!< Procedure call instruction executed. */
#define EVT_PROCEDRET_P     0x24 /*!< Procedure return instruction executed and
                                  *   return address predicted correctly. */
#define EVT_PROCEDRET_IP    0x25 /*!< Procedure return instruction executed and
                                  *   return address redicted incorrectly. */
#define EVT_CYCLE           0xFF /*!< An increment each cycle. */

/* Performance Monitor Control Register bit positions */
#define EVT_CNT0_POS        0x20 /*!< Source of events for Count Register 0. */
#define EVT_CNT1_POS        0x12 /*!< Source of events for Count Register 1. */
#define EVT_EXPORT_POS      0x11
#define EVT_CCR_POS         0x10 /*!< Cycle Counter Register overflow flag. */
#define EVT_CR1_POS         0x09 /*!< Count Register 1 overflow flag. */
#define EVT_CR0_POS         0x08 /*!< Count Register 0 overflow flag. */
#define EVT_ECC_POS         0x06
#define EVT_EC1_POS         0x05
#define EVT_EC0_POS         0x04
#define EVT_D_POS           0x03 /*!< Cycle count divider. */
#define EVT_C_POS           0x02 /*!< Cycle count register reset. */
#define EVT_P_POS           0x01 /*!< Count register 0 & 1 reset. */
#define EVT_E_POS           0x00 /*!< Enable all counters. */

/* Performance Monitor Control Register bit masks */
#define EVT_CNT0            (0xff << EVT_CNT0_POS)
#define EVT_CNT1            (0xff << EVT_CNT1_POS)
#define EVT_EXPORT          (1 << EVT_EXPORT_POS)
#define EVT_CCR             (1 << EVT_CCR_POS)
#define EVT_CR1             (1 << EVT_CR1_POS)
#define EVT_CR0             (1 << EVT_CR0_POS)
#define EVT_ECC             (1 << EVT_ECC_POS)
#define EVT_EC1             (1 << EVT_EC1_POS)
#define EVT_EC0             (1 << EVT_EC0_POS)
#define EVT_D               (1 << EVT_D_POS)
#define EVT_C               (1 << EVT_C_POS)
#define EVT_P               (1 << EVT_P_POS)
#define EVT_E               (1 << EVT_E_POS)

/**
  * @}
  */

/**
  * @}
  */
