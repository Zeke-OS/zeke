/**
 *******************************************************************************
 * @file    bcm2835_interrput.h
 * @author  Olli Vanhoja
 * @brief   Header for bcm2835_interrupts.h
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

/** @addtogroup BCM2835
  * @{
  */

#pragma once
#ifndef BCM2835_INTERRUPT_H
#define BCM2835_INTERRUPT_H

#include <stdint.h>

/* Peripheral Addresses */
#define BCMIRQ_BASE                 0x2000b200
#define BCMIRQ_BASIC_PEND           (BCMIRQ_BASE + 0x00)
#define BCMIRQ_IRQ1_PEND            (BCMIRQ_BASE + 0x04)
#define BCMIRQ_IRQ2_PEND            (BCMIRQ_BASE + 0x08)
#define BCMIRQ_FIQ_CTRL             (BCMIRQ_BASE + 0x0C)
#define BCMIRQ_ENABLE_IRQ1          (BCMIRQ_BASE + 0x10)
#define BCMIRQ_ENABLE_IRQ2          (BCMIRQ_BASE + 0x14)
#define BCMIRQ_ENABLE_BASIC         (BCMIRQ_BASE + 0x18)
#define BCMIRQ_DISABLE_IRQ1         (BCMIRQ_BASE + 0x1C)
#define BCMIRQ_DISABLE_IRQ2         (BCMIRQ_BASE + 0x20)
#define BCMIRQ_DISABLE_BASIC        (BCMIRQ_BASE + 0x24)
/* End of Peripheral Addresses */

#endif /* BCM2835_INTERRUPT_H */

/**
  * @}
  */

/**
  * @}
  */
