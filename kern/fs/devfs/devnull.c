/**
 *******************************************************************************
 * @file    devnull.c
 * @author  Olli Vanhoja
 * @brief   dev/null pseudo device.
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

#include <kinit.h>
#include <kerror.h>
#include <fs/devfs.h>

static int devnull_read(struct dev_info * devnfo, off_t offset,
                        uint8_t * buf, size_t count, int oflags);
static int devnull_write(struct dev_info * devnfo, off_t offset,
                         uint8_t * buf, size_t count, int oflags);

struct dev_info devnull_info = {
    .dev_id = DEV_MMTODEV(1, 3),
    .drv_name = "memdev",
    .dev_name = "null",
    .flags = DEV_FLAGS_MB_READ | DEV_FLAGS_MB_WRITE | DEV_FLAGS_WR_BT_MASK,
    .read = devnull_read,
    .write = devnull_write
};

void devnull_init(void) __attribute__((constructor));
void devnull_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(devfs_init);

    if (dev_make(&devnull_info, 0, 0, 0666)) {
        KERROR(KERROR_ERR, "Failed to init dev/null");
    }

    SUBSYS_INITFINI("dev/null OK");
}

static int devnull_read(struct dev_info * devnfo, off_t offset,
                        uint8_t * buf, size_t count, int oflags)
{
    return 0;
}

static int devnull_write(struct dev_info * devnfo, off_t offset,
                         uint8_t * buf, size_t count, int oflags)
{
    return count;
}
