/**
 *************************************************************************$
 * @file    hal_mcu.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for MCU model
 *************************************************************************$
 */

/** @addtogroup HAL
  * @{
  */

#pragma once
#ifndef HAL_MCU_H
#define HAL_MCU_H

#include "kernel_config.h"

#ifndef KERNEL_CONFIG_H
#error kernel_config.h should be #included before hal_mcu.h
#endif

#ifdef PU_TEST_BUILD
#undef configMCU_MODEL
#endif

extern uint32_t flag_kernel_tick;

/**
 *  Set flag_kernel_tick to 1 if tick is now else 0.
 */
inline void eval_kernel_tick(void);

#ifndef PU_TEST_BUILD
#ifndef configMCU_MODEL
    #error MCU model not selected.
#endif

#if configMCU_MODEL == MCU_MODEL_STM32F0
#include "stm32f0xx.h" /* Library developed by ST
                    (Licensed under MCD-ST Liberty SW License Agreement V2) */
#include "stm32f0_interrupt.h"

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

#error Not yet implemented support for STR912F
#else
    #error No hardware support for the selected MCU model.
#endif

#endif /* PU_TEST_BUILD */
#endif /* HAL_MCU_H */

/**
  * @}
  */

