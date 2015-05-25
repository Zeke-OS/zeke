/*-
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <paths.h>
#include <stdarg.h>

extern char ** environ;

static char ** buildargv(va_list ap, const char * arg, char ***  envpp)
{
    static int memsize;
    static char ** argv;
    register int off;

    argv = NULL;
    for (off = 0;; ++off) {
        if (off >= memsize) {
            memsize += 50;  /* Starts out at 0. */
            memsize *= 2;   /* Ramp up fast. */
            if (!(argv = realloc(argv, memsize * sizeof(char *)))) {
                memsize = 0;
                free(argv);
                return NULL;
            }
            if (off == 0) {
                argv[0] = (char *)arg;
                off = 1;
            }
        }
        if (!(argv[off] = va_arg(ap, char *)))
            break;
    }
    /* Get environment pointer if user supposed to provide one. */
    if (envpp)
        *envpp = va_arg(ap, char **);
    return argv;
}

int execl(const char *name, const char *arg, ...)
{
    va_list ap;
    int sverrno;
    char ** argv;

    va_start(ap, arg);
    if ((argv = buildargv(ap, arg, NULL)))
        (void)execve(name, argv, environ);
    va_end(ap);
    sverrno = errno;
    free(argv);
    errno = sverrno;

    return -1;
}

int execle(const char * name, const char * arg, ...)
{
    va_list ap;
    int sverrno;
    char ** argv;
    char ** envp;

    va_start(ap, arg);
    if ((argv = buildargv(ap, arg, &envp)))
        (void)execve(name, argv, envp);
    va_end(ap);
    sverrno = errno;
    free(argv);
    errno = sverrno;

    return -1;
}

int execlp(const char * name, const char * arg, ...)
{
    va_list ap;
    int sverrno;
    char ** argv;

    va_start(ap, arg);
    if ((argv = buildargv(ap, arg, NULL)))
        (void)execvp(name, argv);
    va_end(ap);
    sverrno = errno;
    free(argv);
    errno = sverrno;

    return -1;
}
