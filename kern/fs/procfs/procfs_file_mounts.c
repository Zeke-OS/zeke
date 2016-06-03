/**
 *******************************************************************************
 * @file    procfs_mounts.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
 * @section LICENSE
 * Copyright (c) 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kmalloc.h>
#include <kstring.h>
#include <proc.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/procfs.h>

static struct procfs_stream * read_mounts(const struct procfs_info * spec)
{
    struct procfs_stream * stream;
    fs_t * fs;
    const size_t maxline = 80;
    ssize_t bytes = 0;

    stream = kzalloc(sizeof(struct procfs_stream) + maxline);
    if (!stream)
        return NULL;

    fs = NULL;
    while ((fs = fs_iterate(fs))) {
        struct procfs_stream * tmp;
        char * p = stream->buf + bytes;
        struct fs_superblock * sb;

        bytes += ksprintf(p, bytes + maxline, "%s\n", fs->fsname);

        sb = NULL;
        while ((sb = fs_iterate_superblocks(fs, sb))) {
            char * p = stream->buf + bytes;

            bytes += ksprintf(p, bytes + maxline, "  (%u,%u)\n",
                              DEV_MAJOR(sb->vdev_id),
                              DEV_MINOR(sb->vdev_id));
        }

        tmp = krealloc(stream, bytes + maxline);
        if (!tmp) {
            kfree(stream);
            return NULL;
        }
        stream = tmp;
    }

    stream->bytes = bytes;
    return stream;
}

static struct procfs_file procfs_file_mounts = {
    .filetype = PROCFS_MOUNTS,
    .filename = "mounts",
    .readfn = read_mounts,
    .writefn = NULL,
    .relefn = kfree,
};
DATA_SET(procfs_files, procfs_file_mounts);
