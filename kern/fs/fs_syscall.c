/**
 *******************************************************************************
 * @file    fs_syscall.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system syscalls.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL 1
#include <kerror.h>
#include <kstring.h>
#include <kmalloc.h>
#include <vm/vm.h>
#include <proc.h>
#include <sched.h>
#include <ksignal.h>
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
#include <fcntl.h>
#include <fs/fs.h>

static int fs_syscall_write(void * p)
{
    struct _fs_write_args fswa;
    char * buf;
    int retval;

    /* Args */
    if (!useracc(p, sizeof(struct _fs_write_args), VM_PROT_READ)) {
        /* No permission to read/write */
        /* TODO Signal/Kill? */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    copyin(p, &fswa, sizeof(struct _fs_write_args));

    /* Buffer */
    if (!useracc(fswa.buf, fswa.nbyte, VM_PROT_READ)) {
        /* No permission to read/write */
        /* TODO Signal/Kill? */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    buf = kmalloc(fswa.nbyte);
    copyin(fswa.buf, buf, fswa.nbyte);

    retval = fs_write_cproc(fswa.fildes, buf, fswa.nbyte,
                            &(fswa.offset));
    kfree(buf);

out:
    return retval;
}

static int fs_syscall_mount(struct _fs_mount_args * user_args)
{
    struct _fs_mount_args args;
    char * source_path = 0;
    char * target_path = 0;
    char * parm_str = 0;
    vnode_t * mpt;
    int retval = -1;

    /* Copyin args struct */
    if (!useracc(user_args, sizeof(struct _fs_mount_args), VM_PROT_READ)) {
        set_errno(EFAULT);
        goto out;
    }
    copyin(user_args, &args, sizeof(struct _fs_mount_args));

    /* Copyin string arguments strings */
    if (!useracc(args.source, args.source_len, VM_PROT_READ) &&
            !useracc(args.target, args.target_len, VM_PROT_READ) &&
            !useracc(args.parm, args.parm_len, VM_PROT_READ)) {
        set_errno(EFAULT);
        goto out;
    }
    source_path = kmalloc(args.source_len);
    target_path = kmalloc(args.target_len);
    parm_str = kmalloc(args.parm_len);
    if (!source_path || !target_path || !parm_str) {
        set_errno(ENOMEM);
        goto out;
    }
    copyin(args.source, source_path, args.source_len);
    copyin(args.target, target_path, args.target_len);
    copyin(args.parm, parm_str, args.parm_len);
    args.source = source_path;
    args.target = target_path;
    args.parm = parm_str;

    if (!fs_namei_proc(&mpt, (char *)args.target)) {
        set_errno(ENOENT); /* Mount point doesn't exist */
        goto out;
    }

    retval = fs_mount(mpt, args.source, args.fsname, args.mode,
                    args.parm, args.parm_len);
    if (retval != 0)
        set_errno(ENOENT); /* TODO Other ernos? */

out:
    if (source_path)
        kfree(source_path);
    if (target_path)
        kfree(target_path);
    if (parm_str)
        kfree(parm_str);

    return retval;
}

static int fs_syscall_open(void * user_args)
{
    struct _fs_open_args args;
    char * name = 0;
    vnode_t * file;
    int retval = 0;

    /* Copyin args struct */
    if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
        set_errno(EFAULT);
        goto out;
    }
    copyin(user_args, &args, sizeof(args));

    name = kmalloc(args.name_len);
    if (!name) {
        set_errno(ENFILE);
        goto out;
    }
    if (!useracc(args.name, args.name_len, VM_PROT_READ)) {
        set_errno(EFAULT);
        goto out;
    }
    copyin(args.name, &name, args.name_len);
    args.name = name;

    if (!fs_namei_proc(&file, name)) {
        if (args.oflags & O_CREAT) {
            /* TODO Create a file */
        } else {
            set_errno(ENOENT);
            goto out;
        }
    }

    retval = fs_fildes_create_cproc(file, args.oflags);
    if (retval) {
        set_errno(-retval);
        retval = -1;
    } else {
        retval = 0;
    }

out:
    if (name)
        kfree(name);
    return retval;
}

/**
 * fs syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uintptr_t fs_syscall(uint32_t type, void * p)
{
    switch (type) {
    case SYSCALL_FS_CREAT:
        set_errno(ENOSYS);
        return -1;

    case SYSCALL_FS_OPEN:
        return (uintptr_t)fs_syscall_open(p);

    case SYSCALL_FS_CLOSE:
        set_errno(ENOSYS);
        return -3;

    case SYSCALL_FS_READ:
        set_errno(ENOSYS);
        return -4;

    case SYSCALL_FS_WRITE:
        return (uintptr_t)fs_syscall_write(p);

    case SYSCALL_FS_LSEEK:
        set_errno(ENOSYS);
        return -6;

    case SYSCALL_FS_DUP:
        set_errno(ENOSYS);
        return -7;

    case SYSCALL_FS_LINK:
        set_errno(ENOSYS);
        return -8;

    case SYSCALL_FS_UNLINK:
        set_errno(ENOSYS);
        return -9;

    case SYSCALL_FS_STAT:
        set_errno(ENOSYS);
        return -10;

    case SYSCALL_FS_FSTAT:
        set_errno(ENOSYS);
        return -11;

    case SYSCALL_FS_ACCESS:
        set_errno(ENOSYS);
        return -12;

    case SYSCALL_FS_CHMOD:
        set_errno(ENOSYS);
        return -13;

    case SYSCALL_FS_CHOWN:
        set_errno(ENOSYS);
        return -14;

    case SYSCALL_FS_UMASK:
        set_errno(ENOSYS);
        return -15;

    case SYSCALL_FS_IOCTL:
        set_errno(ENOSYS);
        return -16;

    case SYSCALL_FS_MOUNT:
        return (uintptr_t)fs_syscall_mount(p);

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
