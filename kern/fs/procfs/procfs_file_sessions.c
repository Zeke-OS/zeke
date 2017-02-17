/**
 *******************************************************************************
 * @file    procfs_sessions.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
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
#include <kmalloc.h>
#include <kstring.h>
#include <proc.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/procfs.h>

#define SESSION_LINE_MAX 40

static struct procfs_stream * read_sessions(const struct procfs_file * spec)
{
    const size_t bufsize = nr_sessions * SESSION_LINE_MAX;
    struct procfs_stream * stream;
    ssize_t bytes = 0;

    stream = kzalloc(sizeof(struct procfs_stream) + bufsize);
    if (!stream)
        return NULL;

    PROC_LOCK();
    struct session * sp;
    char * bpos;

    TAILQ_FOREACH(sp, &proc_session_list_head, s_session_list_entry_) {
        bpos = stream->buf + bytes;
        bytes += ksprintf(bpos, SESSION_LINE_MAX, "%d %d %s\n",
                          sp->s_leader, sp->s_ctty_fd, sp->s_login);
    }
    PROC_UNLOCK();

    stream->bytes = bytes;
    return stream;
}

static struct procfs_file procfs_file_sessions = {
    .filename = "sessions",
    .readfn = read_sessions,
    .relefn = procfs_kfree_stream,
};
DATA_SET(procfs_files, procfs_file_sessions);
