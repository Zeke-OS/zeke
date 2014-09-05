/**
 *******************************************************************************
 * @file    llist.h
 * @author  Olli Vanhoja
 * @brief   Generic linked list.
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

#pragma once
#ifndef LLIST_H
#define LLIST_H

#include <stddef.h>

typedef struct {
    struct llist * lst;
    void * next;
    void * prev;
} llist_nodedsc_t;

/**
 * Generic linked list.
 * @note All nodes should be kmalloc'd.
 */
typedef struct llist {
    size_t offset; /*!< Offset of node descriptor. */
    void * head; /*!< Head node. */
    void * tail; /*!< Tail node. */
    int count; /*!< Node count. */
    /* Functions */
    void * (*get)(struct llist *, int i);
    void (*insert_head)(struct llist *, void * new_node);
    void (*insert_tail)(struct llist *, void * new_node);
    void (*insert_before)(struct llist *, void * node, void * new_node);
    void (*insert_after)(struct llist *, void * node, void * new_node);
    void * (*remove)(struct llist *, void * node);
} llist_t;

#endif /* LLIST_H */
