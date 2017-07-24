/**
 *******************************************************************************
 * @file    bcm2835_interrupt.h
 * @author  Olli Vanhoja
 * @brief   Header for BCN2835 interrupt controllers.
 * @section LICENSE
 * Copyright (c) 2014, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/*
 * IRQ Numbering in Zeke
 * This mapping differs from BCM's numbering as ARM basic interrupts are
 * mapped from 0 to 7 while BCM documents a separate table for them.
 *
 * - 0 ARM Timer
 * - 1 ARM Mailbox
 * - 2 ARM Doorbell 0
 * - 3 ARM Doorbell 1
 * - 4 GPU0 halted
 * - 5 GPU1 halted
 * - 6 Illegal access type 1
 * - 7 Illegal access type 0
 * - 8 reserved
 * - 9 reserved
 * - 10 GPU IRQ 7
 * - 11 GPU IRQ 9
 * - 12 GPU IRQ 10
 * - 13 GPU IRQ 18
 * - 14 GPU IRQ 19
 * - 15 GPU IRQ 53
 * - 16 GPU IRQ 54
 * - 17 GPU IRQ 55
 * - 18 GPU IRQ 56
 * - 19 GPU IRQ 57
 * - 20 GPU IRQ 62
 * - 29 Aux int
 * - 43 i2c slv int
 * - 46 pwa0
 * - 46 pwa1
 * - 48 smi
 * - 49 gpio_int[0]
 * - 50 gpio_int[1]
 * - 51 gpio_int[2]
 * - 52 gpio_int[3]
 * - 53 i2c_int
 * - 54 spi_int
 * - 55 pcm_int
 * - 57 uart_int
 */

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

/* Pending basic interrupts */
#define BCMIRQ_PEND_BASIC_ARM_TIMER 0x000001
#define BCMIRQ_PEND_BASIC_ARM_MBOX  0x000002
#define BCMIRQ_PEND_BASIC_ARM_DB0   0x000004
#define BCMIRQ_PEND_BASIC_ARM_DB1   0x000008
#define BCMIRQ_PEND_BASIC_GPU0_HALT 0x000010
#define BCMIRQ_PEND_BASIC_GPU1_HALT 0x000020
#define BCMIRQ_PEND_BASIC_ILL_ACC1  0x000040
#define BCMIRQ_PEND_BASIC_ILL_ACC0  0x000080
#define BCMIRQ_PEND_BASIC_PEND_REG1 0x000100 /* Check pending register 1 */
#define BCMIRQ_PEND_BASIC_PEND_REG2 0x000200 /* Check pending register 2 */
#define BCMIRQ_PEND_BASIC_GPU_IRQ7  0x000400
#define BCMIRQ_PEND_BASIC_GPU_IRQ9  0x000800
#define BCMIRQ_PEND_BASIC_GPU_IRQ10 0x001000
#define BCMIRQ_PEND_BASIC_GPU_IRQ18 0x002000
#define BCMIRQ_PEND_BASIC_GPU_IRQ19 0x004000
#define BCMIRQ_PEND_BASIC_GPU_IRQ53 0x008000
#define BCMIRQ_PEND_BASIC_GPU_IRQ54 0x010000
#define BCMIRQ_PEND_BASIC_GPU_IRQ55 0x020000
#define BCMIRQ_PEND_BASIC_GPU_IRQ56 0x040000
#define BCMIRQ_PEND_BASIC_GPU_IRQ57 0x080000
#define BCMIRQ_PEND_BASIC_GPU_IRQ62 0x100000

/* Enable/Disable masks for basic interrupts */
#define BCMIRQ_EN_BASIC_ARM_TIMER   0x01
#define BCMIRQ_EN_BASIC_ARM_MBOX    0x02
#define BCMIRQ_EN_BASIC_ARM_DB0     0x04
#define BCMIRQ_EN_BASIC_ARM_DB1     0x08
#define BCMIRQ_EN_BASIC_GPU0        0x10
#define BCMIRQ_EN_BASIC_GPU1        0x20
#define BCMIRQ_EN_BASIC_ACCERR1     0x40
#define BCMIRQ_EN_BASIC_ACCERR0     0x80

/* IRQ1 and IRQ2 masks */
#define BCMIRQ_EN_IRQ1_AUX_INT      (1 << 29)
#define BCMIRQ_EN_IRQ2_I2C_SLV      (1 << (43 - 32))
#define BCMIRQ_EN_IRQ2_PWA0         (1 << (45 - 32))
#define BCMIRQ_EN_IRQ2_PWA1         (1 << (46 - 32))
#define BCMIRQ_EN_IRQ2_SMI          (1 << (48 - 32))
#define BCMIRQ_EN_IRQ2_GPIO_INT0    (1 << (49 - 32))
#define BCMIRQ_EN_IRQ2_GPIO_INT1    (1 << (50 - 32))
#define BCMIRQ_EN_IRQ2_GPIO_INT2    (1 << (51 - 32))
#define BCMIRQ_EN_IRQ2_GPIO_INT3    (1 << (52 - 32))
#define BCMIRQ_EN_IRQ2_I2C_INT      (1 << (53 - 32))
#define BCMIRQ_EN_IRQ2_SPI_INT      (1 << (54 - 32))
#define BCMIRQ_EN_IRQ2_PCM_INT      (1 << (55 - 32))
#define BCMIRQ_EN_IRQ2_UART_INT     (1 << (57 - 32))

#endif /* BCM2835_INTERRUPT_H */

/**
  * @}
  */

/**
  * @}
  */
