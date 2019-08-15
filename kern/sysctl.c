/**
 *******************************************************************************
 * @file    sysctl.c
 * @author  Olli Vanhoja
 *
 * @brief   sysctl kernel code.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1982, 1986, 1989, 1993
 *        The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Quite extensively rewritten by Poul-Henning Kamp of the FreeBSD
 * project, to make these variables more userfriendly.
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
 *        @(#)kern_sysctl.c        8.4 (Berkeley) 4/14/94
 */

#include <errno.h>
#include <stdint.h>
#include <kerror.h>
#include <kinit.h>
#include <klocks.h>
#include <kmalloc.h>
#include <kstring.h>
#include <proc.h>
#include <sys/priv.h>
#include <sys/queue.h>
#include <syscall.h>
#include <vm/vm.h>
#include <sys/sysctl.h>


struct sysctl_oid_list sysctl__children; /* root list */

/*
 * Register the kernel's oids on startup.
 */
SET_DECLARE(sysctl_set, struct sysctl_oid);

SYSCTL_DECL(_sysctl);
SYSCTL_NODE(, 0, sysctl, CTLFLAG_RW, 0,
        "Sysctl internal magic");

/**
 * The sysctllock protects the MIB tree. It also protects sysctl contexts used
 * with dynamic sysctls. The sysctl_register_oid() and sysctl_unregister_oid()
 * routines require the sysctllock to already be held, so the sysctl_lock() and
 * sysctl_unlock() routines are provided for the few places in the kernel which
 * need to use that API rather than using the dynamic API. Use of the dynamic
 * API is strongly encouraged for most code.
 */
static mtx_t sysctllock = MTX_INITIALIZER(MTX_TYPE_SPIN, 0);

#define SYSCTL_LOCK()       mtx_lock(&sysctllock)
#define SYSCTL_UNLOCK()     mtx_unlock(&sysctllock)
#define SYSCTL_ASSERT_XLOCKED() \
    KASSERT(mtx_test(&sysctllock), "sysctllock is required")


static struct sysctl_oid * sysctl_find_oidname(const char * name,
        struct sysctl_oid_list * list);
static int sysctl_sysctl_name(SYSCTL_HANDLER_ARGS);
static int sysctl_sysctl_next_ls(struct sysctl_oid_list * lsp, int * name,
        unsigned int namelen, int * next, int * len, int level,
        struct sysctl_oid ** oidpp);
static int sysctl_sysctl_next(SYSCTL_HANDLER_ARGS);
static int name2oid(char * name, int * oid, int * len,
        struct sysctl_oid ** oidpp);
static int sysctl_sysctl_name2oid(SYSCTL_HANDLER_ARGS);
static int sysctl_sysctl_oidfmt(SYSCTL_HANDLER_ARGS);
static int sysctl_old_kernel(struct sysctl_req * req, const void * p, size_t l);
static int sysctl_new_kernel(struct sysctl_req * req, void * p, size_t l);
static int sysctl_old_user(struct sysctl_req * req, const void * p, size_t l);
static int sysctl_new_user(struct sysctl_req * req, void * p, size_t l);
static int sysctl_root(SYSCTL_HANDLER_ARGS);
static int userland_sysctl(const struct proc_info * proc, int * name,
                    unsigned int namelen, __user void * old,
                    __user size_t * oldlenp, int inkernel, __user void * new,
                    size_t newlen, size_t * retval, int flags);


int __kinit__ sysctl_init(void)
{
    struct sysctl_oid ** oidp;

    SUBSYS_INIT("sysctl");

    SYSCTL_LOCK();
    SET_FOREACH(oidp, sysctl_set)
            sysctl_register_oid(*oidp);
    SYSCTL_UNLOCK();

    return 0;
}

