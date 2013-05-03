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

/**
 * Initializer for interrupt module
 */
int interrupt_init_module(void);

/* ==== Vendor specific interrupt handlers ====
 * These are only referenced inside vendor specific code (mainly assembly) */
void NMI_Handler(void);
/* Hard fault is handled in core specific code */
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
