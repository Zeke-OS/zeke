/**
 *******************************************************************************
 * @file    hal_mcu.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for MCU model
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#pragma once
#ifndef HAL_MCU_H
#define HAL_MCU_H

#include <autoconf.h>

#ifdef PU_TEST_BUILD
#undef configMCU_MODEL
#endif

extern uint32_t flag_kernel_tick;

/**
 *  Sets flag_kernel_tick to 1 if tick is now else 0.
 */
inline void eval_kernel_tick(void);

#ifndef PU_TEST_BUILD
#ifndef configMCU_MODEL
    #error MCU model not selected.
#endif

#if configMCU_MODEL == MCU_MODEL_STM32F0
#include "stm32f0xx.h" /* Library developed by ST
                    (Licensed under MCD-ST Liberty SW License Agreement V2) */
#include "stm32f0/stm32f0_interrupt.h"

inline void eval_kernel_tick(void)
{
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) {
        flag_kernel_tick = 1;
    } else {
        flag_kernel_tick = 0;
    }
}

#elif configMCU_MODEL == MCU_MODEL_STR912F
/** @todo Not implemented yet */
inline void eval_kernel_tick(void)
{
}

#error Support for STR912F is not implemented yet.
#elif configMCU_MODEL == MCU_MODEL_BCM2835
inline void eval_kernel_tick(void)
{
}
    //#error Support for BCM2835 is not implemented yet.
#else
    #error No hardware support for the selected MCU model.
#endif

#endif /* PU_TEST_BUILD */
#endif /* HAL_MCU_H */

/**
  * @}
  */

