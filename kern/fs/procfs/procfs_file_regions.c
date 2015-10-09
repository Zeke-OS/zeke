/**
 *******************************************************************************
 * @file    procfs_mounts.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
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
#include <kmalloc.h>
#include <kstring.h>
#include <proc.h>
#include <buf.h>
#include <vm/vm.h>
#include <fs/fs.h>
#include <fs/procfs.h>

static ssize_t procfs_read_regions(struct procfs_info * spec, char ** retbuf)
{
    char * buf = NULL;
    const size_t maxline = 30;
    ssize_t bytes = 0;
    struct proc_info * proc;
    struct vm_mm_struct * mm;

    buf = kcalloc(maxline, sizeof(char));
    if (!buf)
        return -ENOMEM;

    proc = proc_ref(spec->pid, PROC_NOT_LOCKED);
    if (!proc) {
        bytes = -ENOLINK;
        goto out;
    }
    mm = &proc->mm;
    mtx_lock(&mm->regions_lock);

    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];
        uintptr_t reg_start, reg_end;
        char uap[5];
        char * p;

        if (!region)
            continue;

        p = krealloc(buf, bytes + maxline);
        if (!p) {
            mtx_unlock(&mm->regions_lock);
            bytes = -ENOMEM;
            goto out;
        }
        buf = p;
        p = buf + bytes;

        reg_start = region->b_mmu.vaddr;
        reg_end = region->b_mmu.vaddr + region->b_bufsize - 1;
        vm_get_uapstring(uap, region);

        bytes += ksprintf(p, bytes + maxline, "%x %x %s\n",
                          reg_start, reg_end, uap);
    }
    mtx_unlock(&mm->regions_lock);

    *retbuf = buf;
out:
    proc_unref(proc);
    kfree(buf);
    return bytes;
}

static struct procfs_file procfs_file_regions = {
    .filetype = PROCFS_REGIONS,
    .filename = "regions",
    .readfn = procfs_read_regions,
    .writefn = NULL,
};
DATA_SET(procfs_files, procfs_file_regions);
