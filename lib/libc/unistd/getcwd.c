/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * prepend() tacks a directory name onto the front of a pathname.
 */
static char * prepend(char * dirname, char * pathname, size_t * pathsize,
                      size_t max_size)
{
    size_t i; /* directory name size counter */

    for (i = 0; *dirname != '\0'; i++, dirname++) {
        continue;
    }

    if ((*pathsize += i) < max_size) {
        while (i-- > 0) {
            *--pathname = *--dirname;
        }
    }

    return pathname;
}

char * getcwd(char * pathname, size_t size)
{
    char * pathbuf = NULL;  /* temporary pathname buffer */
    char * pnptr;           /* pathname pointer */
    char * curdir = NULL;   /* current directory buffer */
    char * dptr;            /* directory pointer */
    size_t pathsize = 0;
    dev_t cdev, rdev;       /* current & root device number */
    ino_t cino, rino;       /* current & root inode number */
    DIR * dirp;             /* directory stream */
    struct dirent * dir;    /* directory entry struct */
    struct stat d, dd;      /* file status struct */

    pathbuf = malloc(size);
    curdir = malloc(size);
    if (!(pathbuf && curdir)) {
        errno = ENOMEM;
        goto fail;
    }

    pnptr = &pathbuf[size - 1];
    *pnptr = '\0';
    if (stat("/", &d) < 0) {
        errno = EACCES;
        goto fail;
    }
    rdev = d.st_dev;
    rino = d.st_ino;

    dptr = curdir;
    strcpy(dptr, "./");
    dptr += 2;
    if (stat(curdir, &d) < 0) {
        errno = EACCES;
        goto fail;
    }

    while (1) {
        cino = d.st_ino;
        cdev = d.st_dev;
        if (cino == rino && cdev == rdev)
            break; /* reached root directory */

        strcpy(dptr, "../");
        dptr += 3;
        dirp = opendir(curdir);
        if (dirp == NULL) {
            errno = EACCES;
            goto fail;
        }
        if (fstat(dirfd(dirp), &d)) {
            errno = EACCES;
            goto fail;
        }

        if (cino == d.st_ino && cdev == d.st_dev) {
            /* reached root directory */
            closedir(dirp);
            break;
        }

        do {
            dir = readdir(dirp);
            if (dir == NULL) {
                closedir(dirp);
                errno = EACCES;
                goto fail;
            }
            strcpy(dptr, dir->d_name);
            if (lstat(curdir, &dd) < 0) {
                errno = EACCES;
                goto fail;
            }
        } while (dd.st_ino != cino || dd.st_dev != cdev);
        closedir(dirp);
        pnptr = prepend("/", prepend(dir->d_name, pnptr, &pathsize, size),
                        &pathsize, size);
    }
    if (*pnptr == '\0')     /* current dir == root dir */
        strcpy(pathname, "/");
    else
        strncpy(pathname, pnptr, size);

    free(pathbuf);
    free(curdir);
    return pathname;
fail:
    free(pathbuf);
    free(curdir);
    return NULL;
}
