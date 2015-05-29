/**
 *******************************************************************************
 * @file    split.c
 * @author  Olli Vanhoja
 * @brief   Tiny shell.
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "tish.h"

size_t split(char * buffer, char * argv[], size_t argc_max)
{
    char * p;
    char * start_of_word;
    enum states { DULL, IN_WORD, IN_STRING } state;
    char quote;
    size_t argc = 0;

    state = DULL;
    for (p = buffer; argc < argc_max && *p != '\0'; p++) {
        int c = (unsigned char) *p;
        switch (state) {
        case DULL:
            if (isspace(c)) {
                continue;
            }

            if (c == '"' || c == '\'') {
                quote = c;
                state = IN_STRING;
                start_of_word = p + 1;
                continue;
            }
            state = IN_WORD;
            start_of_word = p;
            continue;

        case IN_STRING:
            if (c == quote) {
                *p = 0;
                argv[argc++] = start_of_word;
                state = DULL;
            }
            continue;

        case IN_WORD:
            if (isspace(c)) {
                *p = 0;
                argv[argc++] = start_of_word;
                state = DULL;
            }
            continue;
        }
    }

    if (state != DULL && argc < argc_max)
        argv[argc++] = start_of_word;
    argv[(argc < argc_max) ? argc : argc_max - 1] = NULL;

    return argc;
}
