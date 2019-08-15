/**
 *******************************************************************************
 * @file    pcaps.c
 * @author  Olli Vanhoja
 * @brief   Show process capabilities.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/priv.h>
#include <sysexits.h>

static void print_n(uint32_t * caps)
{
    size_t i = _PRIV_MLEN - 1;

    do {
        printf("%08x", caps[i]);
    } while (--i > 0);

    putchar('\n');
}

static void print_h(uint32_t * caps)
{
    for (size_t i = 0; i < _PRIV_MLEN; i++) {
        uint32_t b = caps[i];
        for (int j = 0; j < 32; j++) {
            if ((b >> j) & 1) {
                size_t cap_num = i * 32 + j;

                printf("%s (%d), ", priv_cap_name[cap_num], cap_num);
            }
        }
    }
    putchar('\n');
}

int main(int argc, char * argv[])
{
    int err;
    uint32_t eff[_PRIV_MLEN];
    uint32_t bound[_PRIV_MLEN];

    err = priv_getpcaps(eff, bound);
    if (err) {
        return EX_OSERR;
    }

    printf("effective: ");
    print_n(eff);
    print_h(eff);
    printf("bounding:  ");
    print_n(bound);
    print_h(bound);

    return 0;
}
