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
#include <hal/uart.h>
#include <kerror.h>
#include "kerror_ttys.h"

/* Due to current limitations in initialization we must directly use specific
 * hw functions to implement ttyS kerror. Hope this changes some day...
 * This will also cause a compiler warning because these things are
 * technically hidden even inside the kernel. */
#define KERROR_TTYS_UART_UPUTC_FN bcm2835_uputc

/**
 * @param log level.
 * @param where file and line number.
 * @param msg log message.
 */
void kerror_ttys(char level, const char * where, const char * msg)
{
    size_t i = 0;
    char buff[120];

    ksprintf(buff, sizeof(buff), "%u:%s%s\n", level, where, msg);

    while (buff[i] != '\0') {
        KERROR_TTYS_UART_UPUTC_FN(buff[i++]);
    }
}
