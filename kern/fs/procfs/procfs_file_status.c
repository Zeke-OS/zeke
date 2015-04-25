/**
 *******************************************************************************
 * @file    procfs_file_status.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
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

#include <errno.h>
#include <kmalloc.h>
#include <kstring.h>
#include <proc.h>
#include <fs/procfs.h>

static ssize_t procfs_read_status(struct procfs_info * spec, char ** retbuf)
{
    ssize_t bytes;
    struct proc_info * proc;
    char * tmpbuf;
    const size_t bufsz = 200;

    tmpbuf = kcalloc(bufsz, sizeof(char));
    if (!tmpbuf)
        return -EIO;

    PROC_LOCK();
    proc = proc_get_struct(spec->pid);
    if (!proc) {
        PROC_UNLOCK();
        return -ENOLINK;
    }

    bytes = ksprintf(tmpbuf, bufsz,
                     "Name: %s\n"
                     "State: %u\n"
                     "Pid: %u\n"
                     "Uid: %u %u %u\n"
                     "Gid: %u %u %u\n"
                     "User: %u\n"
                     "Sys: %u\n"
                     "Break: %p %p\n",
                     proc->name,
                     proc->state,
                     proc->pid,
                     proc->cred.uid, proc->cred.euid, proc->cred.suid,
                     proc->cred.gid, proc->cred.egid, proc->cred.sgid,
                     proc->tms.tms_utime, proc->tms.tms_stime,
                     proc->brk_start, proc->brk_stop);

    PROC_UNLOCK();
    *retbuf = tmpbuf;
    return bytes;
}

static struct procfs_file procfs_file_status = {
    .filetype = PROCFS_STATUS,
    .filename = "status",
    .readfn = procfs_read_status,
    .writefn = NULL,
};
DATA_SET(procfs_files, procfs_file_status);
