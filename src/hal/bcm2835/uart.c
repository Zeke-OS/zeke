/**
 *******************************************************************************
 * @file    uart.c
 * @author  Olli Vanhoja
 * @brief   Headers for BCM2835 UART.
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

/** @addtogroup BCM2835
* @{
*/

#include <kerror.h>
#include "mmio.h"
#include "uart.h"

/* TODO Maybe GPIO should be somewhere else? */
#define GPIO_BASE       0x20200000
/** Pull up/down control of ALL GPIO pins. */
#define GPPUD           (GPIO_BASE + 0x94)
/** Pull up/down control for specific GPIO pin. */
#define GPPUDCLK0       (GPIO_BASE + 0x98)

#define UART0_BASE      0x20201000
#define UART0_DR        (UART0_BASE + 0x00)
#define UART0_RSRECR    (UART0_BASE + 0x04)
#define UART0_FR        (UART0_BASE + 0x18)
#define UART0_ILPR      (UART0_BASE + 0x20)
#define UART0_IBRD      (UART0_BASE + 0x24)
#define UART0_FBRD      (UART0_BASE + 0x28)
#define UART0_LCRH      (UART0_BASE + 0x2C)
#define UART0_CR        (UART0_BASE + 0x30)
#define UART0_IFLS      (UART0_BASE + 0x34)
#define UART0_IMSC      (UART0_BASE + 0x38)
#define UART0_RIS       (UART0_BASE + 0x3C)
#define UART0_MIS       (UART0_BASE + 0x40)
#define UART0_ICR       (UART0_BASE + 0x44)
#define UART0_DMACR     (UART0_BASE + 0x48)
#define UART0_ITCR      (UART0_BASE + 0x80)
#define UART0_ITIP      (UART0_BASE + 0x84)
#define UART0_ITOP      (UART0_BASE + 0x88)
#define UART0_TDR       (UART0_BASE + 0x8C)

/**
 * Idiotic delay function.
 * @param count delay time.
 */
static void delay(int32_t count) {
    __asm__ volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
    : : [count]"r"(count) : "cc");
}

/**
 * Initialize UART.
 * @param port Port number.
 */
void uart_init(int port)
{
    if (port != 0) {
        KERROR(KERROR_ERR, "We can only init UART0!");
    }

    mmio_start();

    /* Disable UART0. */
    mmio_write(UART0_CR, 0x00000000);

    /* Setup the GPIO pin 14 & 15. */

    /* Disable pull up/down for all GPIO pins & delay for 150 cycles. */
    mmio_write(GPPUD, 0x00000000);
    delay(150);

    /* Disable pull up/down for pin 14, 15 and delay for 150 cycles. */
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);

    /* Write 0 to GPPUDCLK0 to make it take effect. */
    mmio_write(GPPUDCLK0, 0x00000000);

    /* Clear pending interrupts. */
    mmio_write(UART0_ICR, 0x7FF);

    /* Set integer & fractional part of baud rate.
     * Divider = UART_CLOCK/(16 * Baud)
     * Fraction part register = (Fractional part * 64) + 0.5
     * UART_CLOCK = 3000000; Baud = 115200.
     */

    /* Divider = 3000000/(16 * 115200) = 1.627 = ~1.
     * Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
     */
    mmio_write(UART0_IBRD, 1);
    mmio_write(UART0_FBRD, 40);

    /* Enable FIFO & 8 bit data transmissio (1 stop bit, no parity). */
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    /* Mask all interrupts. */
    mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) |
            (1 << 6) | (1 << 7) | (1 << 8) |
            (1 << 9) | (1 << 10));

    /* Enable UART0, receive & transfer part of UART.*/
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

/*
 * Transmit a byte via UARTx.
 * @param byte Byte to send.
 */
void uart_putc(int port, uint8_t byte)
{
    mmio_start();

    /* Wait for UART to become ready to transmit. */
    while (1) {
        if (!(mmio_read(UART0_FR) & (1 << 5))) {
            break;
        }
    }
    mmio_write(UART0_DR, byte);

    mmio_end();
}

/**
* @}
*/

/**
* @}
*/
