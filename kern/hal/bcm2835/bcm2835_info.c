/**
 *******************************************************************************
 * @file    bcm2835_info.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 Info.
 * @section LICENSE
 * Copyright (c) 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/sysctl.h>
#include <kerror.h>
#include <kinit.h>
#include <kstring.h>
#include <libkern.h>
#include "bcm2835_prop.h"

static int get_info_prop(uint32_t * value, size_t value_size, uint32_t tag)
{
    size_t wc = value_size / 4;
    uint32_t mbuf[6 + wc] __attribute__((aligned (16)));
    int err;

    mbuf[0] = sizeof(mbuf);         /* Size */
    mbuf[1] = BCM2835_PROP_REQUEST; /* Request */
    /* Tags */
    mbuf[2] = tag;
    mbuf[3] = value_size;
    mbuf[4] = 0; /* Request len always zero */
    mbuf[5 + wc] = BCM2835_PROP_TAG_END;

    err = bcm2835_prop_request(mbuf);
    if (err) {
        return err;
    }

    while (wc--) {
        value[wc] = mbuf[5 + wc];
    }

    return 0;
}

int __kinit__ bcm2835_info_init(void)
{
    SUBSYS_DEP(bcm2835_prop_init);
    SUBSYS_INIT("BCM2835_info");

    uint32_t value[2];

    /*
     * HW_MODEL
     */
    if (!get_info_prop(value, 4, BCM2835_PROP_TAG_GET_GET_BOARD_MODEL)) {
        char hw_model[20];
        int ctl_name[] = { CTL_HW, HW_MODEL };

        ksprintf(hw_model, sizeof(hw_model), "BCM2835 model %d", value[0]);
        kernel_sysctl_write(ctl_name, num_elem(ctl_name),
                            hw_model, sizeof(hw_model));
    } else {
        KERROR(KERROR_WARN, "%s: Failed to get board model\n", __func__);
    }

    /*
     * HW_PHYSMEM_START & HW_PHYSMEM
     */
    if (!get_info_prop(value, 8, BCM2835_PROP_TAG_GET_ARM_MEMORY)) {
        int ctl_start[] = { CTL_HW, HW_PHYSMEM_START };
        int ctl_size[] = { CTL_HW, HW_PHYSMEM };

        kernel_sysctl_write(ctl_start, num_elem(ctl_start),
                            &value[0], sizeof(uint32_t));
        kernel_sysctl_write(ctl_size, num_elem(ctl_size),
                            &value[1], sizeof(uint32_t));

    } else {
        KERROR(KERROR_WARN, "%s: Failed to get ARM memory info\n", __func__);
    }

    return 0;
}
