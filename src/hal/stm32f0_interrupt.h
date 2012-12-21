/**
 *******************************************************************************
 * @file    stm32f0_interrupt.h
 * @author  Olli Vanhoja
 * @brief   Header for stm32f0_interrupt.c module.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup STM32F0
  * @{
  */

#pragma once
#ifndef STM32F0_INTERRUPT_H
#define STM32F0_INTERRUPT_H

#include <stddef.h>

// Libraries from ST (Licensed under MCD-ST Liberty SW License Agreement V2)
#include "stm32f0xx.h"

int interrupt_init_module(void);

// Interrupt handlers
void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USART1_IRQHandler(void);

#endif /* STM32F0_INTERRUPT_H */

/**
  * @}
  */

/**
  * @}
  */
