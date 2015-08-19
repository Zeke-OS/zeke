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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/tree.h>
#include <termios.h>
#include <fs/dev_major.h>
#include <fs/devfs.h>
#include <fs/fs_util.h>
#include <kerror.h>
#include <kinit.h>
#include <klocks.h>
#include <kmalloc.h>
#include <libkern.h>
#include <proc.h>
#include <queue_r.h>
#include <tty.h>

static ssize_t ptymaster_read(struct file * file, struct uio * uio,
                              size_t count);
static ssize_t ptymaster_write(struct file * file, struct uio * uio,
                               size_t count);

static vnode_ops_t ptmx_vnode_ops = {
    .read = ptymaster_read,
    .write = ptymaster_write,
};

/**
 * Struct describing a single PTY device.
 */
struct pty_device {
    int pty_id;
    struct queue_cb chbuf_ms;
    struct queue_cb chbuf_sm;
    char _cbuf_ms[MAX_INPUT]; /* RFE is MAX_INPUT enough for pty? */
    char _cbuf_sm[MAX_INPUT];
    RB_ENTRY(pty_device) _entry;
};

static const char drv_name[] = "PTY";
static const char dev_name[] = "ptmx";

#define SLAVE_TTY2PTY(_tty_) \
    (struct pty_device *)((uintptr_t)(_tty_) + sizeof(struct tty))

#define SLAVE_PTY2TTY(_pty_) \
    (struct tty *)((uintptr_t)(_pty_) - sizeof(struct tty))

/*
 * pty state variables.
 */
static struct tty * dev_ptmx; /*!< PTY mux device. */
static atomic_t _next_pty_id; /*!< Next PTY id. */
RB_HEAD(ptytree, pty_device) ptys_head = RB_INITIALIZER(_head);
mtx_t pty_lock; /*!< Lock for protecting global PTY data. */

static int ptydev_comp(struct pty_device * a, struct pty_device * b)
{
    return a->pty_id - b->pty_id;
}

RB_PROTOTYPE_STATIC(ptytree, pty_device, _entry, ptydev_comp);
RB_GENERATE_STATIC(ptytree, pty_device, _entry, ptydev_comp);

static unsigned next_ptyid(void)
{
    /* TODO Implement restart for ptyid */
    return atomic_inc(&_next_pty_id);
}

/**
 * Get a pty device by id.
 */
static struct pty_device * pty_get(unsigned id)
{
    struct pty_device find = { .pty_id = id };
    struct pty_device * res;

    mtx_lock(&pty_lock);
    res = RB_FIND(ptytree, &ptys_head, &find);
    mtx_unlock(&pty_lock);

    return res;
}

/**
 * Insert a pty device.
 */
void pty_insert(struct pty_device * ptydev)
{
    mtx_lock(&pty_lock);
    RB_INSERT(ptytree, &ptys_head, ptydev);
    mtx_unlock(&pty_lock);
}

/**
 * Remove a pty device.
 */
void pty_remove(struct pty_device * ptydev)
{
    mtx_lock(&pty_lock);
    RB_REMOVE(ptytree, &ptys_head, ptydev);
    mtx_unlock(&pty_lock);
}

/*
 * TODO Check owner before read/write
 * TODO blocking mode
 */

static ssize_t ptymaster_read(struct file * file, struct uio * uio,
                              size_t count)
{
    int err;
    struct pty_device * ptydev = (struct pty_device *)file->stream;
    uint8_t * buf;
    ssize_t i;

    err = uio_get_kaddr(uio, (void **)(&buf));
    if (err)
        return err;

    for (i = 0; i < (ssize_t)count; i++) {
        if (!queue_pop(&ptydev->chbuf_sm, &buf[i]))
            break;
    }

    return i;
}

static ssize_t ptymaster_write(struct file * file, struct uio * uio,
                               size_t count)
{
    int err;
    struct pty_device * ptydev = (struct pty_device *)file->stream;
    uint8_t * buf;
    ssize_t i;

    err = uio_get_kaddr(uio, (void **)(&buf));
    if (err)
        return err;

    for (i = 0; i < (ssize_t)count; i++) {
        if (!queue_push(&ptydev->chbuf_ms, &buf[i]))
            break;
    }

    return i;
}

