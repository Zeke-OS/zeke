/**
 *******************************************************************************
 * @file    bcm2708_emmc.c
 * @author  Olli Vanhoja
 * @brief   BCM2708 specific emmc driver functions.
 * @section LICENSE
 * Copyright (c) 2014, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *
 *
 * Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *******************************************************************************
 */

#include <errno.h>
#include <kerror.h>
#include "../bcm2835/bcm2835_pm.h"
#include "../bcm2835/bcm2835_prop.h"
#include "../bcm2835/bcm2835_timers.h"
#include "emmc.h"

static int emmc_power_cycle(void)
{
    int resp;

    resp = bcm2835_pm_set_power_state(BCM2835_SD, 0);
    if (resp < 0) {
        KERROR(KERROR_ERR, "Failed to power off (%d)\n", resp);
        return -EIO;
    }

    bcm_udelay(5000);

    resp = bcm2835_pm_set_power_state(BCM2835_SD, 1);
    if (resp != 1) {
        KERROR(KERROR_ERR, "Failed to power on (%d)\n", resp);
        return -EIO;
    }

    return 0;
}

static uint32_t sd_get_base_clock_hz(struct emmc_capabilities * cap)
{
    uint32_t base_clock;
    uint32_t mb[8];

    mb[0] = sizeof(mb); /* size of this message */
    mb[1] = 0;
    /* next comes the first tag */
    mb[2] = BCM2835_PROP_TAG_GET_CLK_RATE;
    mb[3] = 0x8;        /* value buffer size */
    mb[4] = 0x4;        /* is a request, value length = 4 */
    mb[5] = 0x1;        /* clock id + space to return clock id */
    mb[6] = 0;          /* space to return rate (in Hz) */
    /* closing tag */
    mb[7] = BCM2835_PROP_TAG_END;

    if (bcm2835_prop_request(mb)) {
        KERROR(KERROR_ERR,
               "EMMC: property mailbox did not return a valid response.\n");

        return 0;
    }

    if (mb[5] != 0x1) {
        KERROR(KERROR_ERR,
               "EMMC: property mailbox did not return a valid clock id.\n");

        return 0;
    }

    base_clock = mb[6];

    KERROR_DBG("EMMC: base clock rate is %u Hz\n", base_clock);

    return base_clock;
}

struct emmc_hw_support emmc_hw = {
    .emmc_power_cycle = emmc_power_cycle,
    .sd_get_base_clock_hz = sd_get_base_clock_hz,
};
