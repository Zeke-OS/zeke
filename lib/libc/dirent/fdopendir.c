/*
 * Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

DIR * fdopendir(int fd)
{
    struct stat st;
    DIR * dirp;

    if (fd == -1 || fstat(fd, &st)) {
        errno = EBADF;
        return NULL;
    }

    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return NULL;
    }

    dirp = malloc(sizeof(DIR));
    if (!dirp) {
        close(fd);
        return NULL;
    }
    dirp->dd_fd = fd;
    dirp->dd_loc = 0;
    dirp->dd_count = 0;

    return dirp;
}
