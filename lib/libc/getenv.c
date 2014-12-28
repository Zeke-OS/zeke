/*
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

extern char * _findenv(const char * name, size_t * offset);

/*
 * getenv(name) --
 *  Returns ptr to value associated with name, if any, else NULL.
 */
char * getenv(const char * name)
{
    size_t offset;

    return _findenv(name, &offset);
}

/*
 * _findenv(name,offset) --
 *  Returns pointer to value associated with name, if any, else NULL.
 *  Sets offset to be the offset of the name/value combination in the
 *  environmental array, for use by setenv(3) and unsetenv(3).
 *  Explicitly removes '=' in argument name.
 *
 *  This routine *should* be a static; don't use it.
 */
char * _findenv(const char * name, size_t * offset)
{
    size_t len;
    const char * e;
    char ** p;
    char * c;

    len = 0;
    for (e = name; *e && *e != '='; ++e) {
                ++len;
    }

    for (p = environ; *p; ++p) {
        if (!strncmp(*p, name, len)) {
            if (*(c = *p + len) == '=') {
                *offset = p - environ;
                return ++c;
            }
        }
    }

    return NULL;
}
