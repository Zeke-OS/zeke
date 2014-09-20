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
#include <libkern.h>
#include <kstring.h>
#include <kmalloc.h>
#include <vm/vm.h>
#include <vm/vm_copyinstruct.h>
#include <proc.h>
#include <tsched.h>
#include <sys/priv.h>
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
    char * buf = 0;
    int err, retval;

    err = priv_check(curproc, PRIV_VFS_READ);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }

    /* Buffer */
    if (!useracc(args.buf, args.nbytes, VM_PROT_WRITE)) {
        /* No permission to read/write */
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }
    buf = kmalloc(args.nbytes);
    if (!buf) {
        set_errno(ENOMEM);
        retval = -1;
        goto out;
    }

    retval = fs_readwrite_cproc(args.fildes, buf, args.nbytes, O_RDONLY);
    if (retval < 0) {
        set_errno(-retval);
        retval = -1;
    }

    copyout(buf, args.buf, args.nbytes);
out:
    kfree(buf);
    return retval;
}

static int sys_write(void * user_args)
{
    struct _fs_readwrite_args args;
    char * buf = 0;
    int err, retval;

    err = priv_check(curproc, PRIV_VFS_WRITE);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

    /* Args */
    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }

    /* Buffer */
    buf = kmalloc(args.nbytes);
    if (!buf) {
        set_errno(ENOMEM);
        retval = -1;
        goto out;
    }
    err = copyin(args.buf, buf, args.nbytes);
    if (err) {
        set_errno(EFAULT);
        retval = -1;
        goto out;
    }

    retval = fs_readwrite_cproc(args.fildes, buf, args.nbytes, O_WRONLY);
    if (retval < 0) {
        set_errno(-retval);
        retval = -1;
    }

