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

static struct dev_info * dev_ptmx;
static atomic_t _next_pty_id;
RB_HEAD(ptytree, pty_device) ptys_head = RB_INITIALIZER(_head);
mtx_t pty_lock;

static unsigned next_ptyid(void);
static struct pty_device * pty_get(unsigned id);
static int creat_pty(void);
static int make_ptyslave(int pty_id, struct dev_info * slave);
static int ptymaster_read(struct dev_info * devnfo, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags);
static int ptymaster_write(struct dev_info * devnfo, off_t blkno,
                           uint8_t * buf, size_t bcount, int oflags);
static int ptyslave_read(struct dev_info * devnfo, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags);
static int ptyslave_write(struct dev_info * devnfo, off_t blkno,
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
    SUBSYS_DEP(devfs_init);
    SUBSYS_INIT("pty");

    ATOMIC_INIT(_next_pty_id);
    RB_INIT(&ptys_head);
    mtx_init(&pty_lock, MTX_TYPE_TICKET);

    dev_ptmx = kcalloc(1, sizeof(struct dev_info));
    if (!dev_ptmx)
        return -ENOMEM;

    dev_ptmx->dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, 0);
    dev_ptmx->drv_name = drv_name;
    strcpy(dev_ptmx->dev_name, "ptmx");
    dev_ptmx->flags = DEV_FLAGS_MB_READ | DEV_FLAGS_WR_BT_MASK;
    dev_ptmx->block_size = 1;
    dev_ptmx->read = ptymaster_read;
    dev_ptmx->write = ptymaster_write;
    dev_ptmx->ioctl = pty_ioctl;

    if (dev_make(dev_ptmx, 0, 0, 0666, NULL)) {
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

static int creat_pty(void)
{
    const int pty_id = next_ptyid();
    struct pty_device * ptydev;
    int err;

    ptydev = kcalloc(1, sizeof(struct pty_device));
    if (!ptydev)
        return -ENOMEM;

    err = make_ptyslave(pty_id, &ptydev->slave);
    if (err) {
        kfree(ptydev);
        return err;
    }

    mtx_lock(&pty_lock);
    RB_INSERT(ptytree, &ptys_head, ptydev);
    mtx_unlock(&pty_lock);

    return pty_id;
}

static int make_ptyslave(int pty_id, struct dev_info * slave)
{
    struct dev_info * dev = slave;

    dev->dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, pty_id);
    dev->drv_name = drv_name;
    ksprintf(dev->dev_name, sizeof(dev->dev_name), "pty%i", pty_id);
    dev->flags = DEV_FLAGS_MB_READ | DEV_FLAGS_WR_BT_MASK;
    dev->block_size = 1;
    dev->read = ptyslave_read;
    dev->write = ptyslave_write;
    dev->ioctl = pty_ioctl;

    /* TODO Should be created under pts? */
    if (dev_make(dev, 0, 0, 0666, NULL)) {
        KERROR(KERROR_ERR, "Failed to make a pty slave.\n");
        return -ENODEV;
    }

    return 0;
}

/*
 * TODO Check owner before read/write
 */

static int ptymaster_read(struct dev_info * devnfo, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * pty = pty_get(pty_id);

    return 0;
}

static int ptymaster_write(struct dev_info * devnfo, off_t blkno,
                           uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * pty = pty_get(pty_id);

    return bcount;
}

static int ptyslave_read(struct dev_info * devnfo, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * pty = pty_get(pty_id);

    return 0;
}

static int ptyslave_write(struct dev_info * devnfo, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags)
{
    int pty_id = blkno;
    struct pty_device * pty = pty_get(pty_id);

    return bcount;
}

static int pty_ioctl(struct dev_info * devnfo, uint32_t request,
                     void * arg, size_t arg_len)
{
    struct pty_device * ptydev = pty_get(DEV_MINOR(devnfo->dev_id));
    int err;

    if (!ptydev && request == IOCTL_PTY_CREAT) {
        int ret;

        if (arg_len < sizeof(int))
            return -EINVAL;

        ret = creat_pty();
        if (ret < 0)
            return ret;
        *((int *)arg) = ret;

        return 0;
    }

    if (!ptydev)
        return -EINVAL;

    switch (request) {
    case IOCTL_GTERMIOS:
        if (arg_len < sizeof(struct termios))
            return -EINVAL;

        memcpy(arg, &(ptydev->conf), sizeof(struct termios));
        break;

    case IOCTL_STERMIOS:
        if (arg_len < sizeof(struct termios))
            return -EINVAL;

        err = priv_check(curproc, PRIV_TTY_SETA);
            if (err)
                return err;

        memcpy(&(ptydev->conf), arg, sizeof(struct termios));
        break;

    default:
        return -EINVAL;
    }

    return 0;
}
