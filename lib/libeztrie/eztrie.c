/**
 *******************************************************************************
 * @file    eztrie.c
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

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "eztrie.h"

struct eztrie eztrie_create(void)
{
    struct eztrie_node * t;

    t = calloc(1, sizeof(struct eztrie_node) + sizeof(struct eztrie_node *));
    if (t) {
        t->k = '\0';
        t->child_count = 1;
    }

    return (struct eztrie){ .root = t };
}

static int eztrie_node_compar(const void * ap, const void * bp)
{
    const struct eztrie_node * const * a = ap;
    const struct eztrie_node * const * b = bp;
    char ak, bk;

    ak = (*a) ? (*a)->k : '\0';
    bk = (*b) ? (*b)->k : '\0';

    return (int)ak - (int)bk;
}

static struct eztrie_node ** alookup(struct eztrie_node * root, char k)
{
    struct eztrie_node ** a = root->children;
    size_t n = root->child_count;
    const size_t size = sizeof(struct eztrie_node *);

    while (n) {
        size_t cr = n % 2;
        struct eztrie_node ** pivot;
        int d;

        n /= 2;
        pivot = (struct eztrie_node **)((char *)a + (n * size));
        d = (int)k - (int)((*pivot) ? (*pivot)->k : '\0');
        if (d > 0) {
            a = (struct eztrie_node **)((char *)pivot + size);
            n -= 1 - cr;
        } else if (d == 0) {
            return pivot;
        }
    }

    return NULL;
}

static struct eztrie_node * ainsert(struct eztrie_node ** root, struct eztrie_node * node)
{
    struct eztrie_node ** a = (*root)->children;
    size_t i, n = (*root)->child_count;

    for (i = 0; i < n; i++) {
        if (!a[i])
            break;
        assert(a[i]->k != node->k);
    }

    if (i >= n) {
        struct eztrie_node * tmp;

        n++;
        tmp = realloc(*root, sizeof(struct eztrie_node) +
                      n * sizeof(struct eztrie_node *));
        if (!tmp) {
            return NULL;
        }
        tmp->child_count = n;
        a = tmp->children;
        *root = tmp;
    }

    a[i] = node;

    qsort(a, n, sizeof(struct eztrie_node *), eztrie_node_compar);

    return *root;
}

static struct eztrie_node * eztrie_find_root(struct eztrie_node * root,
                                             const char * key)
{
    size_t i, n = strlen(key);

    for (i = 0; i < n; i++) {
        struct eztrie_node ** node;

        node = alookup(root, key[i]);
        if (!node || !(*node)) {
            return NULL;
        }
        root = *node;
    }

    return root;
}

static struct eztrie_iterator eztrie_levelorder(struct eztrie_node * root,
                                                int used_only)
{
    struct eztrie_iterator q = STAILQ_HEAD_INITIALIZER(q);
    struct eztrie_iterator it = STAILQ_HEAD_INITIALIZER(it);

    if (!root) {
        return it;
    }

    STAILQ_INSERT_HEAD(&q, root, _entry);

    while (!STAILQ_EMPTY(&q)) {
        struct eztrie_node * node;
        struct eztrie_node ** child;
        size_t n;

        node = STAILQ_FIRST(&q);
        STAILQ_REMOVE_HEAD(&q, _entry);
        child = node->children;
        n = node->child_count;

        /* Visit; Add to the list of nodes found. */
        if (!used_only || node->value) {
            STAILQ_INSERT_TAIL(&it, node, _entry);
        }

        while (n--) {
            if (child[n]) {
                STAILQ_INSERT_HEAD(&q, child[n], _entry);
            }
        }
    }

    return it;
}

struct eztrie_iterator eztrie_find(struct eztrie * trie, const char * key)
{
    struct eztrie_node * t = trie->root;

    return eztrie_levelorder(eztrie_find_root(t, key), 1);
}

struct eztrie_node_value * eztrie_remove_ithead(struct eztrie_iterator * it)
{
    struct eztrie_node_value * value = NULL;

    if (!STAILQ_EMPTY(it)) {
        value = STAILQ_FIRST(it)->value;
        STAILQ_REMOVE_HEAD(it, _entry);
    }

    return value;
}

void * eztrie_insert(struct eztrie * trie, const char * key, const void * p)
{
    struct eztrie_node ** t = (struct eztrie_node **)(&trie->root);
    struct eztrie_node_value * value;
    size_t i = 0;
    const size_t n = strlen(key);

    while (i < n) {
        struct eztrie_node ** node;

        node = alookup(*t, key[i]);
        if (!(node && *node))
            break;
        t = node;
        i++;
    }

    while (i < n) {
        struct eztrie_node * node;
        void * p;

        node = calloc(1, sizeof(struct eztrie_node) +
                      sizeof(struct eztrie_node *));
        if (!node) {
            return NULL;
        }
        node->k = key[i];
        node->child_count = 1;
        p = ainsert(t, node);
        assert(p);
        t = alookup(*t, node->k);
        i++;
    }

    /* Create the value entry. */
    value = malloc(sizeof(struct eztrie_node_value) + n + 1);
    value->p = p;
    memcpy((char *)value->key, key, n + 1);
    (*t)->value = value;

    return (void *)p;
}

void * eztrie_remove(struct eztrie * trie, const char * key)
{
    struct eztrie_node * const root = trie->root;
    struct eztrie_node ** npp;
    struct eztrie_node * node = root;
    const void * p = NULL;
    size_t i, n = strlen(key);

    for (i = 0; i < n; i++) {
        npp = alookup(node, key[i]);
        if (!(npp && *npp)) {
            return NULL;
        }
        node = *npp;
    }

    if (node == root) {
        return NULL;
    }

    if (node->value) {
        p = node->value->p;
        free(node->value);
        node->value = NULL;
    }
    if (node->child_count == 0) {
        free(node);
        *npp = NULL;
    }

    return (void *)p;
}

void eztrie_destroy(struct eztrie * trie)
{
    struct eztrie_iterator it;
    struct eztrie_node * node;
    struct eztrie_node * node_tmp;

    it = eztrie_find(trie, "");
    STAILQ_FOREACH_SAFE(node, &it, _entry, node_tmp) {
        STAILQ_REMOVE(&it, node, eztrie_node, _entry);
        eztrie_remove(trie, node->value->key);
    }

    it = eztrie_levelorder(eztrie_find_root(trie->root, ""), 0);
    STAILQ_FOREACH_SAFE(node, &it, _entry, node_tmp) {
        STAILQ_REMOVE(&it, node, eztrie_node, _entry);
        free(node);
    }

    trie->root = NULL;
}
