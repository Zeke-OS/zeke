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
#define DESTROY_PREFIX static dtree_node_t *
#define PATHCOMP_PREFIX static size_t
#else
#define DESTROY_PREFIX dtree_node_t *
#define PATHCOMP_PREFIX size_t
#endif
static int dtree_init_child_lists(llist_t * table[DTREE_HTABLE_SIZE]);
static void dtree_add_child(dtree_node_t * parent, dtree_node_t * node);
static void dtree_del_child(dtree_node_t * parent, dtree_node_t * node);
DESTROY_PREFIX dtree_destroy_node(dtree_node_t * node);
static size_t hash_fname(const char * fname, size_t len);
PATHCOMP_PREFIX path_compare(const char * fname, const char * path, size_t offset);
static void dtree_truncate(int change);

#ifndef PU_TEST_BUILD
#define DT_SIZE_MAX configFS_CACHE_MAX
#else
#define DT_SIZE_MAX 4096
#endif
static int dt_size = 0; /*!< Size of dtree ignoring fnames. */

/**
 * Root node.
 */
dtree_node_t dtree_root;

void dtree_init(void) __attribute__((constructor));
void dtree_init(void)
{
    dtree_root.fname = "/"; /* Special case, any other fname should not
                               contain '/'. */
    dtree_root.parent = &dtree_root; /* In POSIX "/" is parent of itself. */
    dtree_root.persist = 1;
    (void)dtree_init_child_lists(dtree_root.child); /* Assume no error. */
}

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

    dtree_truncate(sizeof(dtree_node_t) + nsize
            + DTREE_HTABLE_SIZE * sizeof(llist_t));

    /* Initialize the new node */
    memcpy(nname, fname, nsize);
    nnode->fname = nname;
    nnode->parent = parent;
    memset(&(nnode->list_node), 0, sizeof(llist_nodedsc_t));
    if(dtree_init_child_lists(nnode->child))
        goto free_nnode; /* Malloc Error! */
    nnode->persist = (persist) ? 1 : 0;

    /* Add the new node as a child of its parent. */
    dtree_add_child(parent, nnode);

    goto out; /* Ready. */

free_nnode:
    nnode = dtree_destroy_node(nnode);
out:
    return nnode;
}

static int dtree_init_child_lists(llist_t * table[DTREE_HTABLE_SIZE])
{
    size_t i;

    memset(table, 0, DTREE_HTABLE_SIZE);

    for (i = 0; i < DTREE_HTABLE_SIZE; i ++) {
        /* Create a child node lists. */
        table[i] = dllist_create(dtree_node_t, list_node);
        if (table[i] == 0)
            return -1;
    }

    return 0;
}

static void dtree_add_child(dtree_node_t * parent, dtree_node_t * node)
{
    size_t hash = hash_fname(node->fname, strlenn(node->fname, FS_FILENAME_MAX));
    llist_t * parent_list = parent->child[hash];

    /* By inserting to the head we will always found last mount point instead of
     * an older cached directory entry. */
    parent_list->insert_head(parent_list, node);
}

static void dtree_del_child(dtree_node_t * parent, dtree_node_t * node)
{
    size_t hash = hash_fname(node->fname, strlenn(node->fname, FS_FILENAME_MAX));
    llist_t * parent_list = parent->child[hash];

    parent_list->remove(parent_list, node);
}

/**
 * Discards a dtree node reference.
 * @param node is a dtree node reference.
 */
void dtree_discard_node(dtree_node_t * node)
{
    /* Decrement persist count */
    if (node->persist <= 1) {
        node->persist = 0;
    } else {
        node->persist--;
    }
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
    int retval = 0;

    if (node == 0) {
        goto out;
    }

    for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
        dtree_node_t * curr = node->child[i]->head;
        dtree_node_t * next;
        if (curr == 0)
            continue;
        do {
            next = curr->list_node.next;
            retval |= dtree_remove_node(curr, dpers);
        } while ((curr = next) != 0);
    }

    if (dpers == DTREE_NODE_PERS)
        retval = 0;
    else if (node->persist > 0)
        retval = 1;

    if (retval == 0)
        node = dtree_destroy_node(node);

