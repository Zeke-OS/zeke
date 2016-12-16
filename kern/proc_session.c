/**
 *******************************************************************************
 * @file    proc_session.c
 * @author  Olli Vanhoja
 * @brief   Kernel process session management.
 * @section LICENSE
 * Copyright (c) 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>

struct proc_session_list proc_session_list_head =
    TAILQ_HEAD_INITIALIZER(proc_session_list_head);
int nr_sessions;

/**
 * Free a session struct.
 * This function is called when the last reference to a session is freed.
 */

/**
 * Create a new session.
 */
static struct session * proc_session_create(struct proc_info * leader)
{
    struct session * s;

    s = kzalloc(sizeof(struct session));
    if (!s)
        return NULL;

    TAILQ_INIT(&s->s_pgrp_list_head);
    s->s_leader = leader->pid;
    s->s_ctty_fd = -1;

    /* We expect proclock to protect us here. */
    TAILQ_INSERT_TAIL(&proc_session_list_head, s, s_session_list_entry_);
    nr_sessions++;

    return s;
}

static void proc_session_free(struct session * s)
{
    /* We expect proclock to protect us here. */
    TAILQ_REMOVE(&proc_session_list_head, s, s_session_list_entry_);
    nr_sessions--;

    kfree(s);
}

struct pgrp * proc_session_search_pg(struct session * s, pid_t pg_id)
{
    struct pgrp * pg;

    PROC_KASSERT_LOCK();

    TAILQ_FOREACH(pg, &s->s_pgrp_list_head, pg_pgrp_entry_) {
        if (pg->pg_id == pg_id)
            return pg;
    }

    return NULL;
}

void proc_session_setlogin(struct session * s, char s_login[MAXLOGNAME])
{
    strlcpy(s->s_login, s_login, sizeof(s->s_login));
}

struct pgrp * proc_pgrp_create(struct session * s, struct proc_info * proc)
{
    struct pgrp * pgrp;

    PROC_KASSERT_LOCK();

    if (!s) {
        s = proc_session_create(proc);
        if (!s)
            return NULL;
    }

    pgrp = kzalloc(sizeof(struct pgrp));
    if (!pgrp) {
        return NULL;
    }
    s->s_pgrp_count++;

    TAILQ_INIT(&pgrp->pg_proc_list_head);
    pgrp->pg_id = proc->pid;

    pgrp->pg_session = s;
    TAILQ_INSERT_TAIL(&s->s_pgrp_list_head, pgrp, pg_pgrp_entry_);
    proc_pgrp_insert(pgrp, proc);

    return pgrp;
}

static void proc_pgrp_free(struct pgrp * pgrp)
{
    struct session * s = pgrp->pg_session;

    TAILQ_REMOVE(&s->s_pgrp_list_head, pgrp, pg_pgrp_entry_);
    if (--s->s_pgrp_count == 0) {
        proc_session_free(s);
    }

    kfree(pgrp);
}

void proc_pgrp_insert(struct pgrp * pgrp, struct proc_info * proc)
{
    PROC_KASSERT_LOCK();

    if (proc->pgrp)
        proc_pgrp_remove(proc);

    pgrp->pg_proc_count++;
    TAILQ_INSERT_TAIL(&pgrp->pg_proc_list_head, proc, pgrp_proc_entry_);
    proc->pgrp = pgrp;
}

void proc_pgrp_remove(struct proc_info * proc)
{
    struct pgrp * pgrp = proc->pgrp;

    PROC_KASSERT_LOCK();

    TAILQ_REMOVE(&pgrp->pg_proc_list_head, proc, pgrp_proc_entry_);
    if (--pgrp->pg_proc_count == 0) {
        proc_pgrp_free(pgrp);
    }
}
