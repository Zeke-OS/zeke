/**
 *******************************************************************************
 * @file    proc_session.c
 * @author  Olli Vanhoja
 * @brief   Kernel process session management.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define PROC_INTERNAL
#include <proc.h>
#include <kmalloc.h>

struct session * proc_session_create(struct proc_info * leader,
                                     char s_login[MAXLOGNAME])
{
    struct session * s;

    s = kzalloc(sizeof(struct session));
    if (!s)
        return NULL;

    TAILQ_INIT(&s->s_pgrp_list_head);
    s->s_refcount = ATOMIC_INIT(0);
    strlcpy(s->s_login, s_login, sizeof(s->s_login));

    return s;
}

static void proc_session_ref(struct session * s)
{
    atomic_inc(&s->s_refcount);
}

static void proc_session_rele(struct session * s)
{
    int rc;

    rc = atomic_dec(&s->s_refcount);
    if (rc <= 1)
        kfree(s);
}

struct pgrp * proc_pgrp_create(struct session * s, struct proc_info * proc)
{
    struct pgrp * pgrp;

    pgrp = kzalloc(sizeof(struct pgrp));
    if (!pgrp)
        return NULL;

    TAILQ_INIT(&pgrp->pg_proc_list_head);
    pgrp->pg_id = proc->pid;
    pgrp->pg_refcount = ATOMIC_INIT(0);

    proc_session_ref(s);
    pgrp->pg_session = s;
    TAILQ_INSERT_TAIL(&s->s_pgrp_list_head, pgrp, pg_pgrp_entry_);
    proc_pgrp_insert(pgrp, proc);

    return pgrp;
}

static void proc_pgrp_ref(struct pgrp * pgrp)
{
    atomic_inc(&pgrp->pg_refcount);
}

static void proc_pgrp_rele(struct pgrp * pgrp)
{
    int rc;

    rc = atomic_dec(&pgrp->pg_refcount);
    if (rc <= 1) {
        struct session * s = pgrp->pg_session;

        TAILQ_REMOVE(&s->s_pgrp_list_head, pgrp, pg_pgrp_entry_);
        proc_session_rele(s);
        kfree(pgrp);
    }
}

void proc_pgrp_insert(struct pgrp * pgrp, struct proc_info * proc)
{
    proc_pgrp_ref(pgrp);
    TAILQ_INSERT_TAIL(&pgrp->pg_proc_list_head, proc, pgrp_proc_entry_);
    proc->pgrp = pgrp;
}

void proc_pgrp_remove(struct proc_info * proc)
{
    struct pgrp * pgrp = proc->pgrp;

    TAILQ_REMOVE(&pgrp->pg_proc_list_head, proc, pgrp_proc_entry_);
    proc_pgrp_rele(pgrp);
}
