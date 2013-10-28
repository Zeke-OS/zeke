/**
 *******************************************************************************
 * @file    uart.h
 * @author  Olli Vanhoja
 * @brief   Headers for UART HAL.
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

/** @addtogroup HAL
* @{
*/

#pragma once
#ifndef UART_H
#define UART_H

#include <stdint.h>

/**
 * UART data bits.
 */
enum uart_databits {
    UART_DATABITS_5,        /*!< 5 data bits. */
    UART_DATABITS_6,        /*!< 6 data bits. */
    UART_DATABITS_7,        /*!< 7 data bits. */
    UART_DATABITS_8         /*!< 8 data bits. */
};

/**
 * UART stop bits selection enum type.
 */
enum uart_stopbits {
    UART_STOPBITS_ONE = 1,  /*!< One stop bit */
    UART_STOPBITS_TWO = 2   /*!< Two stop bits */
};

/**
 * UART parity bit generation and detection.
 */
enum uart_parity {
    UART_PARITY_EVEN,       /*!< Even parity. */
    UART_PARITY_ODD,        /*!< Odd parity. */
    UART_PARITY_NO          /*!< No parity bit generation and detection. */
};

/* List of mandatory baud rates. */
#define UART_BAUDRATE_9600      9600
#define UART_BAUDRATE_115200    115200

typedef struct {
    unsigned int baud_rate;         /*!< Baud rate selection. */
    enum uart_databits data_bits;   /*!< Data bits. */
    enum uart_stopbits stop_bits;   /*!< One or Two stop bits. */
    enum uart_parity parity;        /*!< Parity. */
} uart_init_t;

/**
 * Initialize UART.
 * @param port Port number.
 */
void uart_init(int port, const uart_init_t * conf);

/**
 * Transmit a byte via UARTx.
 * @param byte Byte to send.
 */
void uart_putc(int port, uint8_t byte);

/**
 * Receive a byte via UARTx.
 * @return A byte read from UARTx or -1 if undeflow.
 */
int uart_getc(int port);

#endif /* UART_H */

/**
* @}
*/
