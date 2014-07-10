/**
 *******************************************************************************
 * @file    uart.c
 * @author  Olli Vanhoja
 * @brief   UART source code for BCM2835.
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

#include <kinit.h>
#include <kerror.h>
#include "bcm2835_mmio.h"
#include "bcm2835_gpio.h"
#include "bcm2835_timers.h"
#include <hal/core.h>
#include <hal/uart.h>

/* Addresses */
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

/* Bit offsets and masks */
#define UART0_LCRH_SPS_OFFSET   7
#define UART0_LCRH_WLEN_OFFSET  5
#define UART0_LCRH_FEN_OFFSET   4
#define UART0_LCRH_STP2_OFFSET  3
#define UART0_LCRH_EPS_OFFSET   2
#define UART0_LCRH_PEN_OFFSET   1
#define UART0_LCRH_BRK_OFFSET   0

#define UART0_FR_TXFE_OFFSET    7
#define UART0_FR_RXFF_OFFSET    6
#define UART0_FR_TXFF_OFFSET    5
#define UART0_FR_RXFE_OFFSET    4
#define UART0_FR_BUSY_OFFSET    3
#define UART0_FR_CTS_OFFSET     0

static void bcm2835_uart_init(struct uart_port * port);
static void set_baudrate(unsigned int baud_rate);
static void set_lcrh(const struct uart_port_conf * conf);
void bcm2835_uart_uputc(struct uart_port * port, uint8_t byte);
int bcm2835_uart_ugetc(struct uart_port * port);

static struct uart_port port = {
    .init = bcm2835_uart_init,
    .uputc = bcm2835_uart_uputc,
    .ugetc = bcm2835_uart_ugetc
};


void bcm2835_uart_register(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(arm_interrupt_preinit);

    uart_register_port(&port);

    SUBSYS_INITFINI("BCM2835 UART Registered");
}
HW_PREINIT_ENTRY(bcm2835_uart_register);

void bcm2835_uart_init(struct uart_port * port)
{
    struct uart_port_conf * conf = &port->conf;
    istate_t s_entry;

    mmio_start(&s_entry);

    /* Disable UART0. */
    mmio_write(UART0_CR, 0x00000000);

    /* Setup the GPIO pin 14 & 15. */

    /* Disable pull up/down for all GPIO pins & delay for 150 cycles. */
    mmio_write(GPIO_GPPUD, 0x00000000);
    bcm_udelay(150); /* Not 150 cycles anymore but it should work anyway. */

    /* Disable pull up/down for pin 14, 15 and delay for 150 cycles. */
    mmio_write(GPIO_PUDCLK0, (1 << 14) | (1 << 15));
    bcm_udelay(150);

    /* Write 0 to GPPUDCLK0 to make it take effect.
     * (only affects to pins 14 & 15) */
    mmio_write(GPIO_PUDCLK0, 0x00000000);

    /* Clear pending interrupts. */
    mmio_write(UART0_ICR, 0x7FF);

    mmio_end(&s_entry);


    set_baudrate(conf->baud_rate); /* Set baud rate */
    set_lcrh(conf); /* Configure UART */


    mmio_start(&s_entry);

    /* Mask all interrupts. */
    /*mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) |
            (1 << 6) | (1 << 7) | (1 << 8) |
            (1 << 9) | (1 << 10));*/

    /* Enable UART0, receive & transfer part of UART.*/
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));

    mmio_end(&s_entry);
}


static void set_baudrate(unsigned int baud_rate)
{
    /* Integer & fractional part of baud rate.
     * divider = UART_CLOCK / (16 * 115200)
     * fraction = (divider mod 1 * 64) + 0.5
     * UART_CLOCK = 3000000
     */
    uint32_t tmp = 3000000/(16 * ((uint32_t)baud_rate >> 6));
    uint32_t divider = tmp >> 6;
    uint32_t fraction = tmp - (divider << 6);
    istate_t s_entry;

    mmio_start(&s_entry);
    mmio_write(UART0_IBRD, divider);
    mmio_write(UART0_FBRD, fraction);
    mmio_end(&s_entry);
}

static void set_lcrh(const struct uart_port_conf * conf)
{
    uint32_t tmp = 0;
    istate_t s_entry;

    /* Enable FIFOs */
    tmp |= 1 << UART0_LCRH_FEN_OFFSET;

    switch (conf->data_bits) {
    case UART_DATABITS_5:
        /* NOP */
        break;
    case UART_DATABITS_6:
        tmp |= (0x1 << UART0_LCRH_WLEN_OFFSET);
        break;
    case UART_DATABITS_7:
        tmp |= (0x2 << UART0_LCRH_WLEN_OFFSET);
        break;
    case UART_DATABITS_8:
        tmp |= (0x3 << UART0_LCRH_WLEN_OFFSET);
        break;
    }

    switch (conf->parity) {
    case UART_PARITY_NO:
        /* NOP */
        break;
    case UART_PARITY_EVEN:
        tmp |= (1 << UART0_LCRH_EPS_OFFSET);
        break;
    case UART_PARITY_ODD:
        tmp |= (1 << UART0_LCRH_PEN_OFFSET);
        break;
    }

    mmio_start(&s_entry);
    mmio_write(UART0_LCRH, tmp);
    mmio_end(&s_entry);
}

void bcm2835_uart_uputc(struct uart_port * port, uint8_t byte)
{
    istate_t s_entry;

    /* Wait for UART to become ready to transmit. */
    while (1) {
        bcm_udelay(100); /* Seems to work slightly better with this */
        mmio_start(&s_entry);
        if (!(mmio_read(UART0_FR) & (1 << UART0_FR_TXFF_OFFSET))) {
            break;
        }
        mmio_end(&s_entry);
    }

    mmio_write(UART0_DR, byte);
    mmio_end(&s_entry);
}

int bcm2835_uart_ugetc(struct uart_port * port)
{
    int byte = -1;
    istate_t s_entry;

    mmio_start(&s_entry);

    /* Check that receive FIFO/register is not empty. */
    if(!(mmio_read(UART0_FR) & (1 << UART0_FR_RXFE_OFFSET))) {
        byte = mmio_read(UART0_DR);
    }

    mmio_end(&s_entry);

    return byte;
}
