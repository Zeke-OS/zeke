/**
 *******************************************************************************
 * @file    bcm2835_gpio.h
 * @author  Olli Vanhoja
 * @brief   BVM2835 gpio.
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

#ifndef BCM2835_GPIO_H
#define BCM2835_GPIO_H

#include <stdint.h>

#define GPIO_BASE       0x20200000
#define GPIO_GPFSEL1    (GPIO_BASE + 0x04)
#define GPIO_GPSET0     (GPIO_BASE + 0x1c)
#define GPIO_GPCLR0     (GPIO_BASE + 0x28)
#define GPIO_GPLEV0     (GPIO_BASE + 0x34)
/** Pull up/down control of ALL GPIO pins. */
#define GPIO_GPPUD      (GPIO_BASE + 0x94)
/** Pull up/down control for specific GPIO pin. */
#define GPIO_PUDCLK0  (GPIO_BASE + 0x98)
#define GPIO_PUDCLK1  (GPIO_BASE + 0x9c)

void bcm2835_gpio_delay(int32_t count);

#endif /* BCM2835_GPIO_H */

/**
  * @}
  */

/**
  * @}
  */
