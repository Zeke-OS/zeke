/**
 *******************************************************************************
 * @file    kerror.c
 * @author  Olli Vanhoja
 * @brief   Kernel error logging module that logs last n error messages to
 *          in-memory buffer.
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
#include "kerror.h"

char kerror_log[KERROR_LOG_SIZE][KERROR_LOG_MSGSIZE];
int kerror_log_last = 0;

/**
 * Store last KERROR_LOG_SIZE amount of kerror messages.
 * @param log level.
 * @param where file and line number.
 * @param msg log message.
 */
void kerror_last(char level, const char * where, const char * msg)
{
    int i, j;

    i = kerror_log_last;
    if (++i >= KERROR_LOG_SIZE) {
        i = 0;
    }

    *kerror_log[i] = level;
    strncpy(kerror_log[i] + 1 * sizeof(char), where, KERROR_LOG_HLEN_MAX);
    for (j = 0; j < KERROR_LOG_HLEN_MAX; j++) {
        if (kerror_log[i][j] == '\0')
            break;
    }
    strncpy(kerror_log[i] + j * sizeof(char), msg, KERROR_LOG_MSGSIZE - j - 1);

    kerror_log_last = i;
}