void sysctl_register_oid(struct sysctl_oid * oidp)
{
    struct sysctl_oid_list * parent = oidp->oid_parent;
    struct sysctl_oid * p;
    struct sysctl_oid * q;

    /*
     * First check if another oid with the same name already
     * exists in the parent's list.
     */
    SYSCTL_ASSERT_XLOCKED();
    p = sysctl_find_oidname(oidp->oid_name, parent);
    if (p != NULL) {
        if ((p->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
            p->oid_refcnt++;
            return;
        } else {
            KERROR(KERROR_WARN,
                   "can't re-use a leaf (%s)!\n", p->oid_name);
            return;
        }
    }
    /*
     * If this oid has a number OID_AUTO, give it a number which
     * is greater than any current oid.
     * NOTE: DO NOT change the starting value here, change it in
     * <sys/sysctl.h>, and make sure it is at least 256 to
     * accomodate e.g. net.inet.raw as a static sysctl node.
     */
    if (oidp->oid_number == OID_AUTO) {
        static int newoid = CTL_AUTO_START;

        oidp->oid_number = newoid++;
        if (newoid == 0x7fffffff) {
            panic("out of oids");
        }
    }
#if 0
    else if (oidp->oid_number >= CTL_AUTO_START) {
        /* do not panic; this happens when unregistering sysctl sets */
        KERROR(KERROR_WARN, "static sysctl oid too high: %d", oidp->oid_number);
    }
#endif

    /*
     * Insert the oid into the parent's list in order.
     */
    q = NULL;
    SLIST_FOREACH(p, parent, oid_link) {
        if (oidp->oid_number < p->oid_number)
            break;
        q = p;
    }
    if (q)
        SLIST_INSERT_AFTER(q, oidp, oid_link);
    else
        SLIST_INSERT_HEAD(parent, oidp, oid_link);
}

void sysctl_unregister_oid(struct sysctl_oid * oidp)
{
    struct sysctl_oid * p;

    SYSCTL_ASSERT_XLOCKED();
    if (oidp->oid_number == OID_AUTO) {
        /*
         * This can happen when a module fails to register and is
         * being unloaded afterwards.  It should not be a panic()
         * for normal use.
         */
        KERROR(KERROR_WARN, "%s: failed to unregister sysctl\n", __func__);
        return;
    }

    SLIST_FOREACH(p, oidp->oid_parent, oid_link) {
        if (p == oidp) {
            SLIST_REMOVE(oidp->oid_parent, oidp,
                         sysctl_oid, oid_link);
            break;
        }
    }
}

static int sysctl_remove_oid_locked(struct sysctl_oid * oidp,
                                    int del, int recurse)
{
    struct sysctl_oid * p;
    struct sysctl_oid * tmp;
    int error;

    SYSCTL_ASSERT_XLOCKED();
    if (oidp == NULL)
        return -EINVAL;
    if ((oidp->oid_kind & CTLFLAG_DYN) == 0) {
        KERROR(KERROR_ERR, "Can't remove non-dynamic nodes!\n");
        return -EINVAL;
    }

    /*
     * WARNING: normal method to do this should be through
     * sysctl_ctx_free(). Use recursing as the last resort
     * method to purge your sysctl tree of leftovers...
     * However, if some other code still references these nodes,
     * it will panic.
     */
    if ((oidp->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
        if (oidp->oid_refcnt == 1) {
            SLIST_FOREACH_SAFE(p, SYSCTL_CHILDREN(oidp), oid_link, tmp) {
                if (!recurse) {
                    KERROR(KERROR_WARN,
                           "Failed attempt to remove oid %s with child %s\n",
                           oidp->oid_name, p->oid_name);
                    return -ENOTEMPTY;
                }
                error = sysctl_remove_oid_locked(p, del, recurse);
                if (error)
                    return error;
            }
        }
    }
    if (oidp->oid_refcnt > 1) {
        oidp->oid_refcnt--;
    } else {
        if (oidp->oid_refcnt == 0) {
            KERROR(KERROR_WARN, "Bad oid_refcnt=%u (%s)!\n",
                   oidp->oid_refcnt, oidp->oid_name);
            return -EINVAL;
        }
        sysctl_unregister_oid(oidp);
        if (del) {
            /*
             * Wait for all threads running the handler to drain.
             * This preserves the previous behavior when the
             * sysctl lock was held across a handler invocation,
             * and is necessary for module unload correctness.
             */
            while (atomic_read(&oidp->oid_running) > 0) {
                oidp->oid_kind |= CTLFLAG_DYING;
                /* FIXME Sleep until oid_running wakeup */
            }
            if (oidp->oid_descr)
                kfree(__DECONST(char *, oidp->oid_descr));
            kfree(__DECONST(char *, oidp->oid_name));
            kfree(oidp);
        }
    }
    return 0;
}

int sysctl_remove_oid(struct sysctl_oid * oidp, int del, int recurse)
{
    int error;

    SYSCTL_LOCK();
    error = sysctl_remove_oid_locked(oidp, del, recurse);
    SYSCTL_UNLOCK();
    return error;
}

struct sysctl_oid * sysctl_add_oid(struct sysctl_oid_list * parent,
                                   const char * name, int kind,
                                   void * arg1, intmax_t arg2,
                                   int (*handler)(SYSCTL_HANDLER_ARGS),
                                   const char * fmt, const char * descr)
{
    struct sysctl_oid * oidp;

    /* You have to hook up somewhere.. */
    if (parent == NULL)
        return NULL;

    /* Check if the node already exists, otherwise create it */
    SYSCTL_LOCK();
    oidp = sysctl_find_oidname(name, parent);
    if (oidp != NULL) {
        if ((oidp->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
            oidp->oid_refcnt++;
            SYSCTL_UNLOCK();
            return oidp;
        } else {
            SYSCTL_UNLOCK();
            KERROR(KERROR_ERR, "Can't re-use a leaf (%s)!\n", name);
            return NULL;
        }
    }
    oidp = kmalloc(sizeof(struct sysctl_oid));
    *oidp = (struct sysctl_oid){
        .oid_parent = parent,
        .oid_link = { NULL },
        .oid_number = OID_AUTO,
        .oid_kind = CTLFLAG_DYN | kind,
        .oid_fmt = fmt,
        .oid_name = kstrdup(name, CTL_MAXSTRNAME),
        .oid_handler = handler,
        .oid_arg1 = arg1,
        .oid_arg2 = arg2,
        .oid_refcnt = 1,
        .oid_running = ATOMIC_INIT(0),
    };
    if (descr != NULL)
        oidp->oid_descr = kstrdup(descr, CTL_MAXSTRNAME);
    /* Register this oid */
    sysctl_register_oid(oidp);
    SYSCTL_UNLOCK();

    return oidp;
}

int sysctl_rename_oid(struct sysctl_oid * oidp, const char * name)
{
    char * newname;
    char * oldname;

    if ((oidp->oid_kind & CTLFLAG_DYN) == 0)
        return -EROFS;

    newname = kstrdup(name, CTL_MAXSTRNAME);
    SYSCTL_LOCK();
    oldname = __DECONST(char *, oidp->oid_name);
    oidp->oid_name = newname;
    SYSCTL_UNLOCK();
    kfree(oldname);

    return 0;
}

int sysctl_move_oid(struct sysctl_oid * oid, struct sysctl_oid_list * parent)
{
    struct sysctl_oid * oidp;

    if ((oid->oid_kind & CTLFLAG_DYN) == 0)
        return -EROFS;

    SYSCTL_LOCK();
    if (oid->oid_parent == parent) {
        SYSCTL_UNLOCK();
        return 0;
    }

    oidp = sysctl_find_oidname(oid->oid_name, parent);
    if (oidp != NULL) {
        SYSCTL_UNLOCK();
        return -EEXIST;
    }

    sysctl_unregister_oid(oid);
    oid->oid_parent = parent;
    oid->oid_number = OID_AUTO;
    sysctl_register_oid(oid);
    SYSCTL_UNLOCK();
    return 0;
}

int sysctl_find_oid(int * name, unsigned int namelen, struct sysctl_oid ** noid,
                    int * nindx, struct sysctl_req * req)
{
    struct sysctl_oid_list * lsp;
    struct sysctl_oid * oid;
    int indx;

    SYSCTL_ASSERT_XLOCKED();
    lsp = &sysctl__children;
    indx = 0;
    while (indx < CTL_MAXNAME) {
        SLIST_FOREACH(oid, lsp, oid_link) {
            if (oid->oid_number == name[indx])
                break;
        }
        if (oid == NULL)
            return -ENOENT;

        indx++;
        if ((oid->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
            if (oid->oid_handler || indx == (int)namelen) {
                *noid = oid;
                if (nindx != NULL)
                    *nindx = indx;
#if 0
                KASSERT((oid->oid_kind & CTLFLAG_DYING) == 0,
                        ("%s found DYING node %p", __func__, oid));
#endif
                return 0;
            }
            lsp = SYSCTL_CHILDREN(oid);
        } else if (indx == (int)namelen) {
            *noid = oid;
            if (nindx != NULL)
                *nindx = indx;
#if 0
            KASSERT((oid->oid_kind & CTLFLAG_DYING) == 0,
                    ("%s found DYING node %p", __func__, oid));
#endif
            return 0;
        } else {
            return -ENOTDIR;
        }
    }
    return -ENOENT;
}

static struct sysctl_oid * sysctl_find_oidname(const char * name,
                                               struct sysctl_oid_list * list)
{
    struct sysctl_oid * oidp;

    SYSCTL_ASSERT_XLOCKED();
    SLIST_FOREACH(oidp, list, oid_link) {
        if (strcmp(oidp->oid_name, name) == 0) {
            return oidp;
        }
    }
    return NULL;
}


static int sysctl_sysctl_name(SYSCTL_HANDLER_ARGS)
{
    int * name = (int *)arg1;
    unsigned int namelen = arg2;
    int error = 0;
    struct sysctl_oid *oid;
    struct sysctl_oid_list *lsp = &sysctl__children, *lsp2;
    char buf[10];

    SYSCTL_LOCK();
    while (namelen) {
        if (!lsp) {
            ksprintf(buf, sizeof(buf), "%d", *name);
            if (req->oldidx)
                error = req->oldfunc(req, ".", 1);
            if (!error)
                error = req->oldfunc(req, buf, strlenn(buf, sizeof(buf)));
            if (error)
                goto out;
            namelen--;
            name++;
            continue;
        }
        lsp2 = 0;
        SLIST_FOREACH(oid, lsp, oid_link) {
            if (oid->oid_number != *name)
                continue;

            if (req->oldidx)
                error = req->oldfunc(req, ".", 1);
            if (!error)
                error = req->oldfunc(req, oid->oid_name,
                    strlenn(oid->oid_name, CTL_MAXSTRNAME));
            if (error)
                goto out;

            namelen--;
            name++;

            if ((oid->oid_kind & CTLTYPE) != CTLTYPE_NODE)
                break;

            if (oid->oid_handler)
                break;

            lsp2 = SYSCTL_CHILDREN(oid);
            break;
        }
        lsp = lsp2;
    }
    error = req->oldfunc(req, "", 1);
 out:
    SYSCTL_UNLOCK();
    return error;
}

static SYSCTL_NODE(_sysctl, _CTLMAGIC_NAME, name, CTLFLAG_RD,
                   sysctl_sysctl_name, "");

static int sysctl_sysctl_next_ls(struct sysctl_oid_list * lsp, int * name,
                                 unsigned int namelen, int * next, int * len,
                                 int level, struct sysctl_oid ** oidpp)
{
    struct sysctl_oid * oidp;

    SYSCTL_ASSERT_XLOCKED();
    *len = level;
    SLIST_FOREACH(oidp, lsp, oid_link) {
        *next = oidp->oid_number;
        *oidpp = oidp;

        if (oidp->oid_kind & CTLFLAG_SKIP)
            continue;

        if (!namelen) {
            if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE)
                return 0;
            if (oidp->oid_handler)
                /* We really should call the handler here...*/
                return 0;
            lsp = SYSCTL_CHILDREN(oidp);
            if (!sysctl_sysctl_next_ls(lsp, 0, 0, next + 1, len,
                                       level + 1, oidpp))
                return 0;
            goto emptynode;
        }

        if (oidp->oid_number < *name)
            continue;

        if (oidp->oid_number > *name) {
            if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE)
                return 0;
            if (oidp->oid_handler)
                return 0;
            lsp = SYSCTL_CHILDREN(oidp);
            if (!sysctl_sysctl_next_ls(lsp, name + 1, namelen - 1,
                                       next + 1, len, level + 1, oidpp))
                return 0;
            goto next;
        }
        if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE)
            continue;

        if (oidp->oid_handler)
            continue;

        lsp = SYSCTL_CHILDREN(oidp);
        if (!sysctl_sysctl_next_ls(lsp, name + 1, namelen - 1, next + 1,
                                   len, level + 1, oidpp))
            return 0;
    next:
        namelen = 1;
    emptynode:
        *len = level;
    }
    return 1;
}

static int sysctl_sysctl_next(SYSCTL_HANDLER_ARGS)
{
    int * name = (int *)arg1;
    unsigned int namelen = arg2;
    int i, j, error;
    struct sysctl_oid * oid;
    struct sysctl_oid_list *lsp = &sysctl__children;
    int newoid[CTL_MAXNAME];

    SYSCTL_LOCK();
    i = sysctl_sysctl_next_ls(lsp, name, namelen, newoid, &j, 1, &oid);
    SYSCTL_UNLOCK();
    if (i)
        return -ENOENT;
    error = req->oldfunc(req, newoid, j * sizeof(int));
    return error;
}

static SYSCTL_NODE(_sysctl, _CTLMAGIC_NEXT, next, CTLFLAG_RD,
                   sysctl_sysctl_next, "");

static int name2oid(char * name, int * oid, int * len,
                    struct sysctl_oid ** oidpp)
{
    struct sysctl_oid * oidp;
    struct sysctl_oid_list * lsp = &sysctl__children;

    SYSCTL_ASSERT_XLOCKED();

    for (*len = 0; *len < CTL_MAXNAME;) {
        char * p = strsep(&name, ".");

        oidp = SLIST_FIRST(lsp);
        for (;; oidp = SLIST_NEXT(oidp, oid_link)) {
            if (oidp == NULL)
                return -ENOENT;
            if (strcmp(p, oidp->oid_name) == 0)
                break;
        }
        *oid++ = oidp->oid_number;
        (*len)++;

        if (name == NULL || *name == '\0') {
            if (oidpp)
                *oidpp = oidp;
            return 0;
        }

        if ((oidp->oid_kind & CTLTYPE) != CTLTYPE_NODE)
            break;

        if (oidp->oid_handler)
            break;

        lsp = SYSCTL_CHILDREN(oidp);
    }
    return -ENOENT;
}

static int sysctl_sysctl_name2oid(SYSCTL_HANDLER_ARGS)
{
    char * p;
    int error, oid[CTL_MAXNAME], len = 0;
    struct sysctl_oid * op = 0;

    if (!req->newlen)
        return -ENOENT;
    if (req->newlen >= CTL_MAXSTRNAME)
        return -ENAMETOOLONG;

    p = kmalloc(req->newlen + 1);

    error = req->newfunc(req, p, req->newlen);
    if (error) {
        kfree(p);
        return error;
    }

    p[req->newlen] = '\0';

    SYSCTL_LOCK();
    error = name2oid(p, oid, &len, &op);
    SYSCTL_UNLOCK();

    kfree(p);

    if (error)
        return error;

    error = req->oldfunc(req, oid, len * sizeof(*oid));
    return error;
}

SYSCTL_PROC(_sysctl, _CTLMAGIC_NAME2OID, name2oid,
            CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY,
            0, 0, sysctl_sysctl_name2oid, "I", "");

static int sysctl_sysctl_oidfmt(SYSCTL_HANDLER_ARGS)
{
    struct sysctl_oid * oid;
    int error;

    SYSCTL_LOCK();
    error = sysctl_find_oid(arg1, arg2, &oid, NULL, req);
    if (error)
        goto out;

    if (oid->oid_fmt == NULL) {
        error = -ENOENT;
        goto out;
    }
    error = req->oldfunc(req, &oid->oid_kind, sizeof(oid->oid_kind));
    if (error)
        goto out;
    error = req->oldfunc(req, oid->oid_fmt,
                         strlenn(oid->oid_fmt, CTL_MAXSTRNAME) + 1);
 out:
    SYSCTL_UNLOCK();
    return error;
}

static SYSCTL_NODE(_sysctl, _CTLMAGIC_OIDFMT, oidfmt,
                   CTLFLAG_RD,
                   sysctl_sysctl_oidfmt, "");

static int sysctl_sysctl_oiddescr(SYSCTL_HANDLER_ARGS)
{
    struct sysctl_oid *oid;
    int error;

    SYSCTL_LOCK();
    error = sysctl_find_oid(arg1, arg2, &oid, NULL, req);
    if (error)
        goto out;

    if (oid->oid_descr == NULL) {
        error = -ENOENT;
        goto out;
    }
    error = req->oldfunc(req, oid->oid_descr,
                         strlenn(oid->oid_descr, CTL_MAXSTRNAME) + 1);
 out:
    SYSCTL_UNLOCK();
    return error;
}

static SYSCTL_NODE(_sysctl, _CTLMAGIC_OIDDESCR, oiddescr,
                   CTLFLAG_RD,
                   sysctl_sysctl_oiddescr, "");

/**
 * Boolean handler.
 * Two cases:
 * + a variable:  point arg1 at it.
 * + a constant:  pass it in arg2.
 */
int sysctl_handle_bool(SYSCTL_HANDLER_ARGS)
{
    int tmpout, error = 0;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = !!*(int *)arg1;
    else
        tmpout = !!arg2;
    error = req->oldfunc(req, &tmpout, sizeof(int));

    if (error || !req->newptr)
        goto out;

    if (!arg1) {
        error = -EPERM;
    } else {
        int new_val;

        error = req->newfunc(req, &new_val, sizeof(int));
        if (!error) {
            *(int *)arg1 = !!new_val;
        }
    }

out:
    return error;
}

/**
 * Integer handler.
 * Handle an int, signed or unsigned.
 * Two cases:
 * + a variable:  point arg1 at it.
 * + a constant:  pass it in arg2.
 */
int sysctl_handle_int(SYSCTL_HANDLER_ARGS)
{
    int tmpout, error = 0;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = *(int *)arg1;
    else
        tmpout = arg2;
    error = req->oldfunc(req, &tmpout, sizeof(int));

    if (error || !req->newptr)
        goto out;

    if (!arg1) {
        error = -EPERM;
    } else {
        error = req->newfunc(req, arg1, sizeof(int));
    }

out:
    return error;
}

/*
 * Long handler.
 * Handle a long, signed or unsigned.
 * Two cases:
 *     a variable:  point arg1 at it.
 *     a constant:  pass it in arg2.
 */
int sysctl_handle_long(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    long tmplong;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmplong = *(long *)arg1;
    else
        tmplong = arg2;

    error = req->oldfunc(req, &tmplong, sizeof(long));

    if (error || !req->newptr)
        goto out;

    if (!arg1) {
        error = -EPERM;
    } else {
        error = req->newfunc(req, arg1, sizeof(long));
    }

out:
    return error;
}

/**
 * uint32_t handler.
 * Handle a 32 bit int, signed or unsigned.
 * Two cases:
 *     a variable:  point arg1 at it.
 *     a constant:  pass it in arg2.
 */
int sysctl_handle_32(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    uint32_t tmpout;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = *(uint32_t *)arg1;
    else
        tmpout = arg2;
    error = req->oldfunc(req, &tmpout, sizeof(uint32_t));

    if (error || !req->newptr)
        goto out;

    if (!arg1) {
        error = -EPERM;
    } else {
        error = req->newfunc(req, arg1, sizeof(uint32_t));
    }

out:
    return error;
}

/**
 * uint64_t handler.
 * Handle a 64 bit int, signed or unsigned.
 * Two cases:
 *     a variable:  point arg1 at it.
 *     a constant:  pass it in arg2.
 */
int sysctl_handle_64(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    uint64_t tmpout;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = *(uint64_t *)arg1;
    else
        tmpout = arg2;
    error = req->oldfunc(req, &tmpout, sizeof(uint64_t));

    if (error || !req->newptr)
        goto out;

    if (!arg1) {
        error = -EPERM;
    } else {
        error = req->newfunc(req, arg1, sizeof(uint64_t));
    }

out:
    return error;
}

/*
 * Handle our generic '\0' terminated 'C' string.
 * Two cases:
 *         a variable string:  point arg1 at it, arg2 is max length.
 *         a constant string:  point arg1 at it, arg2 is zero.
 */

int sysctl_handle_string(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    char * tmparg;
    size_t outlen;

    /*
     * Attempt to get a coherent snapshot by copying to a
     * temporary kernel buffer.
     */
retry:
    outlen = strlenn((char *)arg1, CTLT_STRING_MAX) + 1;
    tmparg = kmalloc(outlen);

    if (strlcpy(tmparg, (char *)arg1, outlen) >= outlen) {
        kfree(tmparg);
        goto retry;
    }

    error = req->oldfunc(req, tmparg, outlen);
    kfree(tmparg);

    if (error || !req->newptr)
        goto out;

    if ((req->newlen - req->newidx) >= (uintptr_t)arg2) {
        error = -EINVAL;
    } else {
        arg2 = (req->newlen - req->newidx);
        error = req->newfunc(req, arg1, arg2);
        ((char *)arg1)[arg2] = '\0';
    }

out:
    return error;
}

int sysctl_handle_opaque(SYSCTL_HANDLER_ARGS)
{
    int error;

    error = req->oldfunc(req, arg1, arg2);
    if (error)
        return error;

    error = req->newfunc(req, arg1, arg2);

    return error;
}

/**
 * Transfer functions to/from kernel space.
 */
static int sysctl_old_kernel(struct sysctl_req * req, const void * p, size_t l)
{
    size_t i = 0;
    int retval = 0;

    if (req->oldptr) {
        i = l;
        if (req->oldlen <= req->oldidx)
            i = 0;
        else if (i > req->oldlen - req->oldidx)
            i = req->oldlen - req->oldidx;
        if (i > 0)
            memmove((char *)req->oldptr + req->oldidx, p, i);
    }
    req->oldidx += l;
    if (req->oldptr && i != l)
        retval = -ENOMEM;

    return retval;
}

static int sysctl_new_kernel(struct sysctl_req * req, void * p, size_t l)
{
    int retval = 0;

    if (!req->newptr)
        goto out;

    if (req->newlen - req->newidx < l) {
        retval = -EINVAL;
        goto out;
    }
    memmove(p, (char *)req->newptr + req->newidx, l);
    req->newidx += l;

out:
    return retval;
}

int kernel_sysctl_read(int * name, unsigned int namelen,
                       void * old, size_t oldlen)
{
    size_t oldlen_ret = oldlen;
    int retval;

    retval = kernel_sysctl(NULL, name, namelen, old, &oldlen_ret, NULL, 0,
                           NULL, 0);
    if (oldlen_ret != oldlen)
        return -EINVAL;
    return retval;
}

int kernel_sysctl_write(int * name, unsigned int namelen,
                        const void * new, size_t newlen)
{
    return kernel_sysctl(NULL, name, namelen, NULL, NULL, (void *)new, newlen,
                         NULL, 0);
}

int kernel_sysctl(struct cred * cred, int * name, unsigned int namelen,
                  void * old, size_t * oldlenp, void * new, size_t newlen,
                  size_t * retval, int flags)
{
    int error = 0;
    struct sysctl_req req;

    if (!cred) {
        struct proc_info * proc;

        proc = proc_ref(0);
        cred = &proc->cred;
        proc_unref(proc);
    }

    memset(&req, 0, sizeof(req));

    req.cred = cred;
    req.flags = flags | SYSCTL_REQFLAG_KERNEL;

    if (oldlenp) {
        req.oldlen = *oldlenp;
    }
    req.validlen = req.oldlen;
    req.oldptr = old;

    if (new) {
        req.newlen = newlen;
        req.newptr = new;
    }

    req.oldfunc = sysctl_old_kernel;
    req.newfunc = sysctl_new_kernel;

    SYSCTL_LOCK();
    error = sysctl_root(0, name, namelen, &req);
    SYSCTL_UNLOCK();

    if (error && error != -ENOMEM)
        return error;

    if (retval) {
        /*
         * If more than expected was copied then we only report the valid size.
         */
        if (req.oldptr && req.oldidx > req.validlen)
            *retval = req.validlen;
        else
            *retval = req.oldidx;
    }

    return error;
}

int kernel_sysctlbyname(struct cred * cred, char * name, void * old,
                        size_t * oldlenp, void * new, size_t newlen,
                        size_t * retval, int flags)
{
    int oid[CTL_MAXNAME];
    size_t oidlen, plen;
    int error;

    oid[0] = 0; /* sysctl internal magic */
    oid[1] = _CTLMAGIC_NAME2OID;
    oidlen = sizeof(oid);

    error = kernel_sysctl(cred, oid, 2, oid, &oidlen,
            (void *)name, strlenn(name, CTL_MAXSTRNAME), &plen, flags);
    if (error)
        return error;

    error = kernel_sysctl(cred, oid, plen / sizeof(int), old, oldlenp,
            new, newlen, retval, flags);

    return error;
}

/*
 * Transfer function to/from user space.
 */
static int sysctl_old_user(struct sysctl_req * req, const void * p, size_t l)
{
    size_t i, len, origidx;

    origidx = req->oldidx;
    req->oldidx += l;
    if (req->oldptr == NULL)
        return 0;

    i = l;
    len = req->validlen;
    if (len <= origidx) {
        i = 0;
    } else {
        int error;

        if (i > len - origidx)
            i = len - origidx;
        error = copyout(p, (__user char *)req->oldptr + origidx, i);
        if (error != 0)
            return error;
    }
    if (i < l)
        return -ENOMEM;
    return 0;
}

static int sysctl_new_user(struct sysctl_req * req, void * p, size_t l)
{
    int error;

    if (!req->newptr)
        return 0;
    if (req->newlen - req->newidx < l)
        return -EINVAL;

    error = copyin((__user char *)req->newptr + req->newidx, p, l);
    req->newidx += l;

    return error;
}

/*
 * Traverse our tree, and find the right node, execute whatever it points
 * to, and return the resulting error code.
 */

static int sysctl_root(SYSCTL_HANDLER_ARGS)
{
    struct sysctl_oid * oid;
    int error, indx;

    SYSCTL_ASSERT_XLOCKED();

    error = sysctl_find_oid(arg1, arg2, &oid, &indx, req);
    if (error)
        return error;

    if ((oid->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
        /*
         * You can't call a sysctl when it's a node, but has
         * no handler.  Inform the user that it's a node.
         * The indx may or may not be the same as namelen.
         */
        if (!oid->oid_handler)
            return -EISDIR;
    }

    /* Is this sysctl writable? */
    if (req->newptr &&
        !(oid->oid_kind & CTLFLAG_WR ||
          (req->flags & SYSCTL_REQFLAG_KERNEL &&
           oid->oid_kind & CTLFLAG_KERWR))) {
        return -EPERM;
    }

    /* Is this sysctl sensitive to securelevels? */
    if (req->newptr && (oid->oid_kind & CTLFLAG_SECURE)) {
        int lvl = (oid->oid_kind & CTLMASK_SECURE) >> CTLSHIFT_SECURE;
        error = securelevel_gt(lvl);
        if (error)
            return error;
    }

    /* Is this sysctl writable by only privileged users? */
    if (req->newptr && !(req->flags & SYSCTL_REQFLAG_KERNEL) &&
        !(oid->oid_kind & CTLFLAG_ANYBODY)) {
        error = priv_check(req->cred, PRIV_SYSCTL_WRITE);
        if (error)
            return error;
    }

    if (!oid->oid_handler)
        return -EINVAL;

    if ((oid->oid_kind & CTLTYPE) == CTLTYPE_NODE) {
        arg1 = (int *)arg1 + indx;
        arg2 -= indx;
    } else {
        arg1 = oid->oid_arg1;
        arg2 = oid->oid_arg2;
    }

    /* TODO atomic */
    atomic_inc(&oid->oid_running);
    SYSCTL_UNLOCK();

    error = oid->oid_handler(oid, arg1, arg2, req);

    SYSCTL_LOCK();
    atomic_dec(&oid->oid_running);
    /* TODO */
#if 0
    if (oid->oid_running == 0 && (oid->oid_kind & CTLFLAG_DYING) != 0)
        wakeup(&oid->oid_running);
#endif
    return error;
}

static int userland_sysctl(const struct proc_info * proc, int * name,
                           unsigned int namelen, __user void * old,
                           __user size_t * oldlenp, int inkernel,
                           __user void * new, size_t newlen,
                           size_t * retval, int flags)
{
    struct sysctl_req req;
    int error = 0;

    memset(&req, 0, sizeof(req));

    req.cred = &proc->cred;
    req.flags = flags;

    if (oldlenp) {
        if (inkernel) {
            req.oldlen = *oldlenp;
        } else {
            error = copyin(oldlenp, &req.oldlen, sizeof(*oldlenp));
            if (error)
                return error;
        }
    }
    req.validlen = req.oldlen;

    if (old) {
        if (!useracc(old, req.oldlen, VM_PROT_WRITE))
            return -EFAULT;
        req.oldptr = (void *)old;
    }

    if (new != NULL) {
        if (!useracc(new, newlen, VM_PROT_READ))
            return -EFAULT;
        req.newlen = newlen;
        req.newptr = (void *)new;
    }

    req.oldfunc = sysctl_old_user;
    req.newfunc = sysctl_new_user;

    for (;;) {
        req.oldidx = 0;
        req.newidx = 0;
        SYSCTL_LOCK();
        error = sysctl_root(0, name, namelen, &req);
        SYSCTL_UNLOCK();
        if (error != -EAGAIN)
            break;
        thread_yield(THREAD_YIELD_IMMEDIATE);
    }

    if (error && error != -ENOMEM)
        return error;

    if (retval) {
        /*
         * If more than expected was copied then we only report the valid size.
         */
        if (req.oldptr && req.oldidx > req.validlen)
            *retval = req.validlen;
        else
            *retval = req.oldidx;
    }

    return error;
}

intptr_t sysctl_syscall(uint32_t type, __user void * p)
{
    int err, name[CTL_MAXNAME];
    size_t j;
    struct _sysctl_args uap;

    if (type != SYSCALL_SYSCTL_SYSCTL) {
        set_errno(ENOSYS);
        return -1;
    }

    err = copyin(p, &uap, sizeof(uap));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (uap.namelen > CTL_MAXNAME || uap.namelen < 2) {
        set_errno(EINVAL);
        return -1;
    }

    err = copyin((__user void *)uap.name, &name, uap.namelen * sizeof(int));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    err = userland_sysctl(curproc, name, uap.namelen,
                          (__user void *)uap.old, (__user size_t *)uap.oldlenp,
                          0, (__user void *)uap.new, uap.newlen, &j, 0);
    if (err && err != -ENOMEM) {
        set_errno(-err);
        return -1;
    }
    if (uap.oldlenp) {
        err = copyout(&j, (__user size_t *)uap.oldlenp, sizeof(j));
        if (err) {
            set_errno(-err);
            return -1;
        }
    }

    return 0;
}
