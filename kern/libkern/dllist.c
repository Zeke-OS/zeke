/**
 *******************************************************************************
 * @file    llist.c
 * @author  Olli Vanhoja
 * @brief   Generic doubly linked list.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kmalloc.h>
#include <stdint.h>
#include <llist.h>

#define LISTDSC(node) ((llist_nodedsc_t *)(((uint8_t *)node) + lst->offset))

llist_t * dllist_create(size_t offset);
void dllist_destroy(llist_t * lst);
void * dllist_get(llist_t * lst, int i);
void dllist_insert_head(struct llist * lst, void * new_node);
void dllist_insert_tail(struct llist * lst, void * new_node);
void dllist_insert_before(struct llist * lst, void * node, void * new_node);
void dllist_insert_after(struct llist * lst, void * node, void * new_node);
void * dllist_remove(struct llist * lst, void * node);

llist_t * _dllist_create(size_t offset)
{
    llist_t * lst;

    lst = (llist_t *)kmalloc(sizeof(llist_t));
    if (lst == 0)
        goto out;

    /* Init data */
    lst->offset = offset;
    lst->head = 0;
    lst->tail = 0;
    lst->count = 0;

    /* Set function pointers */
    lst->get = dllist_get;
    lst->insert_head = dllist_insert_head;
    lst->insert_tail = dllist_insert_tail;
    lst->insert_before = dllist_insert_before;
    lst->insert_after = dllist_insert_after;
    lst->remove = dllist_remove;

out:
    return lst;
}

void dllist_destroy(llist_t * lst)
{
    if (lst == 0)
        return;

    /*if (lst->head != 0) {
        void * node;
        void * node_next = lst->head;
        do {
            node = node_next;
            node_next = LISTDSC(node)->next;
            kfree(node);
        } while (node_next != 0);
    }*/

    kfree(lst);
}

void * dllist_get(llist_t * lst, int i)
{
    int n = 0;
    void * retval;

    retval = lst->head;
    while (retval != 0) {
        if (i == n++)
            break;

        retval = LISTDSC(retval)->next;
    }

    return retval;
}

void dllist_insert_head(struct llist * lst, void * new_node)
{
    llist_nodedsc_t * dsc = LISTDSC(new_node);

    if (lst->head == 0) {
        lst->head = new_node;
        lst->tail = new_node;
        dsc->next = 0;
        dsc->prev = 0;

        lst->count += 1;
    } else {
        dllist_insert_before(lst, lst->head, new_node);
    }
}

void dllist_insert_tail(struct llist * lst, void * new_node)
{
    if (lst->tail == 0) {
        dllist_insert_head(lst, new_node);
    } else {
        dllist_insert_after(lst, lst->tail, new_node);
    }
}

void dllist_insert_before(struct llist * lst, void * node, void * new_node)
{
    llist_nodedsc_t * old_dsc = LISTDSC(node);
    llist_nodedsc_t * new_dsc = LISTDSC(new_node);

    new_dsc->prev = old_dsc->prev;
    new_dsc->next = node;
    if (old_dsc->prev == 0)
        lst->head = new_node;
    else
        LISTDSC(old_dsc->prev)->next = new_node;
    old_dsc->prev = new_node;

    lst->count += 1;
}

void dllist_insert_after(struct llist * lst, void * node, void * new_node)
{
    llist_nodedsc_t * old_dsc = LISTDSC(node);
    llist_nodedsc_t * new_dsc = LISTDSC(new_node);

    new_dsc->prev = node;
    new_dsc->next = old_dsc->next;
    if (old_dsc->next == 0)
        lst->tail = new_node;
    else
        LISTDSC(old_dsc->next)->prev = new_node;
    old_dsc->next = new_node;

    lst->count += 1;
}

void * dllist_remove(struct llist * lst, void * node)
{
    llist_nodedsc_t * dsc;

    if (!node)
        return 0;

    dsc = LISTDSC(node);

    if (dsc->prev == 0)
        lst->head = dsc->next;
    else
        LISTDSC(dsc->prev)->next = dsc->next;
    if (dsc->next == 0)
        lst->tail = dsc->prev;
    else
        LISTDSC(dsc->next)->prev = dsc->prev;

    lst->count -= 1;

    return node;
}
