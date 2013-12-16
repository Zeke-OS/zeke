/**
 *******************************************************************************
 * @file    dtree.h
 * @author  Olli Vanhoja
 * @brief   dtree - directory tree headers.
 *          dtree is used for directory structure lookup caching.
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

/** @addtogroup fs
  * @{
  */

#pragma once
#ifndef DTREE_H
#define DTREE_H

#include <fs/fs.h>
#include <generic/dllist.h>

#define DTREE_HTABLE_SIZE   16 /*!< Node hash table size, should be 2^n. */

/* Node persist params */
#define DTREE_NODE_NORM     0
#define DTREE_NODE_PERS     1

/* Lookup match params */
#define DTREE_LOOKUP_MATCH_ANY      0
#define DTREE_LOOKUP_MATCH_EXACT    1

typedef struct dtree_node {
    char * fname; /*!< Pointer to the name of this node. */
    vnode_t vnode;
    struct dtree_node * parent;
    llist_t * child[DTREE_HTABLE_SIZE]; /*!< Children of this node,
                                         *   in a hash table. */
    size_t persist; /*!< If this is set to a value greater than zero this node
                     *   is persisted. This value is used as a refcount and if
                     *   value is zero the node can be removed as it's
                     *   considered as unused. */
    llist_nodedsc_t list_node;
} dtree_node_t;

extern dtree_node_t dtree_root;

dtree_node_t * dtree_create_node(dtree_node_t * parent, const char * fname, int persist);
void dtree_discard_node(dtree_node_t * node);
int dtree_remove_node(dtree_node_t * node, int dpers);
dtree_node_t * dtree_lookup(const char * path, int match);
char * dtree_getpath(dtree_node_t * dnode);

#endif /* DTREE_H */

/**
 * @}
 */
