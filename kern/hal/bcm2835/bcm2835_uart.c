/**
 *******************************************************************************
 * @file    uart.c
 * @author  Olli Vanhoja
 * @brief   UART source code for BCM2835.
 * @section LICENSE
 * Copyright (c) 2013 - 2015, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <hal/irq.h>
#include <kinit.h>
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

#define BCM2835_INT_CTS 0x002
#define BCM2835_INT_RX  0x010
#define BCM2835_INT_TX  0x020
#define BCM2835_INT_RT  0x040
#define BCM2835_INT_FE  0x080
#define BCM2835_INT_PE  0x100
#define BCM2835_INT_BE  0x200
#define BCM2835_INT_OE  0x400

static void bcm2835_uart_setconf(struct termios * conf);
static void set_baudrate(unsigned int baud_rate);
static void set_lcrh(const struct termios * conf);
int bcm2835_uart_uputc(struct uart_port * port, uint8_t byte);
int bcm2835_uart_ugetc(struct uart_port * port);
int bcm2835_uart_peek(struct uart_port * port);

static struct uart_port port = {
    .setconf = bcm2835_uart_setconf,
    .uputc = bcm2835_uart_uputc,
    .ugetc = bcm2835_uart_ugetc,
    .peek = bcm2835_uart_peek
};

int bcm2835_uart_register(void)
{
    SUBSYS_DEP(arm_interrupt_preinit);
    SUBSYS_INIT("BCM2836 UART");

    uart_register_port(&port);

    return 0;
}
HW_PREINIT_ENTRY(bcm2835_uart_register);

static uint32_t mis; /* TODO Remove */
static enum irq_ack bcm2835_uart_irq_ack(int irq)
{
    istate_t s_entry;

    KERROR (KERROR_DEBUG, "hello\n");
    mmio_start(&s_entry);
    mis = mmio_read(UART0_MIS);
    mmio_write(UART0_ICR, mis);
    mmio_end(&s_entry);

    return IRQ_WAKE_THREAD;
}

static void bcm2845_uart_irq_handle(int irq)
{
    KERROR (KERROR_DEBUG, "%x\n", mis);
}

static struct irq_handler bcm2835_uart_irq_handler = {
    .name = "BCM2835 UART",
    .ack = bcm2835_uart_irq_ack,
    .handle = bcm2845_uart_irq_handle,
};

static void bcm2835_uart_setconf(struct termios * conf)
{
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


    set_baudrate(conf->c_ospeed); /* Set baud rate */
    set_lcrh(conf); /* Configure UART */


    mmio_start(&s_entry);

#if 0
    /* Enable interrupts. */
    mmio_write(UART0_IMSC,
               BCM2835_INT_RX |
               BCM2835_INT_RT |
               BCM2835_INT_FE |
               BCM2835_INT_PE |
               BCM2835_INT_BE |
               BCM2835_INT_OE);
#endif

    /* Enable UART0, receive & transfer part of the UART.*/
    mmio_write(UART0_CR,
               (1 << 0) |                               /* UART Enable */
               (1 << 8) |                               /* TX Enable */
               (conf->c_cflag & CREAD) ? (1 << 9) : 0   /* RX Enable */
    );

    mmio_end(&s_entry);

    /* TODO Define for the irq num? */
    //irq_register(57, &bcm2835_uart_irq_handler);
}

static void set_baudrate(unsigned int baud_rate)
{
    /*
     * Integer & fractional part of baud rate.
     * divider = UART_CLOCK / (16 * 115200)
     * fraction = (divider mod 1 * 64) + 0.5
     * UART_CLOCK = 3000000
     */
    uint32_t tmp = 3000000 / (16 * ((uint32_t)baud_rate >> 6));
    uint32_t divider = tmp >> 6;
    uint32_t fraction = tmp - (divider << 6);
    istate_t s_entry;

    mmio_start(&s_entry);
    mmio_write(UART0_IBRD, divider);
    mmio_write(UART0_FBRD, fraction);
    mmio_end(&s_entry);
}

static void set_lcrh(const struct termios * conf)
{
    uint32_t tmp = 0;
    istate_t s_entry;

    /* Enable FIFOs */
    tmp |= 1 << UART0_LCRH_FEN_OFFSET;

    switch (conf->c_cflag & CSIZE) {
    case CS5:
        /* NOP */
        break;
    case CS6:
        tmp |= (0x1 << UART0_LCRH_WLEN_OFFSET);
        break;
    case CS7:
        tmp |= (0x2 << UART0_LCRH_WLEN_OFFSET);
        break;
    case CS8:
        tmp |= (0x3 << UART0_LCRH_WLEN_OFFSET);
        break;
    }

    if (conf->c_cflag & PARENB) {
        if (conf->c_cflag & PARODD)
            tmp |= (1 << UART0_LCRH_PEN_OFFSET);
        else /* even */
            tmp |= (1 << UART0_LCRH_EPS_OFFSET);
    }

    mmio_start(&s_entry);
    mmio_write(UART0_LCRH, tmp);
    mmio_end(&s_entry);
}

int bcm2835_uart_uputc(struct uart_port * port, uint8_t byte)
{
    istate_t s_entry;
    int retval;

    bcm_udelay(100); /* Seems to work slightly better with this */
    mmio_start(&s_entry);

    /* Return if buffer is full */
    if ((mmio_read(UART0_FR) & (1 << UART0_FR_TXFF_OFFSET))) {
        retval = -1;
        goto out;
    }

    mmio_write(UART0_DR, byte);
    retval = 0;
out:
    mmio_end(&s_entry);

    return retval;
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

int bcm2835_uart_peek(struct uart_port * port)
{
    istate_t s_entry;
    int retval;

    mmio_start(&s_entry);
    retval = !(mmio_read(UART0_FR) & (1 << UART0_FR_RXFE_OFFSET));
    mmio_end(&s_entry);

    return retval;
}
