/*
 * _PDCLIB_rename(const char *, const char *)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/_PDCLIB_glue.h>
#include <errno.h>

int _PDCLIB_rename(const char * old, const char * new)
{
    if (link(old, new) < 0) {
        if (errno != EEXIST) {
            return -1;
        }
        if (unlink(new) < 0 || link(old, new) < 0) {
            return -1;
        }
    }
    return unlink(old);
}
