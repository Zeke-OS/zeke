/**
 *******************************************************************************
 * @file    kerror_uart.c
 * @author  Olli Vanhoja
 * @brief   UART klogger.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL
#include <kstring.h>
#include <hal/uart.h>
#include <kerror.h>
#include <sys/linker_set.h>

#if configUART == 0
#error configUART must be enabled
#endif

static struct uart_port * kerror_uart;

/**
 * Kerror logger init function called by kerror_init.
 */
static void kerror_uart_init(void)
{
    kerror_uart = uart_getport(0);
}

static void kerror_uart_puts(const char * str)
{
    size_t i = 0;

    while (str[i] != '\0') {
        if (str[i] == '\n')
            kerror_uart->uputc(kerror_uart, '\r');
        kerror_uart->uputc(kerror_uart, str[i++]);
    }
}

static const struct kerror_klogger klogger_uart = {
    .id     = KERROR_UARTLOG,
    .init   = &kerror_uart_init,
    .puts   = &kerror_uart_puts,
    .read   = 0,
    .flush  = 0
};
DATA_SET(klogger_set, klogger_uart);
