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
#include <vm_copyinstruct.h>
#include <proc.h>
#include <sched.h>
#include <ksignal.h>
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <fs/fs.h>

static int sys_read(void * user_args)
{
    struct _fs_readwrite_args args;
    char * buf;
    int retval;

    if (!useracc(user_args, sizeof(struct _fs_readwrite_args), VM_PROT_READ)) {
        /* No permission to read/write */
        /* TODO Signal/Kill? */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    copyin(user_args, &args, sizeof(struct _fs_readwrite_args));

    /* Buffer */
    if (!useracc(args.buf, args.nbytes, VM_PROT_WRITE)) {
        /* No permission to read/write */
        /* TODO Signal/Kill? */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    buf = kmalloc(args.nbytes);

    retval = fs_readwrite_cproc(args.fildes, buf, args.nbytes, O_RDONLY);
    if (retval < 0) {
        set_errno(-retval);
        retval = -1;
    }

    copyout(buf, args.buf, args.nbytes);
    kfree(buf);
out:
    return retval;
}

static int sys_write(void * user_args)
{
    struct _fs_readwrite_args args;
    char * buf;
    int retval;

    /* Args */
    if (!useracc(user_args, sizeof(struct _fs_readwrite_args), VM_PROT_READ)) {
        /* No permission to read/write */
        /* TODO Signal/Kill? */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    copyin(user_args, &args, sizeof(struct _fs_readwrite_args));

    /* Buffer */
    if (!useracc(args.buf, args.nbytes, VM_PROT_READ)) {
        /* No permission to read/write */
        /* TODO Signal/Kill? */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    buf = kmalloc(args.nbytes);
    copyin(args.buf, buf, args.nbytes);

    retval = fs_readwrite_cproc(args.fildes, buf, args.nbytes, O_WRONLY);
    if (retval < 0) {
        set_errno(-retval);
        retval = -1;
    }

    kfree(buf);
out:
    return retval;
}

static int sys_open(void * user_args)
{
    struct _fs_open_args args;
    char * name = 0;
    vnode_t * file;
    int err, retval = 0;

    /* Copyin args struct */
    if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
        set_errno(EFAULT);
        goto out;
    }
    copyin(user_args, &args, sizeof(args));

    if (args.name_len < 2) {
        set_errno(EINVAL);
        goto out;
    }

    name = kmalloc(args.name_len);
    if (!name) {
        set_errno(ENFILE);
        goto out;
    }
    if (!useracc(args.name, args.name_len, VM_PROT_READ)) {
        set_errno(EFAULT);
        goto out;
    }
    copyinstr(args.name, name, args.name_len, 0);
    args.name = name;

    if ((err = fs_namei_proc(&file, name))) {
        if (args.oflags & O_CREAT) {
            /* Create a new file */
            /* TODO Determine correct mode bits?? */
            retval = fs_creat_cproc(name, S_IFREG | args.mode, &file);
            if (retval) {
                set_errno(-retval);
                goto out;
            }
        } else {
            set_errno(ENOENT);
            goto out;
        }
    }

    retval = fs_fildes_create_cproc(file, args.oflags);
    if (retval < 0) {
        set_errno(-retval);
        retval = -1;
    }

out:
    if (name)
        kfree(name);
    return retval;
}

static int sys_close(int fildes)
{
    if(!fs_fildes_ref(curproc->files, fildes, 0)) {
        set_errno(EBADF);
        return -1;
    }
    fs_fildes_ref(curproc->files, fildes, -1);
    curproc->files->fd[fildes] = NULL;

    return 0;
}

static int sys_getdents(void * user_args)
{
    struct _ds_getdents_args args;
    struct dirent * dents;
    size_t bytes_left;
    file_t * fildes;
    struct dirent d; /* Temp storage */
    int count = 0;

    if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
        set_errno(EFAULT);
        return -1;
    }
    copyin(user_args, &args, sizeof(struct _ds_getdents_args));

    /* We must have a write access to the given buffer. */
    if (!useracc(args.buf, args.nbytes, VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }

    fildes = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!fildes) {
        set_errno(EBADF);
        return -1;
    }

    dents = kmalloc(args.nbytes);
    if (!dents) {
        fs_fildes_ref(curproc->files, args.fd, -1);
        set_errno(ENOMEM);
        return -1;
    }

    /*
     * This is a bit tricky, if we are here for the first time there should be a
     * magic value 0x00000000FFFFFFFF set to seek_pos but we can't do a thing if
     * fildes was initialized incorrectly, so lets cross our fingers.
     */
    d.d_off = fildes->seek_pos;
    bytes_left = args.nbytes;
    while (bytes_left >= sizeof(struct dirent)) {
        vnode_t * vnode = fildes->vnode;
        if (vnode->vnode_ops->readdir(vnode, &d))
            break;
        dents[count++] = d;

        bytes_left -= (sizeof(struct dirent));
    }
    fildes->seek_pos = d.d_off;
    fs_fildes_ref(curproc->files, args.fd, -1);

    copyout(dents, args.buf, count * sizeof(struct dirent));
    kfree(dents);

    return count;
}

static int sys_mkdir(void * user_args)
{
    struct _fs_mkdir_args * args = 0;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_mkdir_args),
            GET_STRUCT_OFFSETS(struct _fs_mkdir_args,
                path, path_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    err = fs_mkdir_curproc(args->path, args->mode);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    if (args)
        freecpystruct(args);
    return retval;
}

static int sys_filestat(void * user_args)
{
    struct _fs_stat_args * args = 0;
    vnode_t * vnode;
    struct stat stat_buf;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_stat_args),
            GET_STRUCT_OFFSETS(struct _fs_stat_args,
                path, path_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    if (!useracc(args->buf, sizeof(struct stat), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        goto out;
    }

    if (args->flags & AT_FDARG) { /* by fildes */
        file_t * fildes;
        /* Note: AT_SYMLINK_NOFOLLOW == O_NOFOLLOW */
        const int ofalgs = (args->flags & AT_SYMLINK_NOFOLLOW);

        fildes = fs_fildes_ref(curproc->files, args->fd, 1);
        if (!fildes) {
            set_errno(EBADF);
            goto out;
        }

        err = fildes->vnode->vnode_ops->stat(fildes->vnode, &stat_buf);
        if (!err) {
            /* Check if fildes was opened with O_SEARCH or if not then if we
             * have a permission to search by file permissions. */
            if (fildes->oflags & O_SEARCH || chkperm_cproc(&stat_buf, O_EXEC))
                err = lookup_vnode(&vnode, fildes->vnode, args->path, ofalgs);
            else /* No permission to search */
                err = -EACCES;
        }

        fs_fildes_ref(curproc->files, args->fd, -1);
        if (err) {
            set_errno(-err);
            goto out;
        }
    } else { /* search by path */
        if (fs_namei_proc(&vnode, (char *)args->path)) {
            set_errno(ENOENT);
            goto out;
        }
    }

    if ((args->flags & AT_FDARG) &&
        (args->flags & O_EXEC)) { /* Get stat of given fildes, which we have
                                   * have in stat_buf. */
        goto ready;
    }

    err = vnode->vnode_ops->stat(vnode, &stat_buf);
    if (err) {
        set_errno(-err);
        goto out;
    }

ready:
    copyout(&stat_buf, args->buf, sizeof(struct stat));
    retval = 0;
out:
    if (args)
        freecpystruct(args);
    return retval;
}

static int sys_mount(struct _fs_mount_args * user_args)
{
    struct _fs_mount_args * args = 0;
    vnode_t * mpt;
    int err;
    int retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_mount_args),
            GET_STRUCT_OFFSETS(struct _fs_mount_args,
                source, source_len,
                target, target_len,
                parm,   parm_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    if (fs_namei_proc(&mpt, (char *)args->target)) {
        set_errno(ENOENT); /* Mount point doesn't exist */
        goto out;
    }

    err = fs_mount(mpt, args->source, args->fsname, args->flags,
                    args->parm, args->parm_len);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    if (args)
        freecpystruct(args);

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
    case SYSCALL_FS_OPEN:
        return (uintptr_t)sys_open(p);

    case SYSCALL_FS_CLOSE:
        return (uintptr_t)sys_close((int)p);

    case SYSCALL_FS_READ:
        return (uintptr_t)sys_read(p);

    case SYSCALL_FS_WRITE:
        return (uintptr_t)sys_write(p);

    case SYSCALL_FS_LSEEK:
        set_errno(ENOSYS);
        return -6;

    case SYSCALL_FS_GETDENTS:
        return (uintptr_t)sys_getdents(p);

    case SYSCALL_FS_DUP:
        set_errno(ENOSYS);
        return -7;

    case SYSCALL_FS_LINK:
        set_errno(ENOSYS);
        return -8;

    case SYSCALL_FS_UNLINK:
        set_errno(ENOSYS);
        return -9;

    case SYSCALL_FS_MKDIR:
        return (uintptr_t)sys_mkdir(p);

    case SYSCALL_FS_RMDIR:
        set_errno(ENOSYS);
        return -92;

    case SYSCALL_FS_STAT:
        return (uintptr_t)sys_filestat(p);
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

    case SYSCALL_FS_MOUNT:
        return (uintptr_t)sys_mount(p);

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
