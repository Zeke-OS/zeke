/**
 *******************************************************************************
 * @file    ksprintf.c
 * @author  Olli Vanhoja
 * @brief   Function to compose a string by using printf style formatter.
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

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#ifndef PU_TEST_BUILD
#include <kstring.h>
#endif

/**
 * Composes a string by using printf style format string and additional
 * arguments.
 *
 * This function supports following format specifiers:
 *     %u   Unsigned decimal integer
 *     %x   Unsigned hexadecimal integer
 *     %c   Character
 *     %%   Replaced with a single %
 *
 * @param str Pointer to a buffer where the resulting string is stored.
 * @param maxlen Maximum lenght of str. Replacements may cause writing of more
 *               than maxlen characters!
 * @param format String that contains a format string.
 * @param ... Depending on the format string, a sequence of additional
 *            arguments, each containing a value to be used to replace
 *            a format specifier in the format string.
 */
void ksprintf(char * str, size_t maxlen, const char * format, ...)
{
    va_list args;
    size_t i = 0, n = 0;
    char c;

    va_start(args, format);
    while ((c = format[i++]) != '\0' && n < maxlen) {
        switch (c) {
            case '%':
                c = format[i++];

                if (c == '\0')
                    break;

                switch (c) {
                    case 'u':
                        n += uitoa32(str + n, va_arg(args, uint32_t));
                        break;
                    case 'x':
                        n += uitoah32(str + n, va_arg(args, uint32_t));
                        break;
                    case 'c':
                        str[n++] = (char)(va_arg(args, int));
                        break;
                    case 's':
                        str[n] = '\0';
                        strnncat(str, maxlen, va_arg(args, char *), maxlen - n);
                        n = strlenn(str, maxlen);
                        break;
                    case '%':
                        str[n++] = c;
                        break;
                    default:
                        break;
                }
                break;
            default:
                str[n++] = c;
                break;
        }
    }

    va_end(args);
    str[n] = '\0';
}
