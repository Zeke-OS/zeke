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
#ifndef HAL_CORE_H
#define HAL_CORE_H

#include "kernel_config.h"

#ifndef KERNEL_CONFIG_H
#error kernel_config.h should be #included before hal_mcu.h
#endif

#ifdef PU_TEST_BUILD
#undef configMCU_MODEL
#endif

#ifndef configMCU_MODEL
    #error MCU model not selected.
#endif

#if configMCU_MODEL == MCU_MODEL_STM32F0
#include "stm32f0_interrupt.h"
#else
    #error No hardware support for the selected MCU model.
#endif

#endif /* HAL_CORE_H */

/**
  * @}
  */

