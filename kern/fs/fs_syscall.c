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
#include <syscall.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <fs/fs.h>

static int sys_read(void * user_args)
{
    struct _fs_readwrite_args args;
    char * buf;
    int retval;

    if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
        /* No permission to read/write */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    copyin(user_args, &args, sizeof(args));

    /* Buffer */
    if (!useracc(args.buf, args.nbytes, VM_PROT_WRITE)) {
        /* No permission to read/write */
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
    if (!useracc(user_args, sizeof(args), VM_PROT_READ)) {
        /* No permission to read/write */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    copyin(user_args, &args, sizeof(args));

    /* Buffer */
    if (!useracc(args.buf, args.nbytes, VM_PROT_READ)) {
        /* No permission to read/write */
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

static int sys_lseek(void * user_args)
{
    struct _fs_lseek_args args;
    file_t * file;
    int retval = 0;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        /* No permission to read/write */
        set_errno(EFAULT);
        return -1;
    }
    copyin(user_args, &args, sizeof(args));

    /* Increment refcount for the file pointed by fd */
    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        return -1;
    }

    if (args.whence == SEEK_SET)
        file->seek_pos = args.offset;
    else if (args.whence == SEEK_CUR)
        file->seek_pos += args.offset;
    else if (args.whence == SEEK_END) {
        struct stat stat_buf;
        int err;

        err = file->vnode->vnode_ops->stat(file->vnode, &stat_buf);
        if (!err) {
            const off_t new_offset = stat_buf.st_size + args.offset;

            if (new_offset >= stat_buf.st_size)
                file->seek_pos = new_offset;
            else {
                set_errno(EOVERFLOW);
                retval = -1;
            }
        } else {
            set_errno(EBADF);
            retval = -1;
        }
    } else {
        set_errno(EINVAL);
        retval = -1;
    }

    /* Resulting offset is stored to args */
    args.offset = file->seek_pos;

    /* Decrement refcount for the file pointed by fd */
    fs_fildes_ref(curproc->files, args.fd, -1);

    copyout(&args, user_args, sizeof(args));
    return retval;
}

static int sys_open(void * user_args)
{
    struct _fs_open_args args;
    char * name = 0;
    vnode_t * file;
    int err, retval = -1;

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
    int err;

    err = fs_fildes_close_cproc(fildes);
    if (err) {
        set_errno(-err);
        return -1;
    }

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
    copyin(user_args, &args, sizeof(args));

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

    if (!S_ISDIR(fildes->vnode->vn_mode)) {
        count = -1;
        set_errno(ENOTDIR);
        goto out;
    }

    dents = kmalloc(args.nbytes);
    if (!dents) {
        count = -1;
        set_errno(ENOMEM);
        goto out;
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

    copyout(dents, args.buf, count * sizeof(struct dirent));
    kfree(dents);

out:
    fs_fildes_ref(curproc->files, args.fd, -1);
    return count;
}

static int sys_fcntl(void * user_args)
{
    struct _fs_fcntl_args args;
    file_t * file;
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

    switch (args.cmd) {
    case F_DUPFD_CLOEXEC:
        file->fdflags = FD_CLOEXEC;
    case F_DUPFD:
    {
        int new_fd;

        new_fd = fs_fildes_cproc_next(file, args.third.ival);
        if (new_fd < 0) {
            set_errno(-new_fd);
            goto out;
        }

        fs_fildes_ref(curproc->files, new_fd, 1);
        retval = new_fd;
        break;
    }
    case F_DUP2FD:
    {
        int new_fd = args.third.ival;

        if (args.fd == new_fd) {
            retval = new_fd;
            goto out;
        }

        if (curproc->files->fd[new_fd]) {
            if (fs_fildes_close_cproc(new_fd)) {
                set_errno(EIO);
                goto out;
            }
        }

        new_fd = fs_fildes_cproc_next(file, new_fd);
        if (new_fd < 0) {
            set_errno(-new_fd);
            goto out;
        }
        if (new_fd != args.third.ival ||
            !fs_fildes_ref(curproc->files, new_fd, 1)) {
            fs_fildes_close_cproc(new_fd);
            set_errno(EIO);
            goto out;
        }

        retval = new_fd;
        break;
    }
    case F_GETFD:
        retval = file->fdflags;
        break;
    case F_SETFD:
        file->fdflags = args.third.ival;
        break;
    case F_GETFL:
        retval = file->oflags;
        break;
    case F_SETFL:
        /* TODO sync will need some operations to be done */
        file->oflags = args.third.ival & (O_APPEND | O_SYNC | O_NONBLOCK);
        retval = 0;
        break;
    case F_GETOWN:
        /* TODO */
    case F_SETOWN:
        /* TODO */
    case F_GETLK:
        /* TODO */
    case F_SETLK:
        /* TODO */
    case F_SETLKW:
        /* TODO */
    default:
        set_errno(EINVAL);
    }

out:
    fs_fildes_ref(curproc->files, args.fd, -1);
    return retval;
}

static int sys_unlink(void * user_args)
{
    struct _fs_unlink_args * args;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_unlink_args),
            GET_STRUCT_OFFSETS(struct _fs_unlink_args,
                path, path_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    err = fs_unlink_curproc(args->path);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    return retval;
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
        if (err) { /* Handle previous error */
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
        return (uintptr_t)sys_lseek(p);

    case SYSCALL_FS_GETDENTS:
        return (uintptr_t)sys_getdents(p);

    case SYSCALL_FS_FCNTL:
        return (uintptr_t)sys_fcntl(p);

    case SYSCALL_FS_LINK:
        set_errno(ENOSYS);
        return -8;

    case SYSCALL_FS_UNLINK:
        return (uintptr_t)sys_unlink(p);

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
