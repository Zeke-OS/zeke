/**
 *******************************************************************************
 * @file    uitoa.c
 * @author  Olli Vanhoja
 * @brief   uitoa32 function.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kstring.h>

#define UITOA_TYPE(_type_) ({                       \
    _type_ div = 1, digs = 1;                       \
    size_t n = 0;                                   \
    for (; value / div >= 10; div *= 10, digs++);   \
    do {                                            \
        str[n++] = ((value / div) % 10) + '0';      \
    } while (div /= 10);                            \
    str[n] = '\0';                                  \
    (int)digs; })

#define UITOA_BASE_TYPE(_type_) ({                      \
    _type_ div = 1, digs = 1;                           \
    size_t n = 0;                                       \
    for (; value / div >= base; div *= base, digs++);   \
    do {                                                \
        str[n++] = ((value / div) % base) + '0';        \
    } while (div /= base);                              \
    str[n] = '\0';                                      \
    (int)digs; })

int uitoa32(char * str, uint32_t value)
{
    return UITOA_TYPE(uint32_t);
}

int uitoa64(char * str, uint64_t value)
{
    return UITOA_TYPE(uint64_t);
}

static int uitoah_nbits(char * str, uint64_t value, int nbits)
{
    size_t i = 2;
    int n = nbits - 4;
    uint64_t mask = (uint64_t)0xF << n;

    str[0] = '0';
    str[1] = 'x';

    for (; n >= 0; n -= 4) {
        char c;

        c = (char)((value & mask) >> n);
        c = (c < (char)10) ? '0' + c : 'a' + (c - (char)10);
        str[i++] = c;
        mask >>= 4;
    }
    str[i] = '\0';

    return i;
}

int uitoah32(char * str, uint32_t value)
{
    return uitoah_nbits(str, value, 32);
}

int uitoah64(char * str, uint64_t value)
{
    return uitoah_nbits(str, value, 64);
}

int uitoa32base(char * str, uint32_t value, uint32_t base)
{
    return UITOA_BASE_TYPE(uint32_t);
}

int uitoa64base(char * str, uint64_t value, uint64_t base)
{
    return UITOA_BASE_TYPE(uint64_t);
}
