/**
 *******************************************************************************
 * @file    dyndebug.c
 * @author  Olli Vanhoja
 * @brief   Dynamic kerror debug messages.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kerror.h>
#include <fs/procfs.h>
#include <kstring.h>
#include <kmalloc.h>

__GLOBL(__start_set_debug_msg_sect);
__GLOBL(__stop_set_debug_msg_sect);
extern struct _kerror_debug_msg __start_set_debug_msg_sect;
extern struct _kerror_debug_msg __stop_set_debug_msg_sect;

static struct procfs_stream * read_dyndebug(const struct procfs_info * spec)
{
    struct procfs_stream * stream;
    struct _kerror_debug_msg * msg_opt = &__start_set_debug_msg_sect;
    struct _kerror_debug_msg * stop = &__stop_set_debug_msg_sect;
    const size_t bufsize = 400; /* FIXME */
    size_t bytes = 0;

    if (msg_opt == stop)
        return NULL;

    stream = kzalloc(sizeof(struct procfs_stream) + bufsize);
    if (!stream)
        return NULL;

    while (msg_opt < stop) {
        char * p = stream->buf + bytes;

        bytes += ksprintf(p, bufsize - bytes, ">%u:%s: %s",
                          msg_opt->flags, msg_opt->file, msg_opt->msg);

        msg_opt++;
    }

    stream->bytes = bytes;
    return stream;
}

ssize_t write_dyndebug(const struct procfs_info * spec,
                       struct procfs_stream * stream,
                       const uint8_t * buf, size_t bufsize)
{
    struct _kerror_debug_msg * msg_opt = &__start_set_debug_msg_sect;
    struct _kerror_debug_msg * stop = &__stop_set_debug_msg_sect;
    const char * file = (char *)buf;

    if (msg_opt == stop)
        return 0;

    if (!strvalid(file, bufsize))
        return -EINVAL;

    while (msg_opt < stop) {
        if (strcmp(file, msg_opt->file) == 0) {
            msg_opt->flags ^= 1;
        }

        msg_opt++;
    }

    return stream->bytes;
}

static struct procfs_file procfs_file_dyndebug = {
    .filetype = PROCFS_DYNDEBUG,
    .filename = "dyndebug",
    .readfn = read_dyndebug,
    .writefn = write_dyndebug,
};
DATA_SET(procfs_files, procfs_file_dyndebug);
