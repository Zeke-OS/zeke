/**
 *******************************************************************************
 * @file    dtree.h
 * @author  Olli Vanhoja
 * @brief   dtree - directory tree headers.
 *          dtree is used for directory structure lookup caching.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#define DTREE_HTABLE_SIZE   16 /*!< Node hash table size, should be 2^n. */

typedef struct dtree_node {
    char * fname; /*!< Pointer to the name of this node. */
    vfile_t file;
    struct dtree_node * parent;
    struct dtree_node * pchild[DTREE_HTABLE_SIZE]; /*< Persistent children. */
    struct dtree_node * child[DTREE_HTABLE_SIZE]; /*!< Children of this node,
                                                   *   in a hash table. */
    size_t persist; /*!< Number of persisted entries. If 0 this node can be
                     * discarded freely if necessary. Can be increment by one
                     * to make sure the node is always pesisted. (eg. root) */
} dtree_node_t;

extern dtree_node_t dtree_root;

dtree_node_t * dtree_create_node(dtree_node_t * parent, char * fname, int persist);
int dtree_remove_node(dtree_node_t * node, int dpers);

#endif /* DTREE_H */

/**
 * @}
 */
