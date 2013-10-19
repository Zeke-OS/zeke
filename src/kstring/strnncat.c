/**
 *******************************************************************************
 * @file    strnncat.c
 * @author  Olli Vanhoja
 * @brief   String concatenate.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * Concatenate strings.
 * Appends a copy of the src to the dst string.
 * @param dst is the destination array.
 * @param ndst maximum length of dst.
 * @param src is the source string array.
 * @param nsrc maximum number of characters to bo copied from src.
 */
char * strnncat(char * dst, size_t ndst, const char * src, size_t nsrc)
{
    int i, j, n1, n2;

    for (n1 = 0; n1 < ndst; n1++) {
        if (dst[n1] == '\0')
            break;
    }

    for (n2 = 0; n2 < nsrc; n2++) {
        if (src[n2] == '\0')
            break;
    }

    i = 0;
    j = n1;
    while ((i < n2) && (j < ndst - 1)) {
        dst[j++] = src[i++];
    }
    dst[j] = '\0';

    return dst;
}
