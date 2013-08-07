/**
 *******************************************************************************
 * @file    arm11_interrupt.h
 * @author  Olli Vanhoja
 * @brief   Header for arm11_interrupt.c module.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM11
  * @{
  */

#pragma once
#ifndef ARM11_INTERRUPT_H
#define ARM11_INTERRUPT_H

#include <stddef.h>

/* ==== Vendor specific interrupt handlers ====
 * These are only referenced inside vendor specific code (mainly assembly) */
void NMI_Handler(void);
/* Hard fault is handled in core specific code */
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#endif /* ARM11_INTERRUPT_H */

/**
  * @}
  */

/**
  * @}
  */
