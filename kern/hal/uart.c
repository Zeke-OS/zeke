/**
 *******************************************************************************
 * @file    uart.c
 * @author  Olli Vanhoja
 * @brief   UART HAL.
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

#include <errno.h>
#include <kinit.h>
#include <kstring.h>
#include <kmalloc.h>
#include <fs/devfs.h>
#include <hal/uart.h>

static const char drv_name[] = "UART";

static struct uart_port * uart_ports[UART_PORTS_MAX];
static int uart_nr_ports;
static int vfs_ready;

static int make_uartdev(struct uart_port * port, int port_num);
static int uart_read(struct dev_info * devnfo, off_t offset, uint8_t * buf,
                     size_t count);
static int uart_write(struct dev_info * devnfo, off_t offset, uint8_t * buf,
                      size_t count);

void uart_init(void) __attribute__((constructor));
void uart_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(devfs_init);
    vfs_ready = 1;

    for (int i = 0; i < uart_nr_ports; i++) {
        struct uart_port * port = uart_ports[i];

        if (!(port->flags & UART_PORT_FLAG_FS))
            make_uartdev(port, i);
    }

    SUBSYS_INITFINI("uart OK");
}

static int make_uartdev(struct uart_port * port, int port_num)
{
    struct dev_info * dev = kcalloc(1, sizeof(struct dev_info));

    if (!dev)
        return -ENOMEM;

    dev->dev_id = DEV_MMTODEV(4, port_num);
    dev->drv_name = drv_name;
    ksprintf(dev->dev_name, sizeof(dev->dev_name), "ttyS%i", port_num);
    dev->flags = DEV_FLAGS_WR_BT_MASK;
    dev->block_size = 1;
    dev->read = uart_read;
    dev->write = uart_write;

    if (dev_make(dev, 0, 0, 0666)) {
        KERROR(KERROR_ERR, "Failed to");
    } else {
        port->flags |= UART_PORT_FLAG_FS;
    }

    return -ENODEV;
}

int uart_register_port(struct uart_port * port)
{
    int i, retval = -1;

    i = uart_nr_ports;
    if (i < UART_PORTS_MAX) {
        uart_ports[i] = port;
        uart_nr_ports++;
        retval = i;
    }

    if (vfs_ready)
        make_uartdev(port, i);

    return retval;
}

int uart_nports(void)
{
    return uart_nr_ports;
}

struct uart_port * uart_getport(int port_num)
{
    struct uart_port * retval = 0;

    if (port_num < uart_nr_ports)
        retval = uart_ports[port_num];

    return retval;
}

static int uart_read(struct dev_info * devnfo, off_t offset, uint8_t * buf,
                     size_t count)
{
    struct uart_port * port = uart_getport(DEV_MINOR(devnfo->dev_id));
    int ret;

    if (!port)
        return -ENODEV;

    ret = port->ugetc(port);
    if (ret == -1)
        return -EAGAIN;

    *buf = (char)ret;
    return 1;
}

static int uart_write(struct dev_info * devnfo, off_t offset, uint8_t * buf,
                      size_t count)
{
    struct uart_port * port = uart_getport(DEV_MINOR(devnfo->dev_id));

    if (!port)
        return -ENODEV;

    port->uputc(port, *buf);

    return 1;
}
