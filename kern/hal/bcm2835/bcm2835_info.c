/**
 *******************************************************************************
 * @file    bcm2835_info.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 Info.
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
#include <kerror.h>
#include <kinit.h>
#include <libkern.h>
#include "bcm2835_prop.h"

/*
 * TODO This should provide an alternative for atags, sysinfo data.
 *      Maybe we should somehow cause a compilation error
 *      if there is multiple sysinfo provides.
 */

/**
 * @param name is prined before the tag value.
 * @param tag is the tag id.
 * @param wc is the size of the property in words.
 */
static void print_prop(char * name, uint32_t tag, size_t wc)
{
    uint32_t mbuf[6 + wc] __attribute__((aligned (16)));
    int err;
    char buf[80];
    char * s;

    mbuf[0] = sizeof(mbuf);         /* Size */
    mbuf[1] = BCM2835_PROP_REQUEST; /* Request */
    /* Tags */
    mbuf[2] = tag;
    mbuf[3] = wc * 4;
    mbuf[4] = 0; /* Request len always zero */
    mbuf[5 + wc] = BCM2835_PROP_TAG_END;

    err = bcm2835_prop_request(mbuf);
    if (err) {
        KERROR(KERROR_INFO, "bcm2835_info %s: ERR %d\n", name, err);
        return;
    }

    s = buf;
    for (size_t i = 0; i < wc; i++) {
        s += ksprintf(s, buf + sizeof(buf) - s, "%x ", mbuf[5 + i]) - 1;
    }
    KERROR(KERROR_INFO, "bcm2835_info %s: %s\n", name, buf);
}

int __kinit__ bcm2835_info_init(void)
{
    SUBSYS_DEP(bcm2835_prop_init);
    SUBSYS_INIT("BCM2835_info");

    kputs("\n");
    print_prop("firmware", 0x00000001, 1);
    print_prop("board model", 0x00010001, 1);
    print_prop("board rev", 0x00010002, 1);
    print_prop("board serial", 0x00010004, 2);

    return 0;
}
