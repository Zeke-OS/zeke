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

#ifndef PU_TEST_BUILD
#include <kmalloc.h>
#else
#include <stdlib.h>
#define kmalloc malloc
#define kfree free
#define krealloc realloc
#define strnncat(dst, dsz, src, ssz) strcat(dst, src)
#define ksprintf(dest, max, fmt, ...) snprintf(dest, max, fmt, __VA_ARGS__)
#endif
#include <kstring.h>
#include "dtree.h"

#ifndef PU_TEST_BUILD
#define DESTROY_PREFIX static void
#define PATHCOMP_PREFIX static size_t
#else
#define DESTROY_PREFIX void
#define PATHCOMP_PREFIX size_t
#endif
DESTROY_PREFIX dtree_destroy_node(dtree_node_t * node);
static size_t hash_fname(const char * fname, size_t len);
PATHCOMP_PREFIX path_compare(const char * fname, const char * path, size_t offset);
static void cond_truncate(int change);

#ifndef PU_TEST_BUILD
#define DT_SIZE_MAX configFS_CACHE_MAX
#else
#define DT_SIZE_MAX 4096
#endif
static int dt_size = 0; /*!< Size of dtree ignoring fnames. */

dtree_node_t dtree_root = {
    .fname = "/", /* Special case, any other fname should not contain '/'. */
    .parent = &dtree_root, /* In POSIX "/" is parent of itself. */
    .pchild[0] = &dtree_root,
    .persist = 1
};

/**
 * Create a new dtree node.
 * @param parent is the parent node of the new node.
 * @param fname is the name of the new node. (will be copied)
 * @param persist != 0 to make this node persistent.
 * @return Returns a pointer to the new node; 0 if failed.
 */
dtree_node_t * dtree_create_node(dtree_node_t * parent, char * fname, int persist)
{
    dtree_node_t * nnode = 0;
    char * nname;
    size_t nsize;

    if (parent == 0) {
        goto out;
    }
    if (parent->fname == 0) {
        goto out;
    }

    nnode = kmalloc(sizeof(dtree_node_t));
    if (nnode == 0)
        goto out;

    nsize = strlenn(fname, FS_FILENAME_MAX) + 1;
    nname = kmalloc(nsize);
    if (nname == 0)
        goto free_nnode;

    cond_truncate(sizeof(dtree_node_t) + nsize);

    /* Initialize the new node */
    memcpy(nname, fname, nsize);
    nnode->fname = nname;
    nnode->parent = parent;
    memset(nnode->pchild, 0, DTREE_HTABLE_SIZE);
    memset(nnode->child, 0, DTREE_HTABLE_SIZE);

    /* Add as a child of parent */
    if (persist) {
        size_t i;
        /* TODO relallocatable persist table */
        for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
            if (parent->pchild[i] == 0) {
                parent->pchild[i] = nnode;
                break;
            }
        }
        parent->persist++;
    } else {
        size_t hash = hash_fname(fname, nsize - 1);
        if (parent->child[hash] != 0) {
            dtree_remove_node(parent->child[hash], 1);
        }
        parent->child[hash] = nnode;
    }

    goto out; /* Ready */

free_nnode:
    kfree(nnode);
    nnode = 0;
out:
    return nnode;
}

/**
 * Remove a dtree node and its children.
 * @param node is the node to be removed.
 * @param dpers if != 0 removes also persisted nodes.
 * @return 0 if succeed; Value other than 0 if failed.
 */
int dtree_remove_node(dtree_node_t * node, int dpers)
{
    size_t i;
    dtree_node_t * parent;
    int retval = 0;

    if (node == 0) {
        goto out;
    }

    for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
        if (dpers) (void)dtree_remove_node(node->child[i], 1);
        retval = dtree_remove_node(node->child[i], dpers);
    }

    if (node->persist > 0 || retval != 0) {
        retval = 2;
        goto out;
    }

    parent = node->parent;
    for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
        if (parent->pchild[i] == node) {
            if (dpers) {
                retval = 0;
                parent->pchild[i] = 0;
            } else retval = 1;
        }
        if (parent->child[i] == node) {
            parent->child[i] = 0;
        }
    }

    if (retval == 0)
        dtree_destroy_node(node);

out:
    return retval;
}

/**
 * Destroy dtree node.
 * @note Removes also persisted nodes.
 */
