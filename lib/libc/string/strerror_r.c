/*
 * strerror_r(int)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <string.h>
#include <sys/_PDCLIB_locale.h>

int strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
    char * buf;

    buf = strerror(errnum);
    strlcpy(strerrbuf, buf, buflen);

    return 0;
}
