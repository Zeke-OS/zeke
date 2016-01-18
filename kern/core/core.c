/**
 *******************************************************************************
 * @file    core.c
 * @author  Olli Vanhoja
 * @brief   Core dump facility.
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
#include <fcntl.h>
#include <limits.h>
#include <core.h>
#include <host.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>

SYSCTL_NODE(_kern, OID_AUTO, core, CTLFLAG_RW, 0,
            "Core dump configuration");

static char core_file_pattern[NAME_MAX] = "/tmp/%N.core";
SYSCTL_STRING(_kern_core, OID_AUTO, corefile, CTLFLAG_RW,
              core_file_pattern, 0, "Core file name pattern");

static int core_snprintf(char * str, size_t maxlen, const char * format,
                         struct proc_info * proc)
{
    size_t i = 0, n = 0;
    char c;

    maxlen--;
    while ((c = format[i++]) != '\0' && n <= maxlen) {
        switch (c) {
        case '%':
            c = format[i++];

            if (c == '\0')
                break;

            switch (c) {
            case 'H': /* Hostname */
                strlcpy(str + n, hostname, maxlen - n);
                n = min(n + strlenn(hostname, maxlen), maxlen);
                break;
            case 'N': /* Process name */
                {
                    char * fname;

                    /* It's usually a path. */
                    parsenames(proc->name, NULL, &fname);

                    strlcpy(str + n, fname, maxlen - n);
                    n = min(n + strlenn(fname, maxlen), maxlen);
                    kfree(fname);

                    break;
                }
            case 'P': /* Process ID */
                n += uitoa32(str + n, (uint32_t)proc->pid);
                break;
            case 'U': /* Process UID */
                n += uitoa32(str + n, (uint32_t)proc->cred.uid);
                break;
            case '%':
                str[n++] = c;
                break;
            default:
                break;
            }
            break;
        default:
            str[n++] = c;
            break;
        }
    }

    str[n] = '\0';

    return n + 1;
}

static char * generate_core_name(struct proc_info * proc)
{
    char * str;

    str = kmalloc(NAME_MAX);
    core_snprintf(str, NAME_MAX, core_file_pattern, proc);

    return str;
}

int core_dump_by_curproc(struct proc_info * proc)
{
    char msghead[40];
    char * core_path;
    vnode_t * vn = NULL;
    int fd = -1;
    file_t * file = NULL;
    int err;

    ksprintf(msghead, sizeof(msghead), "%s(%d) by %d:",
             __func__, proc->pid, curproc->pid);

    KERROR(KERROR_DEBUG, "%s Core dump requested\n", msghead);

    core_path = generate_core_name(proc);
    err = fs_creat_curproc(core_path, curproc->files->umask, &vn);
    if (err) {
        KERROR(KERROR_ERR,
               "%s Failed to create a core file: \"%s\"\n",
               msghead, core_path);
        goto out;
    }
    kfree(core_path);

    fd = fs_fildes_create_curproc(vn, O_RDWR);
    if (fd < 0) {
        KERROR(KERROR_ERR,
               "%s Failed to open a core file for write\n",
               msghead);
        err = fd;
        goto out;
    }

    file = fs_fildes_ref(curproc->files, fd, 1);
    if (!file) {
        KERROR(KERROR_ERR,
               "%s Failed to take a ref to a core file\n",
               msghead);
        err = -EBADF;
        goto out;
    }

    core_dump2file(proc, file);

    err = 0;
out:
    if (fd >= 0)
        fs_fildes_ref(curproc->files, fd, -1);
    if (file)
        fs_fildes_close(curproc, fd);
    if (vn)
        vrele(vn);

    return err;
}