DESTROY_PREFIX dtree_destroy_node(dtree_node_t * node)
{
    size_t nsize = 0;

    if (node == 0)
        return;

    if (node->fname != 0) {
        nsize = strlenn(node->fname, FS_FILENAME_MAX) + 1;
        kfree(node->fname);
    }
    kfree(node);

    cond_truncate(-(sizeof(dtree_node_t) + nsize));
}

PATHCOMP_PREFIX path_compare(const char * fname, const char * path, size_t offset)
{
    size_t i = 0;

    while (path[offset] != '/' && path[offset] != '\0' && fname[i] != '\0'
            && (path[offset] == fname[i])) {
        offset++;
        i++;
    }
    if ((path[offset] == '/' || path[offset] == '\0') && fname[i] == '\0')
        return offset;
    return 0;
}

/**
 * Lookup for cached directory entry.
 * @param path       Lookup for this path.
 * @param match      DTREE_LOOKUP_MATCH or DTREE_LOOKUP_ANY
 */
dtree_node_t * dtree_lookup(const char * path, int match)
{
    size_t i, k, prev_k;
    size_t hash;
    dtree_node_t * retval = 0;

    if (path[0] != '/')
        goto out;

    k = 0;
    retval = &dtree_root;
    while (path[k] != '\0') {
        if (path[++k] == '\0')
            break;
        prev_k = k;

        /* First lookup from child htable */
        i = k;
        while (path[i] != '\0' && path[i] != '/') { i++; }
        hash = hash_fname(path + k, i - k);
        if (retval->child[hash] != 0) {
            size_t j;
            j = path_compare(retval->child[hash]->fname, path, k);
            if (j != 0) {
                retval = retval->child[hash];
                k = j;
                continue;
            }
        }

        /* if no hit, then lookup from pchild array */
        for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
            if (retval->pchild[i] != 0) {
                k = path_compare(retval->pchild[i]->fname, path, k);
                if (k != 0) {
                    retval = retval->pchild[i];
                    break;
                }
            }
        }

        /* No exact match */
        if (k == 0 || k == prev_k) {
            if (match == DTREE_LOOKUP_MATCH)
                retval = 0; /* Requested exact match. */
            break;
        }
    }

out:
    return retval;
}

/**
 * Get full path name of dnode.
 * @param dnode is the node to be resolved.
 * @return kmalloced string or zero in case of OOM.
 */
char * dtree_getpath(const dtree_node_t * dnode)
{
    char * path = 0;
    char * tmp_path = 0;
    static char tmp_fname[FS_FILENAME_MAX];
    dtree_node_t * node = dnode;
    size_t len = 0;
    size_t i;

    if (dnode == 0)
        goto out;

    do {
        len += strlenn(node->fname, FS_FILENAME_MAX) + 1;
        tmp_path = krealloc(tmp_path, len + 2);
        if (tmp_path == 0) {
            goto out;
        }

        (void)ksprintf(tmp_fname, FS_FILENAME_MAX, "%s/", node->fname);
        strnncat(tmp_path, FS_FILENAME_MAX, tmp_fname, FS_FILENAME_MAX);

        node = node->parent;
    } while (strcmp(node->fname, "/"));

    tmp_path[len] = '\0';
    path = kmalloc(len + 1);
    if (path == 0) {
        kfree(tmp_path);
        goto out;
    }

    path[0] = '/';
    if (len == 2) { /* If path is root */
        path[1] = '\0';
        goto fr_tmp_path;
    }
    i = len - 1;
    len = 1;
    do {
        size_t k;
        size_t n = 0;

        do {
            n++;
        } while (tmp_path[--i] != '/' && i != 0);
        k = (tmp_path[i] == '/') ? i + 1 : i;
        strncpy(path + len, tmp_path + k, n);
        len += n;
    } while (i != 0);

fr_tmp_path:
    kfree(tmp_path);
out:
    return path;
}

static size_t hash_fname(const char * fname, size_t len)
{
    /* TODO larger hash space if DTREE_HTABLE_SIZE > sizeof char */
    size_t hash = (size_t)(fname[0] ^ fname[len - 1]) & (size_t)(DTREE_HTABLE_SIZE - 1);

    return hash;
}

static void cond_truncate(int change)
{
    if (change > 0) {
        dt_size += change;
    }
    if (change >= 0) {
#ifndef PU_TEST_BUILD
        if (dt_size > DT_SIZE_MAX) {
            dtree_remove_node(&dtree_root, 0);
        }
#endif
    } else if (change < 0) {
        dt_size -= change;
    }
}

/**
  * @}
  */
