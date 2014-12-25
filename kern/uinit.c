/**
 *******************************************************************************
 * @file    uinit.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
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

#include <unistd.h>
#include <fcntl.h>
#include <mount.h>
#include <kstring.h>
#include <kerror.h>
#include <time.h>
#include <sys/stat.h>

#include <../lib/libc/sys.c>
#include <../lib/libc/unistd/write.c>

static int _geterrno(void)
{
    int * ep;

    ep = (int *)syscall(SYSCALL_THREAD_GETERRNO, NULL);

    return *ep;
}

static int _mkdir(const char * path, mode_t mode)
{
    struct _fs_mkdir_args args = {
        .path = path,
        .path_len = strlenn(path, PATH_MAX) + 1,
        .mode = mode
    };

    return syscall(SYSCALL_FS_MKDIR, &args);
}

static int _mount(const char * source, const char * target, const char * type)
{
    struct _fs_mount_args args = {
        .source = source,
        .source_len = strlenn(source, PATH_MAX) + 1,
        .target = target,
        .target_len = strlenn(target, PATH_MAX) + 1,
        .flags = 0,
        .parm = "",
        .parm_len = 1
    };

    strcpy((char *)args.fsname, type);

    return syscall(SYSCALL_FS_MOUNT, &args);
}

static int _chdir(char * path)
{
    struct _proc_chdir_args args = {
        .name       = path,
        .name_len   = strlenn(path, PATH_MAX) + 1,
        .atflags    = AT_FDCWD
    };

    return (int)syscall(SYSCALL_PROC_CHDIR, &args);
}

static int _chrootcwd(void)
{
    return syscall(SYSCALL_FS_CHROOT, NULL);
}

static int _execve(const char * path, char * const argv[], size_t nargv,
                   char * const envp[], size_t nenvp)
{
    struct _fs_open_args open_args = {
        .name = path,
        .name_len = strlenn(path, PATH_MAX) + 1,
        .oflags = O_EXEC | O_CLOEXEC,
        .atflags = AT_FDCWD
    };
    struct _exec_args exec_args = {
        .argv = argv,
        .nargv = nargv,
        .env = envp,
        .nenv = nenvp
    };
    int retval = 0;

    exec_args.fd = syscall(SYSCALL_FS_OPEN, &open_args);
    if (exec_args.fd >= 0) {
        retval = syscall(SYSCALL_EXEC_EXEC, &exec_args);
    } else {
        return -1;
    }

    syscall(SYSCALL_FS_CLOSE, (void *)exec_args.fd);

    return retval;
}

static void printfail(char * str)
{
    char buf[80];
    size_t len;

    len = ksprintf(buf, sizeof(buf), "%s (errno = %d).\n", str, _geterrno());
    write(STDERR_FILENO, buf, len + 1);
}

void * uinit(void * arg)
{
    char *argv[] = { "/sbin/init", NULL };
    char *env[] = { NULL };
    int err;

    write(STDOUT_FILENO, "uinit\n", 7);

    _mkdir("/dev", S_IRWXU | S_IRGRP | S_IXGRP);
    err = _mount("", "/dev", "devfs");
    if (err) {
        printfail("can't mount /dev");
        while (1);
    }

     _mkdir("/mnt", S_IRWXU | S_IRGRP | S_IXGRP);
     err = _mount("/dev/emmc0", "/mnt", "fatfs");
     if (err) {
        printfail("can't mount sd card");
        while (1);
     }

     _chdir("/mnt");
    _chrootcwd();

    err = _mount("", "/dev", "devfs");
    if (err) {
        printfail("can't mount /dev");
        while(1);
    }

    err = _mount("", "/proc", "procfs");
    if (err) {
        printfail("can't mount /proc");
        while(1);
    }

    /* Exec init */
    err = _execve(argv[0], argv, num_elem(argv), env, num_elem(env));
    if (err) {
        printfail("exec init failed");
        while (1);
    }

    write(STDERR_FILENO, "fail", 5);
    while (1);

    return NULL;
}
