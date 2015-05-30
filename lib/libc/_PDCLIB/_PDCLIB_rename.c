/*
 * _PDCLIB_rename( const char *, const char * )
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
    /*
     * TODO if new exists it should be unlink'd first.
     */
    if (link(old, new) == 0)
    {
        if (unlink(old) == EOF) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return EOF;
    }
}
