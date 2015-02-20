/**
 *******************************************************************************
 * @file    gline.c
 * @author  Olli Vanhoja
 * @brief   gline, get line.
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

#include <unistd.h>

char * gline(char * str, int num)
{
    int err, i = 0;
    char ch;
    char buf[2] = {'\0', '\0'};

    while (1) {
        err = read(STDIN_FILENO, &ch, sizeof(ch));
        if (err <= 0)
            continue;

        /* Handle backspace */
        if (ch == '\b') {
            if (i > 0) {
                i--;
                write(STDOUT_FILENO, "\b \b", 4);
            }
            continue;
        }

        /* TODO Handle arrow keys and delete */

        /* Handle return */
        if (ch == '\n' || ch == '\r' || i == num) {
            str[i] = '\0';
            buf[0] = '\n';
            write(STDOUT_FILENO, buf, sizeof(buf));

            return str;
        } else {
            str[i] = ch;
        }

        buf[0] = ch;
        write(STDOUT_FILENO, buf, sizeof(buf));
        i++;
    }
}
