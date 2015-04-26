/**
 *******************************************************************************
 * @file    devfs.h
 * @author  Olli Vanhoja
 * @brief   Device interface headers.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef DEVFS_H
#define DEVFS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/param.h>
#include <fs/fs.h>

#define DEVFS_FSNAME            "devfs" /*!< Name of the devfs in vfs. */

#define DEV_FLAGS_MB_READ       0x01 /*!< Supports multiple block read. */
#define DEV_FLAGS_MB_WRITE      0x02 /*!< Supports multiple block write. */
#define DEV_FLAGS_WR_BT_MASK    0x04 /*!< 0 = Write-back; 1 = Write-through */

struct dev_info {
    dev_t dev_id;           /*!< Device id (major, minor). */
    const char * drv_name;  /*!< Name of the driver associated with the dev. */
    char dev_name[SPECNAMELEN]; /*!< File name of the device. */

    uint32_t flags;         /*!< Configuration flags. */

    size_t block_size;      /*!< Preferred block transfer size. */
    ssize_t num_blocks;

    void * opt_data; /*!< Optional device data internal to the driver. */

    ssize_t (*read)(struct dev_info * devnfo, off_t blkno,
                    uint8_t * buf, size_t bcount, int oflags);
    ssize_t (*write)(struct dev_info * devnfo, off_t blkno,
                     uint8_t * buf, size_t bcount, int oflags);
    /**
     * ioctl for the device driver.
     * @note This function is optional and can be NULL.
     */
    int (*ioctl)(struct dev_info * devnfo, uint32_t request,
                 void * arg, size_t arg_len);

    /**
     * mmap a device.
     * @note This function is optional and can be NULL.
     */
    int (*mmap)(struct dev_info * devnfo, size_t blkno, size_t bsize, int flags,
                struct buf ** bp_out);

    /**
     * The function is called if set and vnode deletion is triggered by
     * one of the vnode release functions.
     */
    void (*delete_vnode_callback)(struct dev_info * devnfo);

    /**
     * The function is called whenever a file associated with tihs device is
     * opened.
     * @note Can be NULL.
     * @return Return 0 if opening the device is ok; Otherwise a negative
     * errno shall be returned.
     */
    int (*opened_callback)(struct proc_info * p, struct dev_info * devnfo);

    /**
     * The function is called whenever a file associated with this device is
     * closed.
     * @note Can be NULL.
     */
    void (*closed_callback)(struct proc_info * p, struct dev_info * devnfo);
};

void _devfs_create_specials(void);

int dev_make(struct dev_info * devnfo, uid_t uid, gid_t gid, int perms,
        vnode_t ** result);

/**
 * Get a string indicating the device name by device vnode.
 * @param dev is a vnode representing a device.
 */
const char * devtoname(struct vnode * dev);

/**
 * Read from a device.
 * @param file      is a pointer to the device file.
 * @param vbuf      is the target buffer.
 * @param count     is the byte count to read.
 * @return  Returns number of bytes read from the device.
 */
ssize_t dev_read(file_t * file, void * vbuf, size_t bcount);

/**
 * Write to a device.
 * @param file      is a pointer to the device file.
 * @param vbuf      is the source buffer.
 * @param count     is the byte count to write.
 * @return  Returns number of bytes written to the device.
 */
ssize_t dev_write(file_t * file, const void * vbuf, size_t bcount);

/**
 * Device control.
 * @param file      is a pointer the device file written.
 * @param request   is a request code, same as in user space.
 * @param arg       is a pointer to the argument (struct).
 * @return  Returns 0 if operation succeed;
 *          Otherwise a negative value representing errno.
 */
int ioctl(file_t * file, uint32_t request, void * arg);

#endif /* DEVFS_H */
