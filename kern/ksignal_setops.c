/*-
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1989, 1993
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
 *
 */

#include <kstring.h>
#include <errno.h>
#include <signal.h>

int sigaddset(sigset_t * set, int signo)
{
    if (signo <= 0 || signo > _SIG_MAXSIG) {
        return -EINVAL;
    }

    set->__bits[_SIG_WORD(signo)] |= _SIG_BIT(signo);
    return 0;
}

int sigdelset(sigset_t * set, int signo)
{

    if (signo <= 0 || signo > _SIG_MAXSIG) {
        return -EINVAL;
    }

    set->__bits[_SIG_WORD(signo)] &= ~_SIG_BIT(signo);
    return 0;
}

int sigemptyset(sigset_t * set)
{
    size_t i;

    for (i = 0; i < _SIG_WORDS; i++) {
        set->__bits[i] = 0;
    }

    return 0;
}

int sigfillset(sigset_t * set)
{
    size_t i;

    for (i = 0; i < _SIG_WORDS; i++) {
        set->__bits[i] = ~0U;
    }

    return 0;
}

int sigismember(const sigset_t * set, int signo)
{
    if (signo <= 0 || signo > _SIG_MAXSIG) {
        return -EINVAL;
    }

    return (set->__bits[_SIG_WORD(signo)] & _SIG_BIT(signo)) ? 1 : 0;
}

int sigisemptyset(const sigset_t * set)
{
    size_t i;

    for (i = 0; i < _SIG_WORDS; i++) {
        if (set->__bits[i])
            return 0;
    }

    return 1;
}

int sigffs(sigset_t * set)
{
    size_t i, j;

    for (i = 0; i < _SIG_WORDS; i++) {
        for (j = 0; j < sizeof(set->__bits[0]); j++)
            if (set->__bits[i] & (1 << j))
                return i * sizeof(set->__bits[0]) * 8 + j;
    }

    return -1;
}

sigset_t * sigunion(sigset_t * target, sigset_t * a, sigset_t * b)
{
    size_t i;
    sigset_t tmp;

    for (i = 0; i < _SIG_WORDS; i++) {
        tmp.__bits[i] = a->__bits[i] | b->__bits[i];
    }

    memcpy(target, &tmp, sizeof(sigset_t));
    return target;
}

sigset_t * sigintersect(sigset_t * target, sigset_t * a, sigset_t * b)
{
    size_t i;
    sigset_t tmp;

    for (i = 0; i < _SIG_WORDS; i++) {
        tmp.__bits[i] = a->__bits[i] & b->__bits[i];
    }

    memcpy(target, &tmp, sizeof(sigset_t));
    return target;
}

sigset_t * sigcompl(sigset_t * target, sigset_t * set)
{
    size_t i;
    sigset_t tmp;

    for (i = 0; i < _SIG_WORDS; i++) {
        tmp.__bits[i] = ~(set->__bits[i]);
    }

    memcpy(target, &tmp, sizeof(sigset_t));
    return target;
}
