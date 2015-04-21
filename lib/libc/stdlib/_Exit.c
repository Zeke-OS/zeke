/* _Exit( int )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/_PDCLIB_glue.h>

void _Exit(int status)
{
    _exit(status);
    __builtin_unreachable();
}
