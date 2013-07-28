/**
 *******************************************************************************
 * @file    arm1176jzf_s_interrupt.h
 * @author  Olli Vanhoja
 * @brief   Header for arm1176jzf_s_interrupt.c module.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup MCU_MODEL_ARM1176JZF_S
  * @{
  */

#pragma once
#ifndef ARM1176JZF_S_INTERRUPT_H
#define ARM1176JZF_S_INTERRUPT_H

#include <stddef.h>

/* ==== Vendor specific interrupt handlers ====
 * These are only referenced inside vendor specific code (mainly assembly) */
void NMI_Handler(void);
/* Hard fault is handled in core specific code */
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#endif /* ARM1176JZF_S_INTERRUPT_H */

/**
  * @}
  */

/**
  * @}
  */
