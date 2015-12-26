/**
 *******************************************************************************
 * @file    mbr.c
 * @author  Olli Vanhoja
 * @brief   MBR driver.
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

#include <errno.h>
#include <stdint.h>
#include <kstring.h>
#include <libkern.h>
#include <kmalloc.h>
#include <kerror.h>
#include <proc.h>
#include <fs/mbr.h>

#define MBR_SIZE            512
#define MBR_NR_ENTRIES      4
#define MBR_ENTRY_SIZE      0x10

#define MBR_OFF_SIGNATURE   0x1fe
#define MBR_OFF_FIRST_ENTRY 0x1be

struct mbr_dev {
    struct dev_info dev;
    struct dev_info * parent;
    int part_no;
    uint32_t start_block;
    uint32_t blocks;
    uint8_t part_id; /* Partition type */
};

struct mbr_part_entry {
    uint8_t stat;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t nr_sect;
} __attribute__((packed));

static char driver_name[] = "mbr";

static int mbr_read(struct dev_info * devnfo, off_t offset,
                    uint8_t * buf, size_t count, int oflags);
static int mbr_write(struct dev_info * devnfo, off_t offset,
                     uint8_t * buf, size_t count, int oflags);

static int read_block_0(uint8_t * block_0, file_t * file)
{
    struct uio uio;
    int ret;

    uio_init_kbuf(&uio, block_0, MBR_SIZE);

    /* Read the first 512 bytes. */
    ret = dev_read(file, &uio, MBR_SIZE);
    if (ret < 0) {
        KERROR(KERROR_ERR, "MBR: block_read failed (%i)\n", ret);

        return ret;
    } else if (ret != MBR_SIZE) {
        KERROR(KERROR_ERR,
               "MBR: Failed to read %d bytes, only %d bytes read\n",
               MBR_SIZE, ret);

        return -ENOENT;
    }

    return 0;
}
/**
 * Check an MBR signature.
 */
static int check_signature(uint8_t * block_0)
{
    uint16_t signature;

    signature = read_halfword(block_0, MBR_OFF_SIGNATURE);
    if (signature != 0xAA55) {
        KERROR(KERROR_ERR, "MBR: Invalid signature (%x)\n", signature);

        return -ENOENT;
    }

    return 0;
}

static int make_dev_mbr(struct dev_info * parent,
                        const struct mbr_part_entry * part,
                        int part_no)
{
    static size_t mbr_dev_count;
    struct mbr_dev * d;
    int major_num = DEV_MAJOR(parent->dev_id) + 1;

    d = kzalloc(sizeof(struct mbr_dev));
    if (!d) {
        KERROR(KERROR_ERR, "MBR: Out of memory");
        return -ENOMEM;
    }

    d->dev.dev_id = DEV_MMTODEV(major_num, mbr_dev_count);
    d->dev.drv_name = driver_name;
    ksprintf(d->dev.dev_name, sizeof(d->dev.dev_name), "%sp%u",
             parent->dev_name, part_no);
    d->dev.read = mbr_read;
    d->dev.write = (parent->write) ? mbr_write : NULL;
    d->dev.block_size = parent->block_size;
    d->dev.flags = parent->flags;
    d->part_no = part_no;
    d->part_id = part->type;
    d->start_block = part->lba_start;
    d->blocks = part->nr_sect;
    d->dev.num_blocks = d->blocks;
    d->parent = parent;

#ifdef configMBR_DEBUG
    KERROR(KERROR_DEBUG,
           "MBR: partition number %i (%s) of type %x, start sector %u, sector count %u\n",
           d->part_no, d->dev.dev_name, d->part_id,
           d->start_block, d->blocks);
#endif

    make_dev(&d->dev, 0, 0, 0666, NULL);
    mbr_dev_count++;

    return 0;
}

