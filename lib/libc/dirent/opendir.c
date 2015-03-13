/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

DIR * opendir(const char * name)
{
    register DIR * dirp;
    register int fd;

    if ((fd = open(name, O_DIRECTORY)) == -1)
        return NULL;

    if ((dirp = malloc(sizeof(DIR))) == NULL) {
        close(fd);
        return NULL;
    }
    dirp->dd_fd = fd;
    dirp->dd_loc = 0;
    dirp->dd_count = 0;

    return dirp;
}
