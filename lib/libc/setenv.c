/*
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>

extern char * _findenv(const char * name, size_t * offset);

/*
 * setenv(name, value, rewrite)
 *  Set the value of the environmental variable "name" to be
 *  "value".  If rewrite is set, replace any current value.
 */
int setenv(const char * name, const char * value, int rewrite)
{
    char * c;
    const char * e;
    size_t l_value, offset;

    if (*value == '=')      /* no `=' in value */
        ++value;

    l_value = strlen(value);
    if ((c = _findenv(name, &offset))) { /* find if already exists */
        if (!rewrite)
            return 0;
        if (strlen(c) >= l_value) { /* old larger; copy over */
            while ((*c++ = *value++));
            return 0;
        }
    } else {                  /* create new slot */
        static int alloced;     /* if allocated space before */
        size_t cnt;
        char ** p;

        for (p = environ, cnt = 0; *p; ++p, ++cnt);
        if (alloced) {          /* just increase size */
            environ = realloc((char *)environ, (sizeof(char *) * (cnt + 2)));
            if (!environ) {
                errno = ENOMEM;
                return -1;
            }
        } else {              /* get new space */
            alloced = 1;        /* copy old entries into it */
            p = malloc((sizeof(char *) * (cnt + 2)));
            if (!p) {
                errno = ENOMEM;
                return -1;
            }
            memmove(p, environ, cnt * sizeof(char *));
            environ = p;
        }
        environ[cnt + 1] = NULL;
        offset = cnt;
    }

    for (e = name; *e && *e != '='; ++e);   /* no `=' in name */
    if (!(environ[offset] =         /* name + `=' + value */
          malloc(((e - name) + l_value + 2)))) {
        errno = ENOMEM;
        return -1;
    }

    for (c = environ[offset]; (*c = *name++) && *c != '='; ++c);
    for (*c++ = '='; (*c++ = *value++); );

    return 0;
}

/*
 * unsetenv(name) --
 *  Delete environmental variable "name".
 */
int unsetenv(const char * name)
{
    char ** p;
    size_t offset;

    if (!name || strstr(name, "=")) {
        errno = EINVAL;
        return -1;
    }

    while (_findenv(name, &offset)) {     /* if set multiple times */
        for (p = &environ[offset] ;; ++p) {
            if (!(*p = *(p + 1)))
                break;
        }
    }

    return 0;
}
