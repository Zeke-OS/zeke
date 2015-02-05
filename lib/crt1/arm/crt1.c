/* LINTLIBRARY */
/*-
 * Copyright 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright 2001 David E. O'Brien.
 * All rights reserved.
 * Copyright 1996-1998 John D. Polstra.
 * All rights reserved.
 * Copyright (c) 1997 Jason R. Thorpe.
 * Copyright (c) 1995 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          FreeBSD Project.  See http://www.freebsd.org/ for
 *          information about FreeBSD.
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef lint
#ifndef __GNUC__
#error "GCC is needed to compile this file"
#endif
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>

#include "../crtbrand.c"
#include "../ignore_init.c"

extern void _start(int argc, char ** argv, char ** envp, void (*cleanup)(void));

#ifdef GCRT
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int eprol;
extern int etext;
#endif

void __start(int argc, char ** argv, char ** envp, void (*cleanup)(void));

/* The entry function. */
__asm(" .text           \n"
"   .align  0           \n"
"   .globl  _start      \n"
"   _start:             \n"
"   /* Ensure the stack is properly aligned before calling C code. */\n"
"   bic sp, sp, #7      \n"
"   sub sp, sp, #8      \n"
"   str r5, [sp, #4]    \n"
"   str r4, [sp, #0]    \n"
"\n"
"   b    __start  ");
/* ARGSUSED */
void __start(int argc, char ** argv, char ** envp, void (*cleanup)(void))
{
    handle_argv(argc, argv, envp);

    /* if (&_DYNAMIC != NULL) */
    if (cleanup)
        atexit(cleanup);
    /*else
        _init_tls();*/
#if 0
#ifdef GCRT
    atexit(_mcleanup);
    monstartup(&eprol, &etext);
#endif
#endif
    handle_static_init(argc, argv, envp);
    fflush(stdout);
    exit(main(argc, argv, envp));
}

#ifdef GCRT
__asm__(".text");
__asm__("eprol:");
__asm__(".previous");
#endif
