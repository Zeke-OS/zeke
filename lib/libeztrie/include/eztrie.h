/**
 *******************************************************************************
 * @file    eztrie.h
 * @author  Olli Vanhoja
 * @brief   Eztrie.
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

#ifndef _EZTRIE_H_
#define _EZTRIE_H_

#ifdef __BSD_VISIBLE
#include <sys/queue.h>
#else
#include "eztrie_queue.h"
#endif

struct eztrie_node {
    char k;
    unsigned char child_count;
    struct eztrie_node_value * value;
    STAILQ_ENTRY(eztrie_node) _entry;
    struct eztrie_node * children[0];
};

struct eztrie {
    struct eztrie_node * root;
};

struct eztrie_node_value {
    void * p;
    char key[0];
};

STAILQ_HEAD(eztrie_iterator, eztrie_node);

/**
 * Create a new trie.
 */
struct eztrie eztrie_create(void);

/**
 * Find values from a trie matching to a given key.
 * @param trie is a pointer to the trie.
 * @param key is the key as a c-string.
 * @returns Returns a trie iterator struct.
 */
struct eztrie_iterator eztrie_find(struct eztrie * trie, char * key);

/**
 * Remove the head of an interator.
 * @param trie is a pointer to the trie.
 * @param key is the key as a c-string.
 * @returns Returns a pointer to a trie value struct.
 */
struct eztrie_node_value * eztrie_remove_ithead(struct eztrie_iterator * it);

/**
 * Insert (key, p) to a trie.
 * @param trie is a pointer to the trie.
 * @param key is the key as a c-string.
 * @param p is a pointer to the value to be inserted.
 * @returns Returns pointer p if it was sucessfully iserted to the trie;
 *          NULL if the insertion failed;
 *          A pointer to the offending value.
 */
void * eztrie_insert(struct eztrie * trie, char * key, void * p);

/**
 * Remove a (key, p) from a trie.
 * @param trie is a pointer to the trie.
 * @param key is the key as a c-string.
 * @returns Returns a pointer to the removed value.
 */
void * eztrie_remove(struct eztrie * trie, char * key);

/**
 * Destroy a trie.
 * Frees all memory allocated by a trie.
 * @param trie is a pointer to the trie.
 */
void eztrie_destroy(struct eztrie * trie);

#endif /* _EZTRIE_H_ */
