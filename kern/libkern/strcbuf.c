/**
 *******************************************************************************
 * @file    strcbuf.c
 * @author  Olli Vanhoja
 * @brief   Generic circular buffer for strings.
 * @section LICENSE
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

#include <kstring.h>
#include <strcbuf.h>

void strcbuf_insert(struct strcbuf * buf, const char * line, size_t len)
{
    size_t i = 0, end = buf->end, next;
    const size_t blen = buf->len;

    if (len > blen)
        return;

    while (1) {
        next = (end + 1) % blen;

        /* Check that queue is not full */
        if (next == buf->start)
            strcbuf_getline(buf, NULL, buf->len);

        buf->data[end] = line[i++];

        if (buf->data[end] == '\0')
            break;
        if (i >= len)
            break;

        end = next;
    }
    buf->data[end] = '\0';
    buf->end = next;
}

size_t strcbuf_getline(struct strcbuf * buf, char * dst, size_t len)
{
    size_t i = 0;
    size_t start = buf->start;
    const size_t end = buf->end;
    const size_t blen = buf->len;
    size_t next;

    if (start == end)
        return 0;

    if (strlenn(&(buf->data[buf->start]), len) >= len)
        return 0;

    while (start != end) {
        next = (start + 1) % blen;
        if (dst)
            dst[i++] = buf->data[start];
        else
            i++;
        if (buf->data[start] == '\0')
            break;
        start = next;
    }

    buf->start = next;
    return i;
}
