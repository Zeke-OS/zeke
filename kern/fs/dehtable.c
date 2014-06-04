/**
 *******************************************************************************
 * @file    dehtable.c
 * @author  Olli Vanhoja
 * @brief   Directory Entry Hashtable.
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

#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <fs/fs.h>
#include "dehtable.h"

/* Chains
 * ------
 *
 * Dirent hash table uses chaining to solve collisions. Chains are dh_dirent
 * arrays that are dynamically resized when needed.
 *
 * To keep a chain unbroken there shall be no empty slots between two entries.
 * If one entry is removed the chain must be restructured so that empty slot is
 * filled. There is no need to free the space left after restructuring a chain
 * because it will either completely reused on next link or of partly freed if
 * more than one entries have been removed before next insert.
 *
 * dh_link tag must be used to mark the end of chain.
 */

/**
 * Chain info struct.
 */
typedef struct chain_info {
    size_t i_last; /*!< Index of the last node of the chain. */
    size_t i_size; /*!< Size of the chain. */
} chain_info_t;

#define DIRENT_SIZE (sizeof(dh_dirent_t) - sizeof(char))

#define CH_NO_LINK  0
#define CH_LINK     1

static chain_info_t find_last_node(dh_dirent_t * chain);
static dh_dirent_t * find_node(dh_dirent_t * chain, const char * name,
        size_t name_len);
static size_t hash_fname(const char * str, size_t len);

/**
 * Get reference to a directory entry in array.
 * @param dea is a directory entry array.
 * @param offset is the offset of dh_dirent in the array.
 * @return Pointer to a dh_dirent struct.
 */
#define get_dirent(dea, offset) ((dh_dirent_t *)((size_t)(dea) + (offset)))

/**
 * Returns true if given dirent node has an invalid dh_size.
 * Invalid dh_size would lead to invalid offset and possibly segmentation fault.
 * @param denode is a directory entry node.
 * @return 0 if dh_size is valid; Otherwise value other than zero.
 */
#define if_invalid_offset(denode) ((denode)->dh_size > \
        DIRENT_SIZE + FS_FILENAME_MAX)

/**
 * Insert a new directory entry link.
 * @param dir       is a directory entry table.
 * @param vnode     is the vnode where the new hard link will point.
 * @param name      is the name of the hard link.
 * @param name_len  is the length of the name without null character.
 * @return Returns 0 if succeed; Otherwise value other than zero.
 */
int dh_link(dh_table_t * dir, vnode_t * vnode, const char * name, size_t name_len)
{
    const size_t h = hash_fname(name, name_len);
    const size_t entry_size = DIRENT_SIZE + name_len + 1;
    dh_dirent_t * dea;
    chain_info_t chinfo;
    int retval = 0;

    /* TODO Should we double check that link with a same name doesn't exist? */
    dea = (*dir)[h];
    if (dea == 0) { /* Chain array not yet created. */
        dea = kmalloc(entry_size);

        chinfo.i_last = 0;
        chinfo.i_size = 0; /* Set size to 0 to fool the assignment code as
                            * this is the first entry. */
    } else { /* Chain array found. */
        /* Find the last node of the chain and realloc a longer chain. */
        chinfo = find_last_node(dea);
        dea = krealloc(dea, chinfo.i_size + entry_size);
    }
    if (dea == 0) {
        /* OOM, can't add new entries. */
        retval = -1;
        goto out;
    }
    (*dir)[h] = dea;

    /* Create a link */
    get_dirent(dea, chinfo.i_last)->dh_link = CH_LINK;
    /* New node */
    get_dirent(dea, chinfo.i_size)->dh_ino = vnode->vnode_num;
    get_dirent(dea, chinfo.i_size)->dh_size = entry_size;
    get_dirent(dea, chinfo.i_size)->dh_link = CH_NO_LINK;
    strncpy(get_dirent(dea, chinfo.i_size)->dh_name, name, name_len + 1);

out:
    return retval;
}

/* TODO unlink */

/**
 * Destroy all dir entries.
 * @param dir is a directory entry hash table.
 */
void dh_destroy_all(dh_table_t * dir)
{
    size_t i;

    /* Free all dir entry chains. */
    for (i = 0; i < DEHTABLE_SIZE; i++) {
        if ((*dir)[i])
            kfree((*dir)[i]);
    }
}

/**
 * Lookup for hard link in dh_table of dir.
 * @param dir       is the dh_table of a directory.
 * @param name      is the link name searched for.
 * @param name_len  is the length of name.
 * @param vnode_num is the vnode number the link is pointing to.
 * @return Returns zero if link with specified name was found;
 *         Otherwise value other than zero indicating type of error.
 */
