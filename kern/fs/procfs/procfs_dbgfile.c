/**
 *******************************************************************************
 * @file    procfs_dbgfile.c
 * @author  Olli Vanhoja
 * @brief   Generic debug file handler.
 * @section LICENSE
 * Copyright (c) 2016, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <stdint.h>
#include <buf.h>
#include <fs/procfs.h>
#include <fs/procfs_dbgfile.h>

#define DBGFILE_MAX_LINE 80

static inline struct procfs_stream * buf2stream(struct buf * streambuf)
{
    return (struct procfs_stream *)(streambuf->b_data + sizeof(struct buf *));
}

static inline struct buf * stream2buf(struct procfs_stream * stream)
{
    return (struct buf *)((uint8_t *)stream - sizeof(struct buf *));
}

struct procfs_stream * procfs_dbgfile_read(const struct procfs_file * spec)
{
    struct procfs_dbgfile_opt * opt = (struct procfs_dbgfile_opt *)spec->opt;
    uint8_t * elem = opt->sect_start;
    uint8_t * stop = opt->sect_stop;
    const size_t nr_msg = ((size_t)stop - (size_t)elem) / opt->bsize;
    size_t bufsize = nr_msg * DBGFILE_MAX_LINE;
    struct buf * streambuf;
    struct procfs_stream * stream;
    struct uio uio;
    size_t bytes = 0;

    if (elem == stop)
        return NULL;

    streambuf = geteblk(bufsize);
    if (!streambuf)
        return NULL;

    bufsize -= sizeof(struct buf *);
    stream = buf2stream(streambuf);

    uio_init_kbuf(&uio, &stream->buf, bufsize);
    while (elem < stop) {
        int len;

        len = opt->read(stream->buf + bytes, bufsize - bytes, elem);
        bytes += len;

        elem += opt->bsize;
    }
    stream->bytes = bytes;

    return stream;
}

ssize_t procfs_dbgfile_write(const struct procfs_file * spec,
                             struct procfs_stream * stream,
                             const uint8_t * buf, size_t bufsize)
{
    struct procfs_dbgfile_opt * opt = (struct procfs_dbgfile_opt *)spec->opt;

    if (opt->sect_start == opt->sect_stop)
        return 0;

    return opt->write(buf, bufsize);
}

void procfs_dbgfile_rele(struct procfs_stream * stream)
{
    vrfree(stream2buf(stream));
}

