/**
 *******************************************************************************
 * @file    fs_uio.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system user io.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <buf.h>
#include <fs/fs_uio.h>
#include <proc.h>
#include <vm/vm.h>

int fs_uio_init_kbuf(struct fs_uio * uio, __kernel void * kbuf, size_t size)
{
    *uio = (struct fs_uio){
        .kbuf = kbuf,
        .ubuf = NULL,
        .proc = NULL,
        .bufsize = size,
    };

    return 0;
}

int fs_uio_init_ubuf(struct fs_uio * uio, __user void * ubuf, size_t size,
                     int rw)
{
    struct proc_info * proc = curproc;

    KASSERT(proc != NULL, "proc must be set");

    if (!useracc_proc(ubuf, size, proc, rw))
        return -EFAULT;

    *uio = (struct fs_uio){
        .kbuf = NULL,
        .ubuf = ubuf,
        .proc = proc,
        .bufsize = size,
    };

    return 0;
}

void fs_uio_buf2kuio(struct buf * bp, struct fs_uio * uio)
{
    if (bp->b_data == 0) {
        KERROR(KERROR_ERR, "buf %p not in memory\n", bp);
        return; /* RFE Maybe an error code should be returned? */
    }

    fs_uio_init_kbuf(uio, (__kernel void *)bp->b_data, bp->b_bcount);
}

int fs_uio_copyout(const void * src, struct fs_uio * uio, size_t offset,
                   size_t size)
{
    int retval;

    if (offset + size > uio->bufsize) {
        retval = -EIO;
    } else if (uio->kbuf) {
        memmove((uint8_t *)uio->kbuf + offset, src, size);
        retval = 0;
    } else if (uio->ubuf) {
        __user uint8_t * uaddr = (__user uint8_t *)uio->ubuf + offset;

        retval = copyout_proc(uio->proc, src, uaddr, size);
    } else {
        retval = -EIO;
    }

    return retval;
}

int fs_uio_copyin(struct fs_uio * uio, void * dst, size_t offset, size_t size)
{
    int retval;

    if (offset + size > uio->bufsize) {
        retval = -EIO;
    } else if (uio->kbuf) {
        memmove(dst, (uint8_t *)uio->kbuf + offset, size);
        retval = 0;
    } else if (uio->ubuf) {
        __user const uint8_t * uaddr = (__user uint8_t *)uio->ubuf + offset;

        retval = copyin_proc(uio->proc, uaddr, dst, size);
    } else {
        retval = -EIO;
    }

    return retval;
}

int fs_uio_get_kaddr(struct fs_uio * uio, __kernel void ** addr)
{
    int retval = 0;

    if (uio->kbuf) {
        *addr = uio->kbuf;
    } else if (uio->ubuf) {
        *addr = vm_uaddr2kaddr(uio->proc, uio->ubuf);
    } else {
        retval = -EINVAL;
    }

    return retval;
}
