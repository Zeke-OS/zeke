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

static const char * const rpi_rev_name[] = {
    [0x2] = "B1",
    [0x3] = "B1+",
    [0x4] = "B2",
    [0x5] = "B2",
    [0x6] = "B2",
    [0x7] = "A",
    [0x8] = "A",
    [0x9] = "A",
    [0xD] = "B2",
    [0xE] = "B2",
    [0xF] = "B2",

};
#define RPI_REV_NAME_MAX 4

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

static void get_hw_model(void)
{
    uint32_t model = 0, rev_id = 0;
    char rev_str[25] = "";
    char hw_model[40];
    int ctl_name[] = { CTL_HW, HW_MODEL };

    get_info_prop(&model, sizeof(model),
                  BCM2835_PROP_TAG_GET_GET_BOARD_MODEL);

    if (!get_info_prop(&rev_id, sizeof(rev_id),
                       BCM2835_PROP_TAG_GET_BOARD_REVISION)) {
        if (rev_id < num_elem(rpi_rev_name) &&
            strlenn(rpi_rev_name[rev_id], RPI_REV_NAME_MAX) > 0) {
            ksprintf(rev_str, sizeof(rev_str), " Raspberry Pi model %s",
                     rpi_rev_name[rev_id]);
        }
    }

    ksprintf(hw_model, sizeof(hw_model), "BCM2835 board model %u%s",
             (unsigned)model, rev_str);
    KERROR(KERROR_DEBUG, "%s\n", hw_model);
    kernel_sysctl_write(ctl_name, num_elem(ctl_name),
                        hw_model, strlenn(hw_model, sizeof(hw_model)));
}

int __kinit__ bcm2835_info_init(void)
{
    SUBSYS_DEP(bcm2835_prop_init);
    SUBSYS_INIT("BCM2835_info");

    uint32_t value[2];

    get_hw_model();

    /*
     * HW_PHYSMEM_START & HW_PHYSMEM
     */
    if (!get_info_prop(value, sizeof(value),
                       BCM2835_PROP_TAG_GET_ARM_MEMORY)) {
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
