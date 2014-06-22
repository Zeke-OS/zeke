/**
 *******************************************************************************
 * @file    block.c
 * @author  Olli Vanhoja
 * @brief
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

#include <fs/block.h>

#define MAX_TRIES 2;

int block_read(vnode_t * vnode, const off_t * offset, void * vbuf, size_t count)
{
    struct block_dev * bdev = (struct block_dev *)vnode->vn_dev;
    uint8_t * buf = (uint8_t *)vbuf;

    if (!bdev->read)
        return 0;

    if ((bdev->flags & BDEV_FLAGS_MB_READ) && ((count / bdev->block_size) > 1))
        return bdev->read(bdev, *offset, buf, count);

    size_t buf_offset = 0;
    ssize_t block_offset = 0;
    do {
        int tries = MAX_TRIES;
        size_t to_read = (count > bdev->block_size) ? bdev->block_size : count;

        while (1) {
            int ret = bdev->read(bdev, *offset + block_offset,
                    &buf[buf_offset], to_read);
            if (ret < 0) {
                tries--;
                if (tries <= 0)
                    return ret;
            } else {
                break;
            }
        }

        buf_offset += to_read;
        block_offset++;

        if (count < bdev->block_size)
            count = 0;
        else
            count -= bdev->block_size;
    } while (count > 0);

    return buf_offset;
}

int block_write(vnode_t * file, const off_t * offset,
        const void * vbuf, size_t count)
{
    return -1;
}
