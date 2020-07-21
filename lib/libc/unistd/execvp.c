/**
 *******************************************************************************
 * @file    execvp.c
 * @author  Olli Vanhoja
 * @brief   Execute a file.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Originally by the Regents of the University of California?
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <ctype.h>
#include <errno.h>
#include <paths.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_PDCLIB_util.h>
#include <syscall.h>
#include <unistd.h>

#define NARG_MAX 256

extern char ** __buildargv(va_list ap, const char * arg, char ***  envpp);
extern char ** environ;

static char * execat(char * s1, const char * s2, char * si)
{
    char * s = si;

    while (*s1 && *s1 != ':') {
        *s++ = *s1++;
    }
    if (si != s)
        *s++ = '/';
    while (*s2) {
        *s++ = *s2++;
    }
    *s = '\0';

    return *s1 ? ++s1 : NULL;
}

static size_t parse_shebang(char * argv[], char * fname, char * line)
{
    char * arg0 = _PDCLIB_skipwhite(line + 2);
    char * arg1;
    size_t skip;

    arg1 = arg0;
    while (arg1 < line + sizeof(line)) {
        if (*arg1 == '\0' || isspace(*arg1)) {
            *arg1 = '\0';
            break;
        }
        arg1++;
    }
    arg1++;

    if (arg1 < (line + sizeof(line)) && *arg1 != '\0') {
        argv[0] = arg0;
        argv[1] = arg1;
        argv[2] = fname;
        skip = 3;
    } else {
        argv[0] = arg0;
        argv[1] = fname;
        skip = 2;
    }

    return skip;
}

static int exec_script(char * const argv[], char * fname)
{
    FILE * fp;
    char * newargs[NARG_MAX];
    char line[MAX_INPUT];
    size_t skip;

    memset(line, '\0', sizeof(line));
    fp = fopen(fname, "r");
    if (!fgets(line, sizeof(line), fp))
        return -ENOEXEC;
    fclose(fp);

    if (line[0] == '#' && line[1] == '!' && line[2] != '\0') {
        skip = parse_shebang(newargs, fname, line) - 1;
    } else { /* Try fallback to sh */
        newargs[0] = _PATH_BSHELL;
        newargs[1] = fname;
        skip = 1;
    }

    /* Handle args */
    for (size_t i = 1; (newargs[i + skip] = argv[i]); i++) {
        if (i >= NARG_MAX - 2) {
            errno = E2BIG;
            return -1;
        }
    }

    return execve(newargs[0], newargs, environ);
}

int execvp(const char * name, char * const argv[])
{
    char * pathstr;
    char * cp;
    char fname[NAME_MAX];
    unsigned etxtbsy = 1;
    int eacces = 0;

    pathstr = getenv("PATH");
    if (!pathstr)
        pathstr = _PATH_STDPATH;
    cp = strchr(name, '/') ? "" : pathstr;

    do {
        cp = execat(cp, name, fname);
    retry:
        execve(fname, argv, environ);
        switch (errno) {
        case ENOEXEC:
            return exec_script(argv, fname);
        case ETXTBSY:
            if (++etxtbsy > 5)
                return -1;
            sleep(etxtbsy);
            goto retry;
        case EACCES:
            eacces = 1;
            break;
        case E2BIG:
        case EFAULT:
        case ENOMEM:
            return -1;
        }
    } while (cp);

    if (eacces)
        errno = EACCES;
    return -1;
}
