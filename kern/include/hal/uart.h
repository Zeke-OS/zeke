/**
 *******************************************************************************
 * @file    uart.h
 * @author  Olli Vanhoja
 * @brief   Headers for UART HAL.
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

/** @addtogroup HAL
 * @{
 */

/** @addtogroup UART
 * @{
 */

#pragma once
#ifndef UART_H
#define UART_H

#include <autoconf.h>
#include <stdint.h>
#include <termios.h>

/* UART HAL Configuration */
#define UART_PORTS_MAX configUART_MAX_PORTS

#define UART_PORT_FLAG_FS       0x01 /*!< Port is exported to the devfs. */

struct uart_port {
    unsigned uart_id;   /*!< ID that can be used by the hal level driver. This
                         *   id is not connected with the port_num.
                         */
    unsigned flags;     /*!< Flags used by the UART abstraction layer. */
    struct termios conf;

    /**
     * Initialize UART.
     */
    void (* init)(struct uart_port * port);

    /**
     * Transmit a byte via UARTx.
     * @param byte Byte to send.
     * @returns 0 if byte was written; Otherwise -1, overflow.
     */
    int (* uputc)(struct uart_port * port, uint8_t byte);

    /**
     * Receive a byte via UART.
     * @return A byte read from UART or -1 if undeflow.
     */
    int (* ugetc)(struct uart_port * port);

    /**
     * Check if there is data available from UART.
     * @return 0 if no data avaiable; Otherwise value other than zero.
     */
    int (* peek)(struct uart_port * port);
};

/**
 * UART port register.
 * @param port_init_struct is a
 */
int uart_register_port(struct uart_port * port);
int uart_nports(void);
struct uart_port * uart_getport(int port_num);

#endif /* UART_H */

/**
 * @}
 */

/**
 * @}
 */
