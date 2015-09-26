/**
 *******************************************************************************
 * @file    uinit.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <fcntl.h>
#include <mount.h>
#include <paths.h>
#include <time.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>
#include <kstring.h>
#include <hal/core.h>
#include "uinit.h"

#include <../lib/libc/sys.c>
#include <../lib/libc/unistd/write.c>

static int * ep;

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
    return syscall(SYSCALL_PROC_CHROOT, NULL);
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

static void fail(char * str)
{
    char buf[80];
    size_t len;

    len = ksprintf(buf, sizeof(buf), "%s (errno = %d).\n", str, *ep);
    write(STDERR_FILENO, buf, len + 1);

    uinit_exit();
}

static void init_errno(void)
{
    struct _sched_tls_desc * tls;

    tls = (__kernel struct _sched_tls_desc *)core_get_tls_addr();

    ep = &tls->errno_val;
}

static void get_rootfs(char * root, char * fsname, const char * rootfs)
{
    const char delim[] = " ";
    char str[40];
    char * strp = str;
    char * token;

    strlcpy(str, rootfs, sizeof(str));

    /* Set first bytes in case of strsep() fails */
    root[0] = '\0';
    fsname[0] = '\0';

    token = strsep(&strp, delim);
    if (token)
        strlcpy(root, token, 40);
    token = strsep(&strp, delim);
    if (token)
        strlcpy(fsname, token, 10);
}

/**
 * A function to initialize the user space and execute the actual init process.
 * This function is kind of special because it's executed in a separate context
 * from the kernel but still the binary is located in the kernel vm region.
 * @param arg is the value of kern.root sysctl.
 */
void * uinit(void * arg)
{
    char root[40];
    char fsname[10];
    char * argv[] = { "/sbin/sinit", NULL };
    char * env[] = {  NULL };

    init_errno();

    _mkdir("/dev", S_IRWXU | S_IRGRP | S_IXGRP);
    if (_mount("", "/dev", "devfs"))
        fail("can't mount /dev");

    _mkdir("/mnt", S_IRWXU | S_IRGRP | S_IXGRP);
    get_rootfs(root, fsname, (char *)arg);
    if (_mount(root, "/mnt", fsname))
        fail("can't mount sd card");

    _chdir("/mnt");
    _chrootcwd();

#if configDEVFS
    if (_mount("", "/dev", "devfs"))
        fail("Failed to mount /dev");
#endif

#if configPROCFS
    if (_mount("", "/proc", "procfs"))
        fail("Failed to mount /proc");
#endif

    if (_mount("", "/tmp", "ramfs"))
        fail("Failed to mount /tmp");

    /* Exec init */
    if (_execve(argv[0], argv, num_elem(argv), env, num_elem(env)))
        fail("exec init failed");

    return NULL;
}

void uinit_exit(void)
{
    const char msg[] = "init is exiting\n";

    write(STDERR_FILENO, msg, sizeof(msg));

    *ep = EX_OSERR;
    syscall(SYSCALL_PROC_EXIT, 0);
    __builtin_unreachable();
}
