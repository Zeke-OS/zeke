/**
 *******************************************************************************
 * @file    bcm2835_mmio.h
 * @author  Olli Vanhoja
 * @brief   Access to MMIO registers on BCM2835.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef BMC2835_MMIO_H
#define BMC2835_MMIO_H

#include <stdint.h>
#include <hal/core.h>

extern void bcm2835_mmio_init(void);

/* mmio_start & mmio_end are related to out-of-order AXI bus system in
 * BCM2835. See p. 7 in BCM2835-ARM-Peripherals.pdf */

/**
 * Begin MMIO access.
 */
static inline void mmio_start(istate_t * s_entry)
{
    const uint32_t rd = 0;

    *s_entry = get_interrupt_state();
    disable_interrupt();

    __asm__ volatile (
        "MCR    p15, 0, %[rd], c7, c10, 4\n\t" /* Drain write buffer. */
        "MCR    p15, 0, %[rd], c7, c10, 5\n\t" /* DMB */
        : : [rd]"r" (rd));
}

/**
 * End MMIO access.
 */
static inline void mmio_end(istate_t * s_entry)
{
    const uint32_t rd = 0;

    __asm__ volatile (
        "MCR    p15, 0, %[rd], c7, c10, 5\n\t" /* DMB */
        : : [rd]"r" (rd));

    set_interrupt_state(*s_entry);
}

/**
 *  Write to MMIO register.
 *  @param reg  Register address.
 *  @param data Data to write.
 */
static inline void mmio_write(uint32_t reg, uint32_t data)
{
    uint32_t *ptr = (uint32_t*)reg;

    __asm__ volatile (
        "str    %[data], [%[reg]]"
        : : [reg]"r"(ptr), [data]"r"(data));
}

/**
 * Read from MMIO register.
 * @param reg   Register address.
 * @return      Data read from MMIO register.
 */
static inline uint32_t mmio_read(uint32_t reg)
{
    uint32_t *ptr = (uint32_t*)reg;
    uint32_t data;

    __asm__ volatile (
        "ldr    %[data], [%[reg]]"
        : [data]"=r"(data) : [reg]"r"(ptr));

    return data;
}

#endif /* BMC2835_MMIO_H */

/**
  * @}
  */

/**
  * @}
  */
