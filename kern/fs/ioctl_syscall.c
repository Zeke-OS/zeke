/**
 *******************************************************************************
 * @file    ioctl_syscall.c
 * @author  Olli Vanhoja
 * @brief   Control devices.
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

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <proc.h>
#include <vm/vm.h>
#include <kmalloc.h>
#include <fs/fs.h>
#include <errno.h>
#include <syscall.h>
#include <sys/ioctl.h>

static int sys_ioctl(void * user_args)
{
    struct _ioctl_get_args args;
    file_t * file;
    void * ioargs = NULL;
    int err, retval = -1;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        return -1;
    }

    if (args.arg) {
        ioargs = kmalloc(args.arg_len);
        if (!ioargs) {
            set_errno(ENOMEM);
            goto out;
        }

        if (args.request & 1) { /* Get request */
            if (!useracc(args.arg, args.arg_len, VM_PROT_WRITE)) {
                set_errno(EFAULT);
                goto out;
            }
            /* Get request doesn't need copyin */
        } else { /* Set request */
            /* Set operation needs copyin */
            err = copyin(args.arg, ioargs, args.arg_len);
            if (err) {
                set_errno(EFAULT);
                goto out;
            }
        }
    }

    /* Actual ioctl call */
    retval = file->vnode->vnode_ops->ioctl(file, args.request,
                                           ioargs, args.arg_len);
    if (retval < 0)
        set_errno(-retval);

    /* Copyout if request type was get. */
    if (args.request & 1)
        copyout(ioargs, args.arg, args.arg_len);

out:
    kfree(ioargs);
    fs_fildes_ref(curproc->files, args.fd, -1);
    return retval;
}

intptr_t ioctl_syscall(uint32_t type, void * p)
{
    switch (type) {
    case SYSCALL_IOCTL_GETSET:
        return (uintptr_t)sys_ioctl(p);

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
