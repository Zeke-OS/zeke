/*-
 * Copyright 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright 2012 Konstantin Belousov <kib@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "notes.h"

extern int main(int, char **, char **);

extern void (*__preinit_array_start[])(int, char **, char **) __hidden;
extern void (*__preinit_array_end[])(int, char **, char **) __hidden;
extern void (*__init_array_start[])(int, char **, char **) __hidden;
extern void (*__init_array_end[])(int, char **, char **) __hidden;
extern void (*__fini_array_start[])(void) __hidden;
extern void (*__fini_array_end[])(void) __hidden;
extern void _fini(void) __hidden;
extern void _init(void) __hidden;

#if 0
extern int _DYNAMIC;
#pragma weak _DYNAMIC
#endif

char **environ;
const char *__progname = "";

static void
finalizer(void)
{
    size_t array_size;

    array_size = __fini_array_end - __fini_array_start;
    for (size_t n = array_size; n > 0; n--) {
        void (*fn)(void);

        fn = __fini_array_start[n - 1];
        if ((uintptr_t)fn != 0 && (uintptr_t)fn != 1)
            (fn)();
    }
}

static inline void
handle_static_init(int argc, char ** argv, char ** env)
{
    size_t array_size;

#if 0
    if (&_DYNAMIC != NULL)
        return;
#endif

    atexit(finalizer);

    array_size = __preinit_array_end - __preinit_array_start;
    for (size_t n = 0; n < array_size; n++) {
        void (*fn)(int, char **, char **);

        fn = __preinit_array_start[n];
        if ((uintptr_t)fn != 0 && (uintptr_t)fn != 1)
            fn(argc, argv, env);
    }

    array_size = __init_array_end - __init_array_start;
    for (size_t n = 0; n < array_size; n++) {
        void (*fn)(int, char **, char **);

        fn = __init_array_start[n];
        if ((uintptr_t)fn != 0 && (uintptr_t)fn != 1)
            fn(argc, argv, env);
    }
}

static inline void
handle_argv(int argc, char * argv[], char ** env)
{
    if (environ == NULL)
        environ = env;

    if (argc > 0 && argv[0] != NULL) {
        const char * s;

        __progname = argv[0];

        /* Discard path from progname */
        for (s = __progname; *s != '\0'; s++) {
            if (*s == '/')
                __progname = s + 1;
        }
    }
}
