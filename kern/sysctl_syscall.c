/**
 *******************************************************************************
 * @file    sysctl_syscall.c
 * @author  Olli Vanhoja
 * @brief   Syscall handler for sysctl.
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
#include <sys/sysctl.h>
#include <syscall.h>
#include <proc.h>

intptr_t sysctl_syscall(uint32_t type, __user void * p)
{
    int err, name[CTL_MAXNAME];
    size_t j;
    struct _sysctl_args uap;

    if (type != SYSCALL_SYSCTL_SYSCTL) {
        set_errno(ENOSYS);
        return -1;
    }

    err = copyin(p, &uap, sizeof(uap));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (uap.namelen > CTL_MAXNAME || uap.namelen < 2) {
        set_errno(EINVAL);
        return -1;
    }

    err = copyin((__user void *)uap.name, &name, uap.namelen * sizeof(int));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    err = userland_sysctl(curproc->pid, name, uap.namelen,
                          (__user void *)uap.old, (__user size_t *)uap.oldlenp,
                          0, (__user void *)uap.new, uap.newlen, &j, 0);
    if (err && err != -ENOMEM) {
        set_errno(-err);
        return -1;
    }
    if (uap.oldlenp) {
        err = copyout(&j, (__user size_t *)uap.oldlenp, sizeof(j));
        if (err) {
            set_errno(-err);
            return -1;
        }
    }

    return 0;
}
