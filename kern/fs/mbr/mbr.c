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
#include <stdint.h>
#include <kstring.h>
#include <libkern.h>
#include <kmalloc.h>
#include <kerror.h>
#include <fs/mbr.h>

#if configDEBUG >= KERROR_DEBUG
#define MBR_DEBUG 1
#endif

/* Code for interpreting an mbr */

struct mbr_block_dev {
    struct block_dev bd;
    struct block_dev *parent;
    int part_no;
    uint32_t start_block;
    uint32_t blocks;
    uint8_t part_id;
};

static char driver_name[] = "mbr";

static int mbr_read(struct block_dev *, off_t starting_block, uint8_t * buf,
        size_t buf_size);
static int mbr_write(struct block_dev *, off_t starting_block, uint8_t * buf,
        size_t buf_size);

int mbr_register(vnode_t * parent_vnode,
        struct block_dev *** partitions, int * part_count)
{
    (void)partitions;
    (void)part_count;
    (void)driver_name;
    struct block_dev * parent = (struct block_dev *)parent_vnode->vn_dev;

    /* Check the validity of the parent device */
    if (parent == 0) {
        KERROR(KERROR_ERR, "MBR: invalid parent device\n");
        return -1;
    }

    /* Read the first 512 bytes */
    uint8_t * block_0 = kmalloc(512);
    if (block_0 == NULL)
        return -1;

#ifdef MBR_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf), "MBR: reading block 0 from device %s\n",
                parent->dev_name);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    int ret = block_read(parent_vnode, 0, block_0, 512);
    if (ret < 0) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "MBR: block_read failed (%i)\n", ret);
        KERROR(KERROR_ERR, buf);

        kfree(block_0);
        return ret;
    }
    if (ret != 512) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                 "MBR: unable to read first 512 bytes of device %s, "
                 "only %d bytes read\n",
                 parent->dev_name, ret);
        KERROR(KERROR_ERR, buf);

        kfree(block_0);
        return -1;
    }

    /* Check the MBR signature */
    if ((block_0[0x1fe] != 0x55) || (block_0[0x1ff] != 0xaa)) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                 "MBR: no valid mbr signature on device %s (bytes are %x %x)\n",
                 parent->dev_name, block_0[0x1fe], block_0[0x1ff]);
        KERROR(KERROR_ERR, buf);

        kfree(block_0);
        return -1;
    }
    {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                 "MBR: found valid MBR on device %s\n", parent->dev_name);
        KERROR(KERROR_INFO, buf);
    }

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
        char buf[80];

        /* We do not support parent device block sizes < 512 */
        ksprintf(buf, sizeof(buf),
                 "MBR: parent block device is too small (%i)\n",
                 parent->block_size);
        KERROR(KERROR_ERR, buf);

        kfree(block_0);
        return -1;
    }

    uint32_t block_size_adjust = parent->block_size / 512;
    if (parent->block_size % 512) {
        char buf[80];

        /* We do not support parent device block sizes that are not a */
        /*  multiple of 512 */
        ksprintf(buf, sizeof(buf),
                 "MBR: parent block size is not a multiple of 512 (%i)\n",
                 parent->block_size);
        KERROR(KERROR_ERR, buf);

        kfree(block_0);
        return -1;
    }

#ifdef MBR_DEBUG
    if (block_size_adjust > 1) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "MBR: block_size_adjust: %i\n",
                 block_size_adjust);
        KERROR(KERROR_ERR, buf);
    }
#endif

    /* Load the partitions */
    struct block_dev **parts = kmalloc(4 * sizeof(struct block_dev *));
    int cur_p = 0;

    for (int i = 0; i < 4; i++) {
        int p_offset = 0x1be + (i * 0x10);
        if (block_0[p_offset + 4] != 0x00) { /* Valid partition */
            struct mbr_block_dev *d = kcalloc(1, sizeof(struct mbr_block_dev));

            d->bd.drv_name = driver_name;

            ksprintf(d->bd.dev_name, sizeof(d->bd.dev_name), "%s_%c0",
                     parent->dev_name, '0' + i);
            d->bd.dev_id = i;
            d->bd.read = mbr_read;
            if (parent->write)
                d->bd.write = mbr_write;
            d->bd.block_size = parent->block_size;
            d->bd.flags = parent->flags;
            d->part_no = i;
            d->part_id = block_0[p_offset + 4];
            d->start_block = read_word(block_0, p_offset + 8);
            d->blocks = read_word(block_0, p_offset + 12);
            d->parent = parent;

            /* Adjust start_block and blocks to the parent block size */
            if (d->start_block % block_size_adjust) {
                char buf[120];

                ksprintf(buf, sizeof(buf),
                         "MBR: partition number %i does not start on a block "
                         "boundary (%i).\n", d->part_no, d->start_block);
                KERROR(KERROR_ERR, buf);

                return -1;
            }
            d->start_block /= block_size_adjust;

            if (d->blocks % block_size_adjust) {
                char buf[120];

                ksprintf(buf, sizeof(buf),
                    "MBR: partition number %i does not have a length "
                    "that is an exact multiple of the block length (%i).\n",
                    d->part_no, d->start_block);
                KERROR(KERROR_ERR, buf);

                return -1;
            }
            d->blocks /= block_size_adjust;
            d->bd.num_blocks = d->blocks;

            parts[cur_p++] = (struct block_dev *)d;
#ifdef MBR_DEBUG
            {
                char buf[120];

                ksprintf(buf, sizeof(buf),
                         "MBR: partition number %i (%s) of type %x, "
                         "start sector %u, sector count %u, p_offset %03x\n",
                         d->part_no, d->bd.dev_name, d->part_id,
                         d->start_block, d->blocks, p_offset);
                KERROR(KERROR_DEBUG, buf);
            }
#endif
            /* Register the fs */
            //register_fs__((struct block_dev *)d, d->part_id);
            /* TODO Register the fs dev with devfs */
        }
    }

    if (NULL != partitions) {
        *partitions = parts;
    } else {
        kfree(parts);
    }
    if (NULL != part_count) {
        *part_count = cur_p;
    }

    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "MBR: found total of %i partition(s)\n",
                 cur_p);
        KERROR(KERROR_INFO, buf);
    }

    kfree(block_0);

    return 0;
}

static int mbr_read(struct block_dev * dev, off_t starting_block, uint8_t * buf,
        size_t buf_size)
{
    struct block_dev *parent = ((struct mbr_block_dev *)dev)->parent;

    return parent->read(parent, starting_block +
            ((struct mbr_block_dev *)dev)->start_block, buf, buf_size);
}

static int mbr_write(struct block_dev * dev, off_t starting_block, uint8_t * buf,
        size_t buf_size)
{
    struct block_dev *parent = ((struct mbr_block_dev *)dev)->parent;

    return parent->write(parent, ((struct mbr_block_dev *)dev)->start_block,
            buf, buf_size);
}