int mbr_register(int fd, int * part_count)
{
    file_t * file;
    vnode_t * parent_vnode;
    struct dev_info * parent;
    uint8_t * block_0 = NULL;
    uint32_t block_size_adjust;
    int parts = 0, retval = 0;

#ifdef configMBR_DEBUG
    KERROR(KERROR_DEBUG, "%s(fd: %d, part_count: %p)\n",
           __func__, fd, part_count);
#endif

    file = fs_fildes_ref(curproc->files, fd, 1);

    parent_vnode = file->vnode;
    parent = (struct dev_info *)parent_vnode->vn_specinfo;
    if (!(S_ISBLK(parent_vnode->vn_mode) || S_ISCHR(parent_vnode->vn_mode))) {
        KERROR(KERROR_ERR, "MBR: not a device\n");

        retval = -ENODEV;
        goto fail;
    }

    /* Check the validity of the parent device. */
    if (!parent) {
        KERROR(KERROR_ERR, "MBR: invalid parent device\n");

        retval = -ENODEV;
        goto fail;
    }

    block_0 = kmalloc(MBR_SIZE);
    if (!block_0) {
        retval = -ENOMEM;
        goto fail;
    }

#ifdef configMBR_DEBUG
    KERROR(KERROR_DEBUG,
           "MBR: reading block 0 from device %s\n",
           parent->dev_name);
#endif

    retval = read_block_0(block_0, file);
    if (retval)
        goto fail;
    retval = check_signature(block_0);
    if (retval)
        goto fail;

#ifdef configMBR_DEBUG
    KERROR(KERROR_DEBUG,
             "MBR: found a valid MBR on device %s\n", parent->dev_name);
#endif

    /*
     * If parent block size is not MBR_SIZE, we have to coerce start_block
     * and blocks to fit.
     */
    if (parent->block_size < MBR_SIZE) {
        /* We do not support parent device block sizes < MBR_SIZE */
        KERROR(KERROR_ERR, "MBR: block size of %s is too small (%i)\n",
               parent->dev_name, parent->block_size);

        retval = -ENOTSUP;
        goto fail;
    }

    block_size_adjust = parent->block_size / MBR_SIZE;
    if (parent->block_size % MBR_SIZE) {
        /*
         * We do not support parent device block sizes that are not
         * multiples of MBR_SIZE.
         */
        KERROR(KERROR_ERR,
               "MBR: block size of %s is not a multiple of %d (%i)\n",
               parent->dev_name, MBR_SIZE, parent->block_size);

        retval = -ENOTSUP;
        goto fail;
    }

#ifdef configMBR_DEBUG
    if (block_size_adjust > 1) {
        KERROR(KERROR_DEBUG, "MBR: block_size_adjust: %i\n", block_size_adjust);
    }
#endif

    for (size_t i = 0; i < MBR_NR_ENTRIES; i++) {
        struct mbr_part_entry part;
        size_t p_offset = MBR_OFF_FIRST_ENTRY + (i * MBR_ENTRY_SIZE);

        memcpy(&part, block_0 + p_offset, sizeof(part));

        if (part.type == 0x00) {
            /* Invalid partition */
            continue;
        }

        if (part.lba_start % block_size_adjust) {
            KERROR(KERROR_ERR,
                   "MBR: partition number %i on %s does not start on a block boundary (%i).\n",
                   i, parent->dev_name, part.lba_start);
            continue;
        }

        if (part.nr_sect % block_size_adjust) {
            /*
             * Partition number i on parent device doesn't have a length that is
             * an exact multiple of the block length part.lba_start.
             */
            KERROR(KERROR_ERR,
                   "MBR: Size of part %i on %s isn't an exact multiple of the block length (%i)\n",
                   i, parent->dev_name, part.lba_start);
            continue;
        }

        part.lba_start /= block_size_adjust;
        part.nr_sect /= block_size_adjust;
        if (make_dev_mbr(parent, &part, i))
            continue;
        parts++;
    }

    KERROR(KERROR_INFO, "MBR: found total of %i partition(s)\n", parts);

fail:
    kfree(block_0);
    fs_fildes_ref(curproc->files, fd, -1);

    if (retval != 0) {
        KERROR(KERROR_ERR, "MBR registration failed on device: \"%s\"\n",
               parent->dev_name);
    }

    return retval;
}

static int mbr_read(struct dev_info * devnfo, off_t offset,
                    uint8_t * buf, size_t count, int oflags)
{
    struct mbr_dev * mbr = container_of(devnfo, struct mbr_dev, dev);
    struct dev_info * parent = mbr->parent;

    return parent->read(parent, offset + mbr->start_block, buf, count, oflags);
}

static int mbr_write(struct dev_info * devnfo, off_t offset,
                     uint8_t * buf, size_t count, int oflags)
{
    struct mbr_dev * mbr = container_of(devnfo, struct mbr_dev, dev);
    struct dev_info * parent = mbr->parent;

    return parent->write(parent, offset + mbr->start_block, buf, count, oflags);
}
