/**
 *******************************************************************************
 * @file    strncpy.c
 * @author  Olli Vanhoja
 * @brief   strncpy C standard function for internal use.
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

#include <string.h>

/**
 * Copy characters from string.
 * Copies the first n characters of source to destination.
 * If the end of the source string (which is signaled by a null-character) is
 * found before n characters have been copied, destination is padded with
 * zeros until a total of num characters have been written to it.
 *
 * No null-character is implicitly appended at the end of destination if
 * source is longer than n. Thus, in this case, destination shall not
 * be considered a null terminated C string (reading it as such would overflow).
 * @param dst pointer to the destination array.
 * @param src string to be copied from.
 * @param n is the maximum number of characters to be copied from src.
 * @return dst.
 */
char * strncpy(char * dst, const char * src, ksize_t n)
{
    int end;

    for (end = 0; end < n; end++) {
        if (src[end] == '\0')
            break;
    }

    memcpy(dst, src, ++end);
    if (end < n) {
        memset(dst + end * sizeof(char), '\0', n - end);
    }

    return dst;
}
