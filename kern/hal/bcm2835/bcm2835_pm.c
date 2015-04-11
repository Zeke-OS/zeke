/**
 *******************************************************************************
 * @file    bcm2835_pm.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 PM.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <hal/hw_timers.h>
#include "bcm2835_prop.h"

int bcm2835_pm_get_power_state(uint32_t devid)
{
    /*
     * Doesn't necessarily need to be aligned but it may make memcpy faster
     * in when passed to prop interface.
     */
    uint32_t mbuf[8] __attribute__((aligned (16)));
    int err;

    /* Format a message */
    mbuf[0] = sizeof(mbuf); /* Size */
    mbuf[1] = 0;            /* Request */
    /* Tags */
    /* Get power state */
    mbuf[2] = 0x00020001;
    mbuf[3] = 8;            /* Value buf size and req/resp */
    mbuf[4] = 4;            /* Value size */
    mbuf[5] = devid;
    mbuf[6] = 0;            /* Space for the response */

    mbuf[7] = 0x0;          /* End tag */

    err = bcm2835_prop_request(mbuf);
    if (err)
        return err;

    if ((mbuf[6] & 0x2))
        return -ENODEV; /* Device doesn't exist */

    return (mbuf[6] & 1);
}

int bcm2835_pm_set_power_state(uint32_t devid, int state)
{
    uint32_t mbuf[8] __attribute__((aligned (16)));

    int err;

    /* Format a message */
    mbuf[0] = sizeof(mbuf); /* Size */
    mbuf[1] = 0;            /* Request */
    /* Tags */
    /* Set power state */
    mbuf[2] = 0x00028001;
    mbuf[3] = 8;            /* Value buf size and req/resp */
    mbuf[4] = 8;            /* Value size */
    mbuf[5] = devid;
    mbuf[6] = (state) ? 3 : 2; /* power on and wait/power off and wait */

    mbuf[7] = 0x0;          /* End tag */

    err = bcm2835_prop_request(mbuf);
    if (err)
        return err;

    if ((mbuf[6] & 0x2))
        return -ENODEV; /* Device doesn't exist */

    return (mbuf[6] & 1);
}

/**
 * Get wait time required after turning on a device.
 * A wait time period is needed after a device is turned on with
 * bcm2835_pm_set_power_state().
 * @param devid is the id of the device.
 * @return Returns a wait time in usec.
 */
int bcm2835_pm_get_timing(uint32_t devid)
{
    uint32_t mbuf[7] __attribute__((aligned (16)));
    int err;

    /* Format a message */
    mbuf[0] = sizeof(mbuf); /* Size */
    mbuf[1] = 0;            /* Request */
    /* Tags */
    /* Get timing */
    mbuf[2] = 0x00020002;
    mbuf[3] = 8;            /* Value buf size and req/resp */
    mbuf[4] = 4;            /* Value size */
    mbuf[4] = devid;
    mbuf[5] = 0;            /* Space for response */

    mbuf[6] = 0x0;          /* End tag */

    err = bcm2835_prop_request(mbuf);
    if (err)
        return err;

    if (mbuf[5] == 0)
        return -ENODEV; /* Device doesn't exist */

    return mbuf[5];
}
