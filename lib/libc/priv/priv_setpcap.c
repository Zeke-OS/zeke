/**
 *******************************************************************************
 * @file    priv_setpcap.c
 * @author  Olli Vanhoja
 * @brief   Process credentials.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define __SYSCALL_DEFS__
#include <stddef.h>
#include <sys/types.h>
#include <syscall.h>
#include <sys/priv.h>

int priv_setpcap(int bounding, size_t priv, int value)
{
    struct _priv_pcap_args args = {
        .priv = priv
    };

    if (bounding) {
        if (value) {
            args.mode = PRIV_PCAP_MODE_SET_BND;
        } else {
            args.mode = PRIV_PCAP_MODE_CLR_BND;
        }
    } else {
        if (value) {
            args.mode = PRIV_PCAP_MODE_SET_EFF;
        } else {
            args.mode = PRIV_PCAP_MODE_CLR_EFF;
        }
    }

    return syscall(SYSCALL_PRIV_PCAP, &args);
}
