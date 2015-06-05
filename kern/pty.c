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
#include <kerror.h>
#include <kinit.h>
#include <klocks.h>
#include <kmalloc.h>
#include <libkern.h>
#include <proc.h>
#include <queue_r.h>
#include <tty.h>

/*
 * TODO Create all ptys at init and just reset the pty
 *      on close. This way we get rid of costly mem allocs
 *      and hard closing/removal procedures.
 */

/**
 * Struct describing a single PTY device.
 */
struct pty_device {
    int pty_id;
    struct tty * tty_slave;
    struct queue_cb chbuf_ms;
    struct queue_cb chbuf_sm;
    char _cbuf_ms[MAX_INPUT]; /* RFE is MAX_INPUT enough for pty? */
    char _cbuf_sm[MAX_INPUT];
    RB_ENTRY(pty_device) _entry;
};

static const char drv_name[] = "PTY";
static const char dev_name[] = "ptmx";

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

/*
 * TODO Check owner before read/write
 */

static int ptymaster_read(struct tty * tty, off_t blkno, uint8_t * buf,
                          size_t bcount, int oflags)
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

static int ptymaster_write(struct tty * tty, off_t blkno, uint8_t * buf,
                           size_t bcount, int oflags)
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
    struct pty_device * ptydev;
    size_t i;

    ptydev = container_of(tty, struct pty_device, tty_slave);

    for (i = 0; i < bcount; i++) {
        if (!queue_pop(&ptydev->chbuf_ms, &buf[i]))
            break;
    }

    return i;
}

static int ptyslave_write(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags)
{
    struct pty_device * ptydev;
    size_t i;

    ptydev = container_of(tty, struct pty_device, tty_slave);

    for (i = 0; i < bcount; i++) {
        if (!queue_push(&ptydev->chbuf_sm, &buf[i]))
            break;
    }

    return i;
}

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

    pty_id = next_ptyid();

    /*
     * Slave device id and name.
     */
    slave_dev_id = DEV_MMTODEV(VDEV_MJNR_PTY, pty_id);
    ksprintf(slave_dev_name, sizeof(slave_dev_name), "pty%i", pty_id);

    slave_tty = tty_alloc(drv_name, slave_dev_id, slave_dev_name,
                          sizeof(struct pty_device));
    if (!slave_tty)
        panic("Not enough memory to create a pty device");
    ptydev = (struct pty_device *)((uintptr_t)slave_tty + sizeof(struct tty));

    /*
     * Slave functions.
     */
    slave_tty->read = ptyslave_read;
    slave_tty->write = ptyslave_write;

    /*
     * Set master muxing id.
     */
    file->seek_pos = pty_id;

    /*
     * Create queues.
     */
    ptydev->chbuf_ms = queue_create(ptydev->_cbuf_ms, sizeof(char),
            sizeof(ptydev->_cbuf_ms));
    ptydev->chbuf_sm = queue_create(ptydev->_cbuf_sm, sizeof(char),
            sizeof(ptydev->_cbuf_sm));

    /* TODO a slave pty should be created under pts dir */
    if (make_ttydev(slave_tty)) {
        tty_free(slave_tty);
        panic("ENODEV while creating a pty");
    }

    mtx_lock(&pty_lock);
    RB_INSERT(ptytree, &ptys_head, ptydev);
    mtx_unlock(&pty_lock);
}

/*
 * TODO if user unlinks the pty slave we will leak some memory.
 * As a solution, we should have a delete event handler here
 * that can do the same as close. This may or may not require adding
 * a DELETING flag to somewhere.
 */
static void close_pty_slave(file_t * file, struct tty * tty)
{
    KERROR(KERROR_DEBUG, "pty master closed\n");
    /*
     * TODO The following code closes pty_slave from slave end but the proper
     * way is to close it when master end is closed.
     */
    /*struct pty_device * ptydev = pty_get(file->seek_pos);*/

#if 0
    vnode_t * vn = file->vnode;
    int (*delete_vnode)(vnode_t * vnode) = vn->sb->delete_vnode;
    struct pty_device * ptydev;

    KERROR(KERROR_DEBUG, "Delete pty slave vnode %d\n", (int)vn->vn_refcount);
    delete_vnode(vn);

    ptydev = container_of(tty, struct pty_device, tty_slave);
    mtx_lock(&pty_lock);
    RB_REMOVE(ptytree, &ptys_head, ptydev);
    mtx_unlock(&pty_lock);
    kfree(tty);
#endif
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

    dev_ptmx->read = ptymaster_read;
    dev_ptmx->write = ptymaster_write;
    dev_ptmx->open_callback = creat_pty;
    dev_ptmx->close_callback = close_pty_slave;

    if (make_ttydev(dev_ptmx)) {
        KERROR(KERROR_ERR, "Failed to make /dev/ptmx");
        tty_free(dev_ptmx);
        return -ENODEV;
    }

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
