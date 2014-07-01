/**
 *******************************************************************************
 * @file    devfs.h
 * @author  Olli Vanhoja
 * @brief   Device interface headers.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <fs/fs.h>

#define DEVFS_FSNAME            "devfs"
#define DEVFS_VDEV_MAJOR_ID     11

#define DEV_FLAGS_MB_READ       0x01 /*!< Supports multiple block read. */
#define DEV_FLAGS_MB_WRITE      0x02 /*!< Supports multiple block write. */
#define DEV_FLAGS_WR_BT_MASK    0x04 /*!< 0 = Write-back; 1 = Write-through */

struct dev_info {
    dev_t dev_id;
    char * drv_name;
    char dev_name[20];

    /*!< Configuration flags for block device handling */
    uint32_t flags;

    size_t block_size;
    ssize_t num_blocks;

    int (*read)(struct dev_info * devnfo, off_t offset,
            uint8_t * buf, size_t count);
    int (*write)(struct dev_info * devnfo, off_t offset,
            uint8_t * buf, size_t count);
};

/**
 * Read from a device.
 * @param vnode     is a vnode pointing to the device.
 * @param offset    is the offset to start from.
 * @param vbuf      is the target buffer.
 * @param count     is the byte count to read.
 * @return  Returns number of bytes read from the device.
 */
size_t dev_read(vnode_t * vnode, const off_t * offset,
        void * vbuf, size_t count);
/**
 * Write to a device.
 * @param vnode     is a vnode pointing to the device.
 * @param offset    is the offset to start from.
 * @param vbuf      is the source buffer.
 * @param count     is the byte count to write.
 * @return  Returns number of bytes written to the device.
 */
size_t dev_write(vnode_t * vnode, const off_t * offset,
        const void * vbuf, size_t count);

#endif /* DEVFS_H */
