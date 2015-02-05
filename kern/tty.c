/**
 *******************************************************************************
 * @file    tty.c
 * @author  Olli Vanhoja
 * @brief   Generic tty.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kmalloc.h>
#include <kerror.h>
#include <errno.h>
#include <proc.h>
#include <sys/priv.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fs/devfs.h>
#include <tty.h>

static ssize_t tty_read(struct dev_info * devnfo, off_t blkno, uint8_t * buf,
                        size_t bcount, int oflags);
static ssize_t tty_write(struct dev_info * devnfo, off_t blkno, uint8_t * buf,
                         size_t bcount, int oflags);
static int tty_ioctl(struct dev_info * devnfo, uint32_t request,
                     void * arg, size_t arg_len);

int make_ttydev(struct tty * tty, const char * drv_name, dev_t dev_id,
                const char * dev_name)
{
    struct dev_info * dev;

    dev = kcalloc(1, sizeof(struct dev_info));
    if (!dev)
        return -ENOMEM;

    dev->dev_id = dev_id;
    dev->drv_name = drv_name;
    strcpy(dev->dev_name, dev_name);
    dev->flags = DEV_FLAGS_MB_READ | DEV_FLAGS_WR_BT_MASK;
    dev->block_size = 1;
    dev->read = tty_read;
    dev->write = tty_write;
    dev->ioctl = tty_ioctl;
    dev->opt_data = tty;

    if (dev_make(dev, 0, 0, 0666, NULL)) {
        KERROR(KERROR_ERR, "Failed to make a tty dev.\n");
        return -ENODEV;
    }

    return 0;
}

static ssize_t tty_read(struct dev_info * devnfo, off_t blkno, uint8_t * buf,
                        size_t bcount, int oflags)
{
    struct tty * tty = (struct tty *)devnfo->opt_data;

    KASSERT(tty, "opt_data should have tty");

    return tty->read(tty, blkno, buf, bcount, oflags);
}

static ssize_t tty_write(struct dev_info * devnfo, off_t blkno, uint8_t * buf,
                         size_t bcount, int oflags)
{
    struct tty * tty = (struct tty *)devnfo->opt_data;

    KASSERT(tty, "opt_data should have tty");

    return tty->write(tty, blkno, buf, bcount, oflags);
}

static int tty_ioctl(struct dev_info * devnfo, uint32_t request,
                     void * arg, size_t arg_len)
{
    int err;
    struct tty * tty = (struct tty *)(devnfo->opt_data);

    if (!tty)
        return -EINVAL;

    /*
     * First call ioctl of the device driver since it may override some ioctls
     * defined here.
     */
    if (tty->ioctl) {
        err = tty->ioctl(devnfo, request, arg, arg_len);
        if (err == 0 || err != -EINVAL)
            return err;
    }

    switch (request) {
    case IOCTL_GTERMIOS:
        if (arg_len < sizeof(struct termios))
            return -EINVAL;

        memcpy(arg, &(tty->conf), sizeof(struct termios));
        break;

    case IOCTL_STERMIOS:
        if (arg_len < sizeof(struct termios))
            return -EINVAL;

        err = priv_check(curproc, PRIV_TTY_SETA);
        if (err)
            return err;

        memcpy(&(tty->conf), arg, sizeof(struct termios));
        tty->setconf(&tty->conf);
        break;

    default:
        return -EINVAL;
    }

    return 0;
}
