/**
 *******************************************************************************
 * @file    fatfs_zeke.c
 * @author  Olli Vanhoja
 * @brief   IO wrapper for FatFs.
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

#include <libkern.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kstring.h>
#include <libkern.h>
#include <hal/core.h>
#include <kerror.h>
#include <fs/fs.h>
#include <fs/devfs.h>
#include "src/diskio.h"
#include "fatfs.h"

/**
 * Read sector(s).
 * @param pdrv      is a (physical) drive nmuber to identify the drive.
 * @param buff      is a data buffer to store read data.
 * @param sector    is a sector address in LBA.
 * @param count     is the number of bytes to read.
 *
 */
DRESULT fatfs_disk_read(uint8_t pdrv, uint8_t * buff, DWORD sector,
                        unsigned int count)
{
    file_t * file;
    struct fs_uio uio;
    ssize_t retval;

    if (pdrv >= configFATFS_MAX_MOUNTS || !fatfs_sb_arr[pdrv])
        return RES_PARERR;

    file = &fatfs_sb_arr[pdrv]->ff_devfile;
    file->seek_pos = sector;

    fs_uio_init_kbuf(&uio, buff, count);
    retval = file->vnode->vnode_ops->read(file, &uio, count);
    if (retval < 0) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_ERR, "fatfs_disk_read(): err %i\n", retval);
#endif
        return RES_ERROR;
    }

    if (retval != (ssize_t)count) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_WARN, "retval(%i) != count(%i)\n",
                (uint32_t)retval, (uint32_t)count);
#endif
        return RES_PARERR;
    }

    return 0;
}

/**
 * Write sector(s).
 * @param pdrv      is a (physical) drive nmuber to identify the drive.
 * @param buff      is the data buffer to be written.
 * @param sector    is a sector address in LBA.
 * @param count     is the number of bytes to write.
 */
DRESULT fatfs_disk_write(uint8_t pdrv, const uint8_t * buff, DWORD sector,
                         unsigned int count)
{
    file_t * file;
    struct fs_uio uio;
    ssize_t retval;

    if (pdrv >= configFATFS_MAX_MOUNTS || !fatfs_sb_arr[pdrv])
        return RES_PARERR;

    file = &fatfs_sb_arr[pdrv]->ff_devfile;
    file->seek_pos = sector;

    fs_uio_init_kbuf(&uio, (void *)buff, count);
    retval = file->vnode->vnode_ops->write(file, &uio, count);
    if (retval < 0) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_ERR, "fatfs_disk_write(): err %i\n", retval);
#endif

        return RES_ERROR;
    }

    if (retval != (ssize_t)count)
        return RES_PARERR;

    return 0;
}

DRESULT fatfs_disk_ioctl(uint8_t pdrv, unsigned cmd, void * buff, size_t bsize)
{
    file_t * file;
    ssize_t err;
#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG,
            "fatfs_disk_ioctl(pdrv %u, cmd %u, buff %p, bsize %u)\n",
            (uint32_t)pdrv, (uint32_t)cmd, buff, (uint32_t)bsize);
#endif

    if (pdrv >= configFATFS_MAX_MOUNTS || !fatfs_sb_arr[pdrv])
        return RES_PARERR;

    file = &fatfs_sb_arr[pdrv]->ff_devfile;
    if (!file->vnode->vnode_ops->ioctl)
        return RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC:
        /* TODO Not implemented yet. */
        return 0;
        break;
    case GET_BLOCK_SIZE:
    case CTRL_ERASE_SECTOR:
    default:
        break;
    }

    err = file->vnode->vnode_ops->ioctl(file, cmd, buff, bsize);
    if (err)
        return RES_ERROR;

    return 0;
}
