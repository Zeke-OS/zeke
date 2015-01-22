/**
 *******************************************************************************
 * @file    posix_pt.c
 * @author  Olli Vanhoja
 * @brief   Open pty.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <sys/param.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int posix_openpt(int flags)
{
    int fd, err;

    fd = open("/dev/ptmx", flags & (O_RDWR | O_NOCTTY));
    if (fd < 0) {
        if (errno != EMFILE && errno != ENFILE)
            errno = EAGAIN;
        return fd;
    }

    err = _ioctl(fd, IOCTL_PTY_CREAT, NULL, 0);
    if (err < 0) {
        errno = EAGAIN;
        return -1;
    }

    return fd;
}

int grantpt(int fildes)
{
    int err = _ioctl(fildes, IOCTL_PTY_GRANT, NULL, 0);
    if (err < 0)
        return -1;
}

int unlockpt(int fildes)
{
    return 0;
}

char * ptsname(int fildes)
{
    const char devpath[] = "/dev/";
    char dev[SPECNAMELEN];
    char * path;
    const size_t size = sizeof(devpath) + sizeof(dev);
    int err;

    path = malloc(size);
    if (!path) {
        errno = ENOMEM;
        return NULL;
    }

    memcpy(path, devpath, sizeof(devpath));
    err = _ioctl(fildes, IOCTL_PTY_PTSNAME, path + sizeof(devpath),
                 size - sizeof(devpath));
    if (err < 0) {
        errno = EINVAL;
        return NULL;
    }

    return path;
}
