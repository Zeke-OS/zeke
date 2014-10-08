/**
 *******************************************************************************
 * @file    unistd.c
 * @author  Olli Vanhoja
 * @brief   Standard functions.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <syscall.h>

pid_t fork(void)
{
    return (pid_t)syscall(SYSCALL_PROC_FORK, NULL);
}

int chdir(const char * path)
{
    struct _proc_chdir_args args = {
        .name       = path,
        .name_len   = strlen(path) + 1,
        .atflags    = AT_FDCWD
    };

    return (int)syscall(SYSCALL_PROC_CHDIR, &args);
}

int access(const char * path, int amode)
{
    struct _fs_access_args args = {
        .path = path,
        .amode = amode,
        .flag = 0
    };

    return (int)syscall(SYSCALL_FS_ACCESS, &args);
}

int faccessat(int fd, const char * path, int amode, int flag)
{
    struct _fs_access_args args = {
        .path = path,
        .amode = amode,
        .flag = AT_FDARG | flag
    };

    return (int)syscall(SYSCALL_FS_ACCESS, &args);
}

int chown(const char *path, uid_t owner, gid_t group)
{
    int err;
    struct _fs_chown_args args = {
        .owner = owner,
        .group = group
    };

    args.fd = open(path, O_WRONLY);
    if (args.fd < 0)
        return -1;

    err = syscall(SYSCALL_FS_CHOWN, &args);

    close(args.fd);

    return err;
}

int fchownat(int fd, const char *path, uid_t owner, gid_t group,
             int flag)
{
    int err;
    struct _fs_chown_args args = {
        .owner = owner,
        .group = group
    };

    args.fd = openat(fd, path, O_WRONLY, flag);
    if (args.fd < 0)
        return -1;

    err = syscall(SYSCALL_FS_CHOWN, &args);

    close(args.fd);

    return err;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    struct _fs_chown_args args = {
        .fd = fd,
        .owner = owner,
        .group = group
    };

    return (int)syscall(SYSCALL_FS_CHOWN, &args);
}

pid_t getpid(void)
{
    pid_t pid;
    int err;

    err = (int)syscall(SYSCALL_PROC_GETPID, &pid);

    if (err)
        return -1;
    return pid;
}

pid_t getppid(void)
{
    pid_t pid;
    int err;

    err = (int)syscall(SYSCALL_PROC_GETPPID, &pid);

    if (err)
        return -1;
    return pid;
}

ssize_t read(int fildes, void * buf, size_t nbytes)
{
    struct _fs_readwrite_args args = {
        .fildes = fildes,
        .buf = buf,
        .nbytes = nbytes
    };

    return (ssize_t)syscall(SYSCALL_FS_READ, &args);
}

#if 0
ssize_t pwrite(int fildes, const void * buf, size_t nbyte,
        off_t offset)
{
    struct _fs_readwrite_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbytes = nbyte
    };

    /* TODO Seek first */
    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}
#endif

ssize_t write(int fildes, const void * buf, size_t nbyte)
{
    struct _fs_readwrite_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbytes = nbyte
    };

    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}

off_t lseek(int fildes, off_t offset, int whence)
{
    int err;
    struct _fs_lseek_args args = {
        .fd = fildes,
        .offset = offset,
        .whence = whence
    };

    err = (int)syscall(SYSCALL_FS_LSEEK, &args);
    if (err)
        return -1;

    return args.offset;
}

int dup(int fildes)
{
    return fcntl(fildes, F_DUPFD, 0);
}

int dup2(int fildes, int fildes2)
{
    return fcntl(fildes, F_DUP2FD, fildes2);
}

int link(const char * path1, const char * path2)
{
    struct _fs_link_args args = {
        .path1 = path1,
        .path1_len = strlen(path1) + 1,
        .path2 = path2,
        .path2_len = strlen(path2) + 1
    };

    return (int)syscall(SYSCALL_FS_LINK, &args);
}

int unlink(const char * path)
{
    struct _fs_unlink_args args = {
        .path = path,
        .path_len = strlen(path) + 1,
        .flag = AT_FDCWD
    };

    return (int)syscall(SYSCALL_FS_UNLINK, &args);
}

int unlinkat(int fd, const char * path, int flag)
{
    if (!(flag & AT_FDCWD))
        flag |= AT_FDARG;

    struct _fs_unlink_args args = {
        .fd = fd,
        .path = path,
        .path_len = strlen(path) + 1,
        .flag = flag
    };

    return (int)syscall(SYSCALL_FS_UNLINK, &args);
}