out:
    kfree(buf);
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
    struct _fs_open_args * args = 0;
    vnode_t * file;
    int err, retval = -1;

    /* Copyin args struct */
    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_open_args),
            GET_STRUCT_OFFSETS(struct _fs_open_args,
                name, name_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    if (args->name_len < 2) { /* File name too short */
        set_errno(EINVAL);
        goto out;
    }

    /* Validate name string */
    if (!strvalid(args->name, args->name_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    err = fs_namei_proc(&file, args->fd, args->name, args->atflags);
    if (err) {
        if (args->oflags & O_CREAT) {
            /* Create a new file */
            /* TODO Determine correct mode bits?? */
            retval = fs_creat_cproc(args->name, S_IFREG | args->mode, &file);
            if (retval) {
                set_errno(-retval);
                goto out;
            }
        } else {
            set_errno(ENOENT);
            goto out;
        }
    }

    retval = fs_fildes_create_cproc(file, args->oflags);
    if (retval < 0) {
        set_errno(-retval);
        retval = -1;
    }

out:
    freecpystruct(args);
    return retval;
}

static int sys_close(void * p)
{
    int err;
    int fildes = (int)p;

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
    int err, count = 0;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

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

    bytes_left = args.nbytes;
    while (bytes_left >= sizeof(struct dirent)) {
        vnode_t * vnode = fildes->vnode;
        if (vnode->vnode_ops->readdir(vnode, &d, &fildes->seek_pos))
            break;
        dents[count++] = d;

        bytes_left -= (sizeof(struct dirent));
    }

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

static int sys_link(void * user_args)
{
    struct _fs_link_args * args;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_link_args),
            GET_STRUCT_OFFSETS(struct _fs_link_args,
                path1, path1_len,
                path2, path2_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    /* Validate strings */
    if (!strvalid(args->path1, args->path1_len) ||
                  !strvalid(args->path2, args->path2_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    err = fs_link_curproc(args->path1, args->path1_len,
                args->path2, args->path2_len);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    freecpystruct(args);
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

    /* Validate path string */
    if (!strvalid(args->path, args->path_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    err = fs_unlink_curproc(args->fd, args->path, args->path_len, args->flag);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    freecpystruct(args);
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

    /* Validate path string */
    if (!strvalid(args->path, args->path_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    err = fs_mkdir_curproc(args->path, args->mode);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    freecpystruct(args);
    return retval;
}

static int sys_rmdir(void * user_args)
{
    struct _fs_rmdir_args * args = 0;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_rmdir_args),
            GET_STRUCT_OFFSETS(struct _fs_rmdir_args,
                path, path_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    /* Validate path string */
    if (!strvalid(args->path, args->path_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    err = fs_rmdir_curproc(args->path);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
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

    /* Validate path string */
    if (!strvalid(args->path, args->path_len)) {
        set_errno(ENAMETOOLONG);
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
        if (fs_namei_proc(&vnode, -1, (char *)args->path, AT_FDCWD)) {
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
    freecpystruct(args);
    return retval;
}

static int sys_access(void * user_args)
{
    struct _fs_access_args * args = 0;
    vnode_t * vnode;
    uid_t euid;
    gid_t egid;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _fs_access_args),
            GET_STRUCT_OFFSETS(struct _fs_access_args,
                path, path_len));
    if (err) {
        set_errno(-err);
        goto out;
    }

    if (!strvalid(args->path, args->path_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    if (!(args->flag & AT_FDARG)) { /* access() */
        err = fs_namei_proc(&vnode, -1, args->path, AT_FDCWD);
        if (err) {
            set_errno(-err);
            goto out;
        }
    } else { /* faccessat() */
        fs_namei_proc(&vnode, args->fd, args->path, AT_FDARG);

        set_errno(ENOTSUP);
        return -1;
    }

    if (args->flag & AT_EACCESS) {
        euid = curproc->euid;
        egid = curproc->egid;
    } else {
        euid = curproc->uid;
        egid = curproc->gid;
    }

    if (args->amode & F_OK) {
        retval = 0;
        goto out;
    }
    retval = chkperm_vnode(vnode, euid, egid, args->amode);

out:
    return retval;
}

/* Only fchmod() is implemented at the kernel level and rest must be implemented
 * in user space. */
static int sys_chmod(void * user_args)
{
    struct _fs_chmod_args args;
    int err;

    /* Copyin args struct */
    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return fs_chmod_curproc(args.fd, args.mode);
}

/* Only fchown() is implemented at the kernel level and rest must be implemented
 * in user space. */
static int sys_chown(void * user_args)
{
    struct _fs_chown_args args;
    int err;

    /* Copyin args struct */
    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return fs_chown_curproc(args.fd, args.owner, args.group);
}

static int sys_umask(void * user_args)
{
    struct _fs_umask_args args;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }

    copyin(user_args, &args, sizeof(args));
    args.oldumask = curproc->files->umask;
    curproc->files->umask = args.newumask;
    copyout(&args, user_args, sizeof(args));

    return 0;
}

static int sys_mount(void * user_args)
{
    struct _fs_mount_args * args = 0;
    vnode_t * mpt;
    int err;
    int retval = -1;

    err = priv_check(curproc, PRIV_VFS_MOUNT);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

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

    /* Validate path strings */
    if (!strvalid(args->source, args->source_len) ||
        !strvalid(args->target, args->target_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    if (fs_namei_proc(&mpt, -1, (char *)args->target, AT_FDCWD)) {
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
    freecpystruct(args);
    return retval;
}

/**
 * Declarations of fs syscall functions.
 */
static const syscall_handler_t fs_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_OPEN, sys_open),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_CLOSE, sys_close),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_READ, sys_read),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_WRITE, sys_write),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_LSEEK, sys_lseek),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_GETDENTS, sys_getdents),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_FCNTL, sys_fcntl),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_LINK, sys_link),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_UNLINK, sys_unlink),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_MKDIR, sys_mkdir),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_RMDIR, sys_rmdir),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_STAT, sys_filestat),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_FSTAT, 0), /* Not implemented */
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_ACCESS, sys_access),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_CHMOD, sys_chmod),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_CHOWN, sys_chown),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_UMASK, sys_umask),
    ARRDECL_SYSCALL_HNDL(SYSCALL_FS_MOUNT, sys_mount)
};
SYSCALL_HANDLERDEF(fs_syscall, fs_sysfnmap)