static int ptyslave_read(struct tty * tty, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags)
{
    struct pty_device * ptydev = SLAVE_TTY2PTY(tty);
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
    struct pty_device * ptydev = SLAVE_TTY2PTY(tty);
    size_t i;

    for (i = 0; i < bcount; i++) {
        if (!queue_push(&ptydev->chbuf_sm, &buf[i]))
            break;
    }

    return i;
}

/*
 * TODO if user unlinks the pty slave we will leak some memory.
 * As a solution, we should have a delete event handler here
 * that can do the same as close. This may or may not require adding
 * a DELETING flag to somewhere.
 */
/**
 * Create a new pty master/slave pair.
 */
static void creat_pty(file_t * file, struct tty * tty)
{
    int pty_id;
    dev_t slave_dev_id;
    char slave_dev_name[SPECNAMELEN];
    struct tty * slave_tty;
    struct pty_device * ptydev;

    file->stream = NULL; /* Just in case we fail to create a new pty device. */

    pty_id = next_ptyid();

    /*
     * Slave device id and name.
     */
    slave_dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, pty_id);
    ksprintf(slave_dev_name, sizeof(slave_dev_name), "pty%i", pty_id);

    slave_tty = tty_alloc(drv_name, slave_dev_id, slave_dev_name,
                          sizeof(struct pty_device));
    if (!slave_tty) {
        KERROR(KERROR_ERR, "%s(): Not enough memory to create a pty device",
               __func__);
        return;
    }
    ptydev = SLAVE_TTY2PTY(slave_tty);

    /*
     * Slave functions.
     */
    slave_tty->read = ptyslave_read;
    slave_tty->write = ptyslave_write;

    /*
     * Create queues.
     */
    ptydev->chbuf_ms = queue_create(ptydev->_cbuf_ms, sizeof(char),
                                    sizeof(ptydev->_cbuf_ms));
    ptydev->chbuf_sm = queue_create(ptydev->_cbuf_sm, sizeof(char),
                                    sizeof(ptydev->_cbuf_sm));

    if (make_ttydev(slave_tty)) {
        tty_free(slave_tty);
        KERROR(KERROR_ERR, "%s(): Failed to create a pty", __func__);
        return;
    }

    /*
     * seek_pos must be set to pty_id so the user space can figure out
     * the slave device name matching with this file descriptor.
     */
    file->seek_pos = pty_id;
    file->stream = ptydev; /* ptydev is the stream */

    pty_insert(ptydev);
}

/**
 * Close the pty slave end when the master end is closed.
 */
static void close_ptmx(file_t * file, struct tty * tty)
{
    struct pty_device * ptydev = (struct pty_device *)file->stream;
    struct tty * slave_tty;

    if (!ptydev) {
        KERROR(KERROR_ERR, "%s(): Pointer to ptydev missing\n", __func__);
        return;
    }

    slave_tty = SLAVE_PTY2TTY(ptydev);

    pty_remove(ptydev);

    destroy_ttydev(slave_tty);
    tty_free(slave_tty);
}

/**
 * Create the pty master mux device.
 * @note This function should be called only once, that's from
 * the pty init function.
 */
static int make_ptmx(void)
{
    dev_t dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, 0);

    dev_ptmx = tty_alloc(drv_name, dev_id, dev_name, 0);
    if (!dev_ptmx)
        return -ENOMEM;

    dev_ptmx->read = NULL;  /* Not needed because we override dev and tty. */
    dev_ptmx->write = NULL; /* -"- */
    dev_ptmx->open_callback = creat_pty;
    dev_ptmx->close_callback = close_ptmx;

    if (make_ttydev(dev_ptmx)) {
        KERROR(KERROR_ERR, "Failed to make /dev/ptmx");
        tty_free(dev_ptmx);
        return -ENODEV;
    }

    /*
     * We need to create our own vnode ops for ptymx to handle muxing and
     * remove unnecessary overhead caused by devfs + tty abstraction.
     */
    fs_inherit_vnops(&ptmx_vnode_ops, dev_ptmx->tty_vn->vnode_ops);
    dev_ptmx->tty_vn->vnode_ops = &ptmx_vnode_ops; /* and replace. */

    return 0;
}

int __kinit__ pty_init(void)
{
    SUBSYS_DEP(devfs_init);
    SUBSYS_INIT("pty");

    _next_pty_id = ATOMIC_INIT(0);
    RB_INIT(&ptys_head);
    mtx_init(&pty_lock, MTX_TYPE_TICKET, 0);

    return make_ptmx();
}
