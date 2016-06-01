/**
 *******************************************************************************
 * @file    ioctl_syscall.c
 * @author  Olli Vanhoja
 * @brief   Control devices.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fs/fs.h>
#include <errno.h>
#include <syscall.h>
#include <sys/ioctl.h>

static int sys_ioctl(__user void * user_args)
{
    struct _ioctl_get_args args;
    void * ioargs = NULL;
    file_t * file;
    int err, retval;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (args.arg) {
        struct uio uio;
        __user void * user_buf = (__user void *)args.arg;

        /* Get request needs wr and set request needs rd */
        const int rw = (args.request & 1) ? VM_PROT_WRITE : VM_PROT_READ;


        if ((err = uio_init_ubuf(&uio, user_buf, args.arg_len, rw)) ||
            (err = uio_get_kaddr(&uio, &ioargs))) {
            set_errno(-err);
            return -1;
        }
    }

    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        return -1;
    }

    /* Actual ioctl call */
    retval = file->vnode->vnode_ops->ioctl(file, args.request,
                                           ioargs, args.arg_len);
    if (retval < 0) {
        retval = -1;
        set_errno(-retval);
    }

    fs_fildes_ref(curproc->files, args.fd, -1);
    return retval;
}

/**
 * Declarations of syscall functions.
 */
static const syscall_handler_t ioctl_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_IOCTL_GETSET, sys_ioctl),
};
SYSCALL_HANDLERDEF(ioctl_syscall, ioctl_sysfnmap);
