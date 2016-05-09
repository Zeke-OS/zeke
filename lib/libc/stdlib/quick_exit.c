/**
 *******************************************************************************
 * @file    quick_exit.c
 * @author  Olli Vanhoja
 * @brief   File control.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdlib.h>
#include <sys/queue.h>
#include <unistd.h>

struct quick_exit_handler {
    void (*func)(void);
    SLIST_ENTRY(quick_exit_handler) _entries;
};

static SLIST_HEAD(slisthead, quick_exit_handler) head =
    SLIST_HEAD_INITIALIZER(head);

int at_quick_exit(void (*func)(void))
{
    struct quick_exit_handler * np;

    np = malloc(sizeof(struct quick_exit_handler));
    if (!np)
        return -1;

    SLIST_INSERT_HEAD(&head, np, _entries);

    return 0;
}

_Noreturn void quick_exit(int status)
{
    struct quick_exit_handler * np;

    SLIST_FOREACH(np, &head, _entries) {
        np->func();
    }

    _exit(status);
    __builtin_unreachable();
}