int dh_lookup(dh_table_t * dir, const char * name, size_t name_len,
        ino_t * vnode_num)
{
    const size_t h = hash_fname(name, name_len);
    dh_dirent_t * dea;
    dh_dirent_t * dent;
    int retval = 0;

    dea = (*dir)[h];
    if (dea == 0) {
        retval = -1;
        goto out;
    }

    dent = find_node(dea, name, name_len);
    if (dent == 0) {
        retval = -2;
        goto out;
    }

    *vnode_num = dent->dh_ino;
out:
    return retval;
}

/**
 * Get dirent hashtable iterator.
 * @param is a directory entry hash table.
 * @return Returns a dent hash table iterator struct.
 */
dh_dir_iter_t dh_get_iter(dh_table_t * dir)
{
    dh_dir_iter_t it = {
        .dir = dir,
        .dea_ind = 0,
        .ch_ind = SIZE_MAX
    };

    return it;
}

/**
 * Get next directory entry from iterator it.
 * @param it is a dirent hash table iterator.
 * @return Next directory entry in hash table.
 */
dh_dirent_t * dh_iter_next(dh_dir_iter_t * it)
{
    dh_dirent_t * dea; /* Dir entry array (chain). */
    dh_dirent_t * node = 0; /* Dir entry node in chain. */

    /* Already iterated over all nodes? */
    if (it->dea_ind >= DEHTABLE_SIZE) {
        goto out;
    }

    if (it->ch_ind == SIZE_MAX) { /* Get a next chain array */
        size_t i = it->dea_ind;

        /* Already iterated over all nodes? */
        //if (it->dea_ind >= DEHTABLE_SIZE) {
        //    goto out;
        //}

        it->ch_ind = 0; /* Reset chain index */

        do {
            dea = (*(it->dir))[i]; /* Get a dir entry array */
            i++;
        } while (dea == 0 && i < DEHTABLE_SIZE);
        it->dea_ind = i - 1;
    } else { /* Still iterating the old chain. */
        dea = (*(it->dir))[it->dea_ind];
        if (dea == 0) {
            goto out; /* Empty hash table or No more entries. */
        }
    }
    if (dea == 0) {
        goto out; /* Empty hash table or No more entries. */
    }


    node = get_dirent(dea, it->ch_ind); /* Get a node from the chain. */
    if (if_invalid_offset(node)) {
        node = 0;
        goto out; /* Broken dirent hash table. */
    }
    if (node->dh_link == CH_LINK) {
        it->ch_ind += node->dh_size;
    } else {
        it->dea_ind++;
        it->ch_ind = SIZE_MAX; /* It's quite safe to assume the array is
                                * shorter than SIZE_MAX. */
    }

out:
    return node;
}

/**
 * Find the last node of the chain.
 * @param chain is the dirent chain array.
 * @return Returns the updated chain_info struct.
 */
static chain_info_t find_last_node(dh_dirent_t * chain)
{
    dh_dirent_t * node;
    chain_info_t chinfo;

    chinfo.i_size = 0;
    do {
        node = get_dirent(chain, chinfo.i_size);
        if (if_invalid_offset(node)) {
            /* Error */
            KERROR(KERROR_ERR, "Invalid offset in deh node");
            break;
        }
        chinfo.i_size += node->dh_size;
    } while (node->dh_link == CH_LINK);
    chinfo.i_last = chinfo.i_size - node->dh_size;

    return chinfo;
}

/**
 * Find a specific dirent node in chain.
 * @param chain     is the chain array.
 * @param name      is the name of the node searched for.
 * @param name_len  is the length of the name without null character.
 * @return Returns a pointer to the node; Or null if node not found.
 */
static dh_dirent_t * find_node(dh_dirent_t * chain, const char * name,
        size_t name_len)
{
    size_t offset;
    dh_dirent_t * node;
    dh_dirent_t * retval = 0;

    offset = 0;
    do {
        node = get_dirent(chain, offset);
        if (if_invalid_offset(node)) {
            /* Error */
            KERROR(KERROR_ERR, "Invalid offset in deh node");
            break;
        }

        if(strncmp(node->dh_name, name, name_len + 1) == 0) {
            retval = node;
            break;
        }
        offset += node->dh_size;
    } while (node->dh_link == CH_LINK);

    return retval;
}

/**
 * Hash function.
 * @param str is the string to be hashed.
 * @param len is the length of str without '\0'.
 * @return Hash value.
 */
static size_t hash_fname(const char * str, size_t len)
{
#if DEHTABLE_SIZE == 16
    size_t hash = (size_t)(str[0] ^ str[len - 1]) & (size_t)(DEHTABLE_SIZE - 1);
#else
#error No suitable hash function for selected DEHTABLE_SIZE.
#endif

    return hash;
}
