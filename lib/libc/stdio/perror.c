/*
 * perror( const char * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/_PDCLIB_locale.h>

void perror(const char * s)
{
    if ((s != NULL) && (s[0] != '\n')) {
        fprintf(stderr, "%s: ", s);
    }

    if (errno >= _PDCLIB_ERRNO_MAX) {
        fprintf(stderr, "Unknown error\n");
    } else {
        fprintf(stderr, "%s\n", _PDCLIB_threadlocale()->_ErrnoStr[errno]);
    }

    return;
}
