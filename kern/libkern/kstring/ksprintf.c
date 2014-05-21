/**
 *******************************************************************************
 * @file    ksprintf.c
 * @author  Olli Vanhoja
 * @brief   Function to compose a string by using printf style formatter.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdarg.h>
#include <kstring.h>

void ksprintf(char * str, size_t maxlen, const char * format, ...)
{
    va_list args;
    size_t i = 0, n = 0;
    char c;

    maxlen--;
    va_start(args, format);
    while ((c = format[i++]) != '\0' && n <= maxlen) {
        switch (c) {
            case '%':
                c = format[i++];

                if (c == '\0')
                    break;

                switch (c) {
                    case 'd':
                    case 'i':
                        {
                            int x = va_arg(args, int);
                            if (x < 0) {
                                x *= -1;
                                str[n++] = '-';
                            }
                            n += uitoa32(str + n, (uint32_t)x);
                        }
                        break;
                    case 'u':
                        n += uitoa32(str + n,
                                (uint32_t)(va_arg(args, unsigned int)));
                        break;
                    case 'p':
                        str[n++] = 'b';
                        /*@fallthrough@*/
                    case 'x':
                        n += uitoah32(str + n,
                                (uint32_t)(va_arg(args, unsigned int)));
                        break;
                    case 'c':
                        str[n++] = (char)(va_arg(args, int));
                        break;
                    case 's':
                        //str[n] = '\0';
                        {
                            size_t j = 0;
                            char * buf = va_arg(args, char *);
                            for (; n < maxlen; n++) {
                                if (buf[j] == '\0')
                                    break;
                                str[n] = buf[j++];
                            }
                        }
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

    str[n] = '\0';
    va_end(args);
}
