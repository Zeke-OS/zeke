/**
 *******************************************************************************
 * @file    mbr.c
 * @author  Olli Vanhoja
 * @brief   MBR driver.
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
 *
 *
 * Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *******************************************************************************
 */

#include <autoconf.h>
#include <errno.h>
#include <stdint.h>
#include <kstring.h>
#include <libkern.h>
#include <kmalloc.h>
#include <kerror.h>
#include <proc.h>
#include <fs/mbr.h>

#if configDEBUG >= KERROR_DEBUG
#define MBR_DEBUG 1
#endif

struct mbr_dev {
    struct dev_info dev;
    struct dev_info * parent;
    int part_no;
    uint32_t start_block;
    uint32_t blocks;
    uint8_t part_id;
};

static size_t mbr_dev_count;

static char driver_name[] = "mbr";

static int mbr_read(struct dev_info * devnfo, off_t offset,
                    uint8_t * buf, size_t count, int oflags);
static int mbr_write(struct dev_info * devnfo, off_t offset,
                     uint8_t * buf, size_t count, int oflags);


int mbr_register(int fd, int * part_count)
{
    file_t * file;
    vnode_t * parent_vnode;
    struct dev_info * parent;
    uint8_t * block_0 = 0;
#ifdef MBR_DEBUG
    char msgbuf[120];
#endif
    int parts = 0;
    int retval = 0;

    /* Lock file */
    file = fs_fildes_ref(curproc->files, fd, 1);

    parent_vnode = file->vnode;
    parent = (struct dev_info *)parent_vnode->vn_specinfo;

    if (!(S_ISBLK(parent_vnode->vn_mode) || S_ISCHR(parent_vnode->vn_mode))) {
        KERROR(KERROR_ERR, "MBR: not a device\n");

        retval = -ENODEV;
        goto fail;
    }

    /* Check the validity of the parent device */
    if (!parent) {
        KERROR(KERROR_ERR, "MBR: invalid parent device\n");

        retval = -ENODEV;
        goto fail;
    }

    /* Read the first 512 bytes */
    block_0 = kmalloc(512);
    if (!block_0) {
        retval = -ENOMEM;
        goto fail;
    }

#ifdef MBR_DEBUG
    ksprintf(msgbuf, sizeof(msgbuf), "MBR: reading block 0 from device %s\n",
            parent->dev_name);
    KERROR(KERROR_DEBUG, msgbuf);
#endif

    int ret = dev_read(file, block_0, 512);
    if (ret < 0) {
        ksprintf(msgbuf, sizeof(msgbuf), "MBR: block_read failed (%i)\n", ret);
        KERROR(KERROR_ERR, msgbuf);

        retval = ret;
        goto fail;
    } else if (ret != 512) {
        ksprintf(msgbuf, sizeof(msgbuf),
                 "MBR: unable to read first 512 bytes of device %s, "
                 "only %d bytes read\n",
                 parent->dev_name, ret);
        KERROR(KERROR_ERR, msgbuf);

        retval = -ENOENT;
        goto fail;
    }

    /* Check the MBR signature */
    if ((block_0[0x1fe] != 0x55) || (block_0[0x1ff] != 0xaa)) {
        ksprintf(msgbuf, sizeof(msgbuf),
                 "MBR: no valid mbr signature on device %s (bytes are %x %x)\n",
                 parent->dev_name, block_0[0x1fe], block_0[0x1ff]);
        KERROR(KERROR_ERR, msgbuf);

        retval = -ENOENT;
        goto fail;
    }

    ksprintf(msgbuf, sizeof(msgbuf),
             "MBR: found valid MBR on device %s\n", parent->dev_name);
    KERROR(KERROR_INFO, msgbuf);

#ifdef MBR_DEBUG
#if 0
    /* Dump the first sector */
    printf("MBR: first sector:");
    for (int dump_idx = 0; dump_idx < 512; dump_idx++) {
        if ((dump_idx & 0xf) == 0)
            printf("\n%03x: ", dump_idx);
        printf("%02x ", block_0[dump_idx]);
    }
    printf("\n");
#endif
#endif

    /* If parent block size is not 512, we have to coerce start_block */
    /*  and blocks to fit */
    if (parent->block_size < 512) {
        /* We do not support parent device block sizes < 512 */
        ksprintf(msgbuf, sizeof(msgbuf),
                 "MBR: parent block device is too small (%i)\n",
                 parent->block_size);
        KERROR(KERROR_ERR, msgbuf);

        retval = -ENOTSUP;
        goto fail;
    }

    uint32_t block_size_adjust = parent->block_size / 512;
    if (parent->block_size % 512) {
        /* We do not support parent device block sizes that are not a */
        /*  multiple of 512 */
        ksprintf(msgbuf, sizeof(msgbuf),
                 "MBR: parent block size is not a multiple of 512 (%i)\n",
                 parent->block_size);
        KERROR(KERROR_ERR, msgbuf);

        retval = -ENOTSUP;
        goto fail;
    }

#ifdef MBR_DEBUG
    if (block_size_adjust > 1) {
        ksprintf(msgbuf, sizeof(msgbuf), "MBR: block_size_adjust: %i\n",
                 block_size_adjust);
        KERROR(KERROR_ERR, msgbuf);
    }
#endif

    for (int i = 0; i < 4; i++) {
        int p_offset = 0x1be + (i * 0x10);
        if (block_0[p_offset + 4] != 0x00) { /* Valid partition */
            struct mbr_dev * d = kmalloc(sizeof(struct mbr_dev));

            if (!d) {
                KERROR(KERROR_ERR, "OOM while reading MBR");

                retval = -ENOMEM;
                break;
            } else {
                parts++;
                mbr_dev_count++;
            }

            d->dev.drv_name = driver_name;

            ksprintf(d->dev.dev_name, sizeof(d->dev.dev_name), "%s_%c0",
                     parent->dev_name, '0' + i);
            d->dev.dev_id = i;
            d->dev.read = mbr_read;
            if (parent->write)
                d->dev.write = mbr_write;
            d->dev.block_size = parent->block_size;
            d->dev.flags = parent->flags;
            d->part_no = i;
            d->part_id = block_0[p_offset + 4];
            d->start_block = read_word(block_0, p_offset + 8);
            d->blocks = read_word(block_0, p_offset + 12);
            d->parent = parent;

            /* Adjust start_block and blocks to the parent block size */
            if (d->start_block % block_size_adjust) {
                ksprintf(msgbuf, sizeof(msgbuf),
                         "MBR: partition number %i does not start on a block "
                         "boundary (%i).\n", d->part_no, d->start_block);
                KERROR(KERROR_ERR, msgbuf);

                return -1;
            }
            d->start_block /= block_size_adjust;

            if (d->blocks % block_size_adjust) {
                ksprintf(msgbuf, sizeof(msgbuf),
                    "MBR: partition number %i does not have a length "
                    "that is an exact multiple of the block length (%i).\n",
                    d->part_no, d->start_block);
                KERROR(KERROR_ERR, msgbuf);

                return -1;
            }
            d->blocks /= block_size_adjust;
            d->dev.num_blocks = d->blocks;

#ifdef MBR_DEBUG
            {
                ksprintf(msgbuf, sizeof(msgbuf),
                         "MBR: partition number %i (%s) of type %x, "
                         "start sector %u, sector count %u, p_offset %03x\n",
                         d->part_no, d->dev.dev_name, d->part_id,
                         d->start_block, d->blocks, p_offset);
                KERROR(KERROR_DEBUG, msgbuf);
            }
#endif
            /* Register the fs */
            //register_fs__((struct dev_info *)d, d->part_id);
            /* TODO Register the fs dev with devfs */
            dev_make(&d->dev, 0, 0, 0666, NULL);
        }
    }

    ksprintf(msgbuf, sizeof(msgbuf), "MBR: found total of %i partition(s)\n",
             parts);
    KERROR(KERROR_INFO, msgbuf);

fail:
    kfree(block_0);
    fs_fildes_ref(curproc->files, fd, -1);

    return retval;
}

static int mbr_read(struct dev_info * devnfo, off_t offset,
                    uint8_t * buf, size_t count, int oflags)
{
    struct mbr_dev * mbr = container_of(devnfo, struct mbr_dev, dev);
    struct dev_info * parent =  mbr->parent;

    return parent->read(parent, offset + mbr->start_block, buf, count, oflags);
}

static int mbr_write(struct dev_info * devnfo, off_t offset,
                     uint8_t * buf, size_t count, int oflags)
{
    struct mbr_dev * mbr = container_of(devnfo, struct mbr_dev, dev);
    struct dev_info * parent =  mbr->parent;

    return parent->write(parent, offset + mbr->start_block, buf, count, oflags);
}
