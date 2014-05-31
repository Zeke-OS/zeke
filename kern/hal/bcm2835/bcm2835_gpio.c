/**
 *******************************************************************************
 * @file    bcm2835_gpio.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 gpio.
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

#include <stddef.h>
#include <hal/core.h>
#include "bcm2835_mmio.h"
#include "bcm2835_gpio.h"

void bcm2835_set_gpio_func(int gpio, int func_code)
{
    istate_t s_entry;
    const size_t reg_ind = gpio / 10;
    const int bit = (gpio % 10) * 3;
    const uint32_t mask = 0b111 << bit;
    uint32_t mmio_addr = (uint32_t)(((uint32_t *)GPIO_BASE)[reg_ind]);
    uint32_t old;
    uint32_t new = ((func_code << bit) & mask);

    mmio_start(&s_entry);
    old = mmio_read(mmio_addr);
    new |= (old & ~mask);
    mmio_write(mmio_addr, new);
    mmio_end(&s_entry);
}
