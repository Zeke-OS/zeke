/* remove( const char * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <string.h>
#include <sys/_PDCLIB_io.h>
#include <unistd.h>

extern FILE * _PDCLIB_filelist;

int remove(const char * pathname)
{
    FILE * current = _PDCLIB_filelist;
    while (current != NULL) {
        if ((current->filename != NULL) &&
            (strcmp(current->filename, pathname) == 0)) {
            return EOF;
        }
        current = current->next;
    }
    return unlink(pathname);
}
