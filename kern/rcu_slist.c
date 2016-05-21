/**
 *******************************************************************************
 * @file    rcu_slist.c
 * @author  Olli Vanhoja
 * @brief   A singly-linked list implementation for Read-Copy-Update.
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

#include <stddef.h>
#include <rcu.h>

/* RFE Should all reads use rcu_dereference() */


void rcu_slist_insert_head(struct rcu_slist_head * head, struct rcu_cb * elem)
{
    rcu_assign_pointer(elem->next, rcu_dereference(head->head));
    rcu_assign_pointer(head->head, elem);
}

void rcu_slist_insert_after(struct rcu_cb * elem1, struct rcu_cb * elem2)
{
    rcu_assign_pointer(elem2->next, rcu_dereference(elem1->next));
    rcu_assign_pointer(elem1->next, elem2);
}

void rcu_slist_insert_tail(struct rcu_slist_head * head, struct rcu_cb * elem)
{
    struct rcu_cb * next = head->head;
    struct rcu_cb * last;

    elem->next = NULL;

    if (!next) {
        head->head = elem;
        return;
    }

    do {
        last = next;
    } while ((next = last->next));
    rcu_assign_pointer(last->next, elem);
}

struct rcu_cb * rcu_slist_remove_head(struct rcu_slist_head * head)
{
    struct rcu_cb * old_head = head->head;

    if (old_head) {
        rcu_assign_pointer(head->head, rcu_dereference(old_head->next));
        rcu_assign_pointer(old_head->next, NULL);
    }

    return old_head;
}

struct rcu_cb * rcu_slist_remove(struct rcu_slist_head * head,
                                 struct rcu_cb * elem)
{
    struct rcu_cb * next = head->head;
    struct rcu_cb * prev;

    if (!next)
        return NULL;

    do {
        prev = next;
        next = prev->next;
        if (!next)
            return NULL;
    } while (next != elem);
    rcu_assign_pointer(prev->next, elem->next);

    rcu_assign_pointer(elem->next, NULL);
    return elem;
}

struct rcu_cb * rcu_slist_remove_tail(struct rcu_slist_head * head)
{
    struct rcu_cb * prev = NULL;
    struct rcu_cb * next = head->head;
    struct rcu_cb * last = NULL;

    if (!next)
        return NULL;

    do {
        prev = last;
        last = next;
    } while ((next = last->next));

    if (prev) {
        rcu_assign_pointer(prev->next, NULL);
    }

    return last;
}
