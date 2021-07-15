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

#ifndef TTY_H
#define TTY_H

#include <stddef.h>
#include <stdint.h>

struct file;
struct vnode;
struct termios;
struct winsize;

struct tty {
    struct termios conf;
    struct winsize winsize;

    off_t write_count;

    /**
     * A pointer back to the vnode.
     * Having this pointer prevents us having multiple vnode end points to a tty
     * device, though ttys are usually end-to-end anyway so I don't see a big
     * problem there.
     */
    struct vnode * tty_vn;

    void * opt_data;

    void (* setconf)(struct termios * conf);

    /**
     * @param blkno can be used for tty muxing, see pty driver.
     */
    ssize_t (*read)(struct tty * tty, off_t blkno, uint8_t * buf,
                    size_t bcount, int oflags);

    ssize_t (*write)(struct tty * tty, off_t blkno, uint8_t * buf,
                     size_t bcount, int oflags);

    /**
     * TTY opened callback.
     * @note Can be NULL.
     */
    void (*open_callback)(struct file * file, struct tty * tty);

    /**
     * TTY closed callback.
     * @note Can be NULL.
     */
    void (*close_callback)(struct file * file, struct tty * tty);

    /**
     * Overriding ioctl.
     * @note Can be NULL.
     */
    int (*ioctl)(struct dev_info * devnfo, uint32_t request,
                 void * arg, size_t arg_len);
};

/**
 * Allocate a prefilled dev + tty struct.
 * @param drv_name should be allocated from heap, referenced.
 * @param dev_name device name, copied.
 * @param data_size is an additional allocation made after the tty data that can
 *                  be used by the caller to store some driver specific data.
 *                  The idea is to simplify memory management and allocation
 *                  overhead by allocating memory in bigger blocks and less
 *                  often.
 */
struct tty * tty_alloc(const char * drv_name, dev_t dev_id,
                       const char * dev_name, size_t data_size);

/**
 * Free a device struct.
 * @note Pointer tty is invalid after calling this function so destroy_ttydev()
 *       must be called before calling this function if make_ttydev() was
 *       called for tty.
 */
void tty_free(struct tty * tty);

/**
 * Get the device info struct of a tty device.
 * @param tty is a pointer to the tty device.
 */
struct dev_info * tty_get_dev(struct tty * tty);

/**
 * Create a tty device.
 * Usually you can create multiple end points to a device by calling make_dev()
 * multiple times but make_ttydev() only supports one end point per struct tty.
 */
int make_ttydev(struct tty * tty);

/**
 * Destroy a tty device.
 * @note tty_free() must not be called before this function.
 */
void destroy_ttydev(struct tty * tty);

#endif /* TTY_H */
