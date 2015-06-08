/**
 *******************************************************************************
 * @file    devspecial.c
 * @author  Olli Vanhoja
 * @brief   Special pseudo devices.
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

#include <kinit.h>
#include <kstring.h>
#include <kerror.h>
#include <fs/dev_major.h>
#include <fs/devfs.h>
#include <fs/devspecial.h>

struct dev_info devnull_info = {
    .dev_id = DEV_MMTODEV(VDEV_MJNR_SPECIAL, 3),
    .drv_name = "memdev",
    .dev_name = "null",
    .flags = DEV_FLAGS_MB_READ | DEV_FLAGS_MB_WRITE | DEV_FLAGS_WR_BT_MASK,
    .read = devnull_read,
    .write = devnull_write
};

struct dev_info devzero_info = {
    .dev_id = DEV_MMTODEV(VDEV_MJNR_SPECIAL, 5),
    .drv_name = "memdev",
    .dev_name = "zero",
    .flags = DEV_FLAGS_MB_READ | DEV_FLAGS_MB_WRITE | DEV_FLAGS_WR_BT_MASK,
    .read = devzero_read,
    .write = devzero_write
};

struct dev_info devfull_info = {
    .dev_id = DEV_MMTODEV(VDEV_MJNR_SPECIAL, 7),
    .drv_name = "memdev",
    .dev_name = "full",
    .flags = DEV_FLAGS_MB_READ | DEV_FLAGS_MB_WRITE | DEV_FLAGS_WR_BT_MASK,
    .read = devfull_read,
    .write = devfull_write
};

void _devfs_create_specials()
{
    if (make_dev(&devnull_info, 0, 0, 0666, NULL)) {
        KERROR(KERROR_ERR, "Failed to init dev/null\n");
    }

    if (make_dev(&devzero_info, 0, 0, 0666, NULL)) {
        KERROR(KERROR_ERR, "Failed to init dev/zero\n");
    }

    if (make_dev(&devfull_info, 0, 0, 0666, NULL)) {
        KERROR(KERROR_ERR, "Failed to init dev/full\n");
    }
}

int devnull_read(struct dev_info * devnfo, off_t blkno,
                 uint8_t * buf, size_t bcount, int oflags)
{
    return 0;
}

int devnull_write(struct dev_info * devnfo, off_t blkno,
                  uint8_t * buf, size_t bcount, int oflags)
{
    return bcount;
}

int devzero_read(struct dev_info * devnfo, off_t blkno,
                 uint8_t * buf, size_t bcount, int oflags)
{
    memset(buf, '\0', bcount);

    return bcount;
}

int devzero_write(struct dev_info * devnfo, off_t blkno,
                  uint8_t * buf, size_t bcount, int oflags)
{
    return bcount;
}

int devfull_read(struct dev_info * devnfo, off_t blkno,
                 uint8_t * buf, size_t bcount, int oflags)
{
    memset(buf, '\0', bcount);

    return bcount;
}

int devfull_write(struct dev_info * devnfo, off_t blkno,
                  uint8_t * buf, size_t bcount, int oflags)
{
    return -ENOSPC;
}
