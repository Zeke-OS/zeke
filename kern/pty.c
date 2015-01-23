/**
 *******************************************************************************
 * @file    pty.c
 * @author  Olli Vanhoja
 * @brief   Pseudo terminal driver.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/*
 * This is the pseudo terminal device driver for Zeke.
 * New ptys are created as per POSIX by calling posix_openpt(), we do
 * have a ptmx device file but there is no intention to support pty
 * creation by opening the file since we don't have the traditional open()
 * semantics inside the kernel.
 */

#define KERNEL_INTERNAL
#include <sys/tree.h>
#include <queue_r.h>
#include <kinit.h>
#include <klocks.h>
#include <errno.h>
#include <kmalloc.h>
#include <proc.h>
#include <fcntl.h>
#include <fs/devfs.h>
#include <fs/dev_major.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pty.h>

static const char drv_name[] = "PTY";

static struct tty * dev_ptmx;
static atomic_t _next_pty_id;
RB_HEAD(ptytree, pty_device) ptys_head = RB_INITIALIZER(_head);
mtx_t pty_lock;

static unsigned next_ptyid(void);
static struct pty_device * pty_get(unsigned id);
static int creat_pty(void);
static int make_ptyslave(int pty_id, struct tty * tty_slave);
static int ptymaster_read(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags);
static int ptymaster_write(struct tty * tty, off_t blkno,
                           uint8_t * buf, size_t bcount, int oflags);
static int ptyslave_read(struct tty * tty, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags);
static int ptyslave_write(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags);
static int pty_ioctl(struct dev_info * devnfo, uint32_t request,
                     void * arg, size_t arg_len);

static int ptydev_comp(struct pty_device * a, struct pty_device * b)
{
    return a->pty_id - b->pty_id;
}

RB_PROTOTYPE_STATIC(ptytree, pty_device, _entry, ptydev_comp);
RB_GENERATE_STATIC(ptytree, pty_device, _entry, ptydev_comp);

int pty_init(void) __attribute__((constructor));
int pty_init(void)
{
    dev_t dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, 0);
    char dev_name[] = "ptmx";

    SUBSYS_DEP(devfs_init);
    SUBSYS_INIT("pty");

    ATOMIC_INIT(_next_pty_id);
    RB_INIT(&ptys_head);
    mtx_init(&pty_lock, MTX_TYPE_TICKET);

    dev_ptmx = kcalloc(1, sizeof(struct tty));
    if (!dev_ptmx)
        return -ENOMEM;

    dev_ptmx->read = ptymaster_read;
    dev_ptmx->write = ptymaster_write;
    dev_ptmx->ioctl = pty_ioctl;

    if (make_ttydev(dev_ptmx, drv_name, dev_id, dev_name)) {
        KERROR(KERROR_ERR, "Failed to make /dev/ptmx");
        return -ENODEV;
    }

    return 0;
}

static unsigned next_ptyid(void)
{
    /* TODO Implement restart */
    return atomic_inc(&_next_pty_id);
}

static struct pty_device * pty_get(unsigned id)
{
    struct pty_device find = { .pty_id = id };
    struct pty_device * res = NULL;

    if (id == 0)
        goto out;

    mtx_lock(&pty_lock);
    res = RB_FIND(ptytree, &ptys_head, &find);
    mtx_unlock(&pty_lock);

out:
    return res;
}

/**
 * Create a new pty master/slave pair.
 */
static int creat_pty(void)
{
    const int pty_id = next_ptyid();
    struct pty_device * ptydev;
    int err;

    ptydev = kcalloc(1, sizeof(struct pty_device));
    if (!ptydev)
        return -ENOMEM;

    err = make_ptyslave(pty_id, &ptydev->tty_slave);
    if (err) {
        kfree(ptydev);
        return err;
    }

    ptydev->chbuf_ms = queue_create(ptydev->_cbuf_ms, sizeof(char),
            sizeof(ptydev->_cbuf_ms));
    ptydev->chbuf_sm = queue_create(ptydev->_cbuf_sm, sizeof(char),
            sizeof(ptydev->_cbuf_sm));

    mtx_lock(&pty_lock);
    RB_INSERT(ptytree, &ptys_head, ptydev);
    mtx_unlock(&pty_lock);

    return pty_id;
}

static int make_ptyslave(int pty_id, struct tty * tty_slave)
{
    dev_t dev_id;
    char dev_name[SPECNAMELEN];

    dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, pty_id);
    ksprintf(dev_name, sizeof(dev_name), "pty%i", pty_id);
    tty_slave->read = ptyslave_read;
    tty_slave->write = ptyslave_write;
    tty_slave->ioctl = pty_ioctl;

    /* TODO Should be created under pts? */
    if (make_ttydev(tty_slave, drv_name, dev_id, dev_name))
        return -ENODEV;

    return 0;
}

/*
 * TODO Check owner before read/write
 */

static int ptymaster_read(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * ptydev = pty_get(pty_id);
    size_t i;

    for (i = 0; i < bcount; i++) {
        if (!queue_pop(&ptydev->chbuf_sm, &buf[i]))
            break;
    }

    return i;
}

static int ptymaster_write(struct tty * tty, off_t blkno,
                           uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * ptydev = pty_get(pty_id);
    size_t i;

    for (i = 0; i < bcount; i++) {
        if (!queue_push(&ptydev->chbuf_ms, &buf[i]))
            break;
    }

    return i;
}

static int ptyslave_read(struct tty * tty, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * ptydev = pty_get(pty_id);
    size_t i;

    for (i = 0; i < bcount; i++) {
        if (!queue_pop(&ptydev->chbuf_ms, &buf[i]))
            break;
    }

    return i;
}

static int ptyslave_write(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * ptydev = pty_get(pty_id);
    size_t i;

    for (i = 0; i < bcount; i++) {
        if (!queue_push(&ptydev->chbuf_sm, &buf[i]))
            break;
    }

    return i;
}

static int pty_ioctl(struct dev_info * devnfo, uint32_t request,
                     void * arg, size_t arg_len)
{
    struct pty_device * pty_slave = pty_get(DEV_MINOR(devnfo->dev_id));

    if (!pty_slave && request == IOCTL_PTY_CREAT)
        return creat_pty();

    if (!pty_slave)
        return -EINVAL;

    switch (request) {
    default:
        return -EINVAL;
    }

    return 0;
}
