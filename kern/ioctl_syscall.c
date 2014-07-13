/**
 *******************************************************************************
 * @file    ioctl_syscall.c
 * @author  Olli Vanhoja
 * @brief   Control devices.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL
#include <stdint.h>
#include <stddef.h>
#include <proc.h>
#include <vm/vm.h>
#include <kmalloc.h>
#include <fs/fs.h>
#include <errno.h>
#include <termios.h>
#include <syscall.h>
#include <sys/ioctl.h>

static int sys_ioctl(void * user_args)
{
    struct _ioctl_get_args args;
    file_t * file;
    void * ioargs = 0;
    int changed = 0;
    int retval = -1;

    if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
        set_errno(EFAULT);
        return -1;
    }
    copyin(user_args, &args, sizeof(args));

    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        return -1;
    }

    /* Copyin actual args based on request */
    switch (args.request) {
    /* Get operation, no need to copyin */
    case IOCTL_GTERMIOS:
        if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
            set_errno(EFAULT);
            goto out;
        }

        ioargs = kmalloc(sizeof(struct termios));
        if (!ioargs) {
            set_errno(ENOMEM);
            goto out;
        }
        changed = 1; /* Ensure copyout */

        break;

    /* Set operation, copyin needed */
    case IOCTL_STERMIOS:
        if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
            set_errno(EFAULT);
            goto out;
        }

        ioargs = kmalloc(sizeof(struct termios));
        if (!ioargs) {
            set_errno(ENOMEM);
            goto out;
        }
        copyin(args.args, ioargs, sizeof(struct termios));

        break;

    default:
        set_errno(EINVAL);
        goto out;
    }

    /* Actual ioctl call */
    if (!file->vnode->vnode_ops->ioctl) {
        set_errno(ENOTTY);
        goto out;
    }
    retval = file->vnode->vnode_ops->ioctl(file, args.request, ioargs);
    if (retval)
        set_errno(-retval);

    /* Copyout if request type was get. */
    if (changed)
        copyout(ioargs, args.args, sizeof(struct termios));

out:
    if (ioargs)
        kfree(ioargs);
    fs_fildes_ref(curproc->files, args.fd, -1);
    return retval;
}

uintptr_t ioctl_syscall(uint32_t type, void * p)
{
    switch (type) {
    case SYSCALL_IOCTL_GETSET:
        return (uintptr_t)sys_ioctl(p);

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
