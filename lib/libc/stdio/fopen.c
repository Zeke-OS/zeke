/*
 * fopen( const char *, const char * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/_PDCLIB_io.h>
#include <sys/_PDCLIB_glue.h>
#include <string.h>
#include <errno.h>

extern FILE * _PDCLIB_filelist;

FILE * fopen(const char * _PDCLIB_restrict filename,
             const char * _PDCLIB_restrict mode)
{
    int imode = _PDCLIB_filemode(mode);
    _PDCLIB_fd_t fd;
    const _PDCLIB_fileops_t * ops;
    FILE * f;

    if(imode == 0 || filename == NULL)
        return NULL;

    if(!_PDCLIB_open(&fd, &ops, filename, imode)) {
        return NULL;
    }

    f = _PDCLIB_fvopen(fd, ops, imode, filename);
    if(!f) {
        int saveErrno;

        saveErrno = errno;
        ops->close(fd);
        errno = saveErrno;
    }

    return f;
}
