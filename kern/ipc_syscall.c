/**
 *******************************************************************************
 * @file    ipc_syscall.c
 * @author  Olli Vanhoja
 * @brief   Generic IPC syscalls.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2015, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <proc.h>
#include <kern_ipc.h>

static intptr_t sys_pipe(__user void * user_args)
{
    struct _ipc_pipe_args args;
    int err;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }
    copyin(user_args, &args, sizeof(args));

    err = fs_pipe_curproc_creat(curproc->files, args.fildes, args.len);
    if (err) {
        set_errno(-err);
        return -1;
    }

    (void)copyout(&args, user_args, sizeof(args));

    return 0;
}

/**
 * Declarations of ipc syscall functions.
 */
static const syscall_handler_t ipc_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_IPC_PIPE, sys_pipe),
};
SYSCALL_HANDLERDEF(ipc_syscall, ipc_sysfnmap)