out:
    return retval;
}

/**
 * Destroy dtree node.
 * @note Removes also persisted nodes.
 * @param node is the node to be destroyed.
 * @return Always returns null pointer.
 */
DESTROY_PREFIX dtree_destroy_node(dtree_node_t * node)
{
    size_t nsize = 0;
    size_t i;

    if (node == 0)
        return (dtree_node_t *)0;

    dtree_del_child(node->parent, node);

    if (node->fname != 0) {
        nsize = strlenn(node->fname, FS_FILENAME_MAX) + 1;
        kfree(node->fname);
        //node->fname = 0;
    }

    /* Drestroy child lists */
    for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
        dllist_destroy(node->child[i]);
    }

    kfree(node);

    dtree_truncate(-(sizeof(dtree_node_t) + nsize
                + DTREE_HTABLE_SIZE * sizeof(llist_t)));

    return (dtree_node_t *)0;
}

/**
 * Compare if fname is a substring of path between offset and '/'.
 * @param fname is a normal null terminated string.
 * @param path  is a full slash separated path string.
 * @param offset is the comparison offset.
 * @return Returns next offset value or 0 if strings differ.
 */
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
 * @param match      DTREE_LOOKUP_MATCH_EXACT or DTREE_LOOKUP_MATCH_ANY
 */
dtree_node_t * dtree_lookup(const char * path, int match)
{
    size_t i, j, k, prev_k;
    size_t hash;
    dtree_node_t * curr;
    dtree_node_t * retval = 0;

    if (path[0] != '/')
        goto out;

    k = 0;
    retval = &dtree_root;
    while (path[k] != '\0') {
        if (path[++k] == '\0')
            break;
        prev_k = k;

        /* Lookup from child htable */
        i = k;
        while (path[i] != '\0' && path[i] != '/') { i++; }
        if ((j = path_compare("..", path, k))) { /* Goto parent */
            retval = retval->parent;
            k = j;
            continue;
        } else if ((j = path_compare(".", path, k))) { /* Ifnore dot */
            k = j;
            continue;
        }
        hash = hash_fname(path + k, i - k);
        curr = (dtree_node_t *)(retval->child[hash]->head);
        if (curr != 0) {
            do {
                j = path_compare(curr->fname, path, k);
                if (j != 0) {
                    retval = curr;
                    k = j;
                    break;
                }
            } while ((curr = (dtree_node_t *)(curr->list_node.next)) != 0);
        }

        /* No exact match */
        if (k == 0 || k == prev_k) {
            if (match == DTREE_LOOKUP_MATCH_EXACT)
                retval = 0; /* Requested exact match. */
            break;
        }
    }

out:
    if (retval != 0)
        retval->persist++; /* refcount */
    return retval;
}

/**
 * Get full path name of dnode.
 * @param dnode is the node to be resolved.
 * @return kmalloced string or zero in case of OOM.
 */
char * dtree_getpath(dtree_node_t * dnode)
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
        /* TODO Optimize memory allocations. */
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

/**
 * Hash function.
 * @param str is the string to be hashed.
 * @param len is the length of str without '\0'.
 * @return Hash value.
 */
static size_t hash_fname(const char * str, size_t len)
{
    /* TODO larger hash space if DTREE_HTABLE_SIZE > sizeof char */
#if DTREE_HTABLE_SIZE == 16
    size_t hash = (size_t)(str[0] ^ str[len - 1]) & (size_t)(DTREE_HTABLE_SIZE - 1);
#else
#error No suitable hash function for selected DTREE_HTABLE_SIZE.
#endif

    return hash;
}

/**
 * Calculate new cache total memory usage and check if cache needs to be
 * truncated.
 * @TODO usage stats based discard...
 * @param change is the change in bytes to total cache size.
 */
static void dtree_truncate(int change)
{
    if (change >= 0) {
        dt_size += change;
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
