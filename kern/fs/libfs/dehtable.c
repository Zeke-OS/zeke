/**
 *******************************************************************************
 * @file    dehtable.c
 * @author  Olli Vanhoja
 * @brief   Directory Entry Hashtable.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <libkern.h>
#include <errno.h>
#include <fs/fs.h>
#include <fs/dehtable.h>

/*
 * Chains
 * ------
 *
 * Dirent hash table uses chaining to solve collisions. Chains are dh_dirent
 * arrays that are dynamically resized when necessary.
 *
 * To keep a chain unbroken there shall be no empty slots between two entries.
 * If an entry is removed in the middle the chain must be restructured so that
 * empty slot is filled. There is no need to free the space left after
 * restructuring a chain because it will either completely reused on next link
 * or of partly freed if more than one entries have been removed before the next
 * insert.
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

#define CH_NOLINK   0
#define CH_LINK     1

/**
 * Get reference to a directory entry in array.
 * @param dea is a directory entry array.
 * @param offset is the offset of dh_dirent in the array.
 * @return Pointer to a dh_dirent struct.
 */
#define get_dirent(dea, offset) \
    ((dh_dirent_t *)((size_t)(dea) + (offset)))

/**
 * Returns true if given dirent node has an invalid dh_size.
 * Invalid dh_size would lead to invalid offset and possibly segmentation fault.
 * @param denode is a directory entry node.
 * @return 0 if dh_size is valid; Otherwise value other than zero.
 */
#define is_invalid_offset(denode) \
    ((denode)->dh_size > (DIRENT_SIZE + NAME_MAX + 1))

/**
 * Hash function.
 * @note Only upto 32 bits.
 * @param str is the string to be hashed.
 * @return Hash value.
 */
static size_t hash_fname(const char * str)
{
    uint32_t hash = 5381, retval = 0;
    int chunks = NBITS(DEHTABLE_SIZE);
    int c, mask;

    /* djb2 */
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    /* fold */
    mask = (DEHTABLE_SIZE - 1);
    while (chunks--) {
        retval += hash & mask;
        mask <<= (DEHTABLE_SIZE - 1);
    }

    /* truncate */
    retval &= (DEHTABLE_SIZE - 1);

    return retval;
}

/**
 * Find a specific dirent node in chain.
 * @param chain     is the chain array.
 * @param name      is the name of the node searched for.
 * @param name_len  is the length of the name.
 * @return Returns a pointer to the node; Or null if node not found.
 */
static dh_dirent_t * find_node(dh_dirent_t * chain, const char * name)
{
    size_t name_len = strlenn(name, NAME_MAX + 1) + 1;
    size_t offset = 0;
    dh_dirent_t * node;
    dh_dirent_t * retval = 0;

    do {
        node = get_dirent(chain, offset);
        if (is_invalid_offset(node)) {
            KERROR(KERROR_ERR, "Invalid offset in deh node");
            break;
        }

        if (strncmp(node->dh_name, name, name_len) == 0) {
            retval = node;
            break;
        }
        offset += node->dh_size;
    } while (node->dh_link == CH_LINK);

    return retval;
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
        if (is_invalid_offset(node)) {
            KERROR(KERROR_ERR, "Invalid offset in deh node: %u",
                     node->dh_size);

            break;
        }
        chinfo.i_size += node->dh_size;
    } while (node->dh_link == CH_LINK);
    chinfo.i_last = chinfo.i_size - node->dh_size;

    return chinfo;
}

static int rm_node(dh_dirent_t ** chain, const char * name)
{
    const chain_info_t chinfo = find_last_node(*chain);
    size_t old_offset = 0, new_offset = 0, prev_noffset = 0;
    dh_dirent_t * new_chain;
    int match = 0;

    if (chinfo.i_size < sizeof(void *))
        return -ENOENT;

    new_chain = kzalloc(chinfo.i_size);
    if (!new_chain)
        return -ENOMEM;

    /* Initial empty first entry. */
    *get_dirent(new_chain, 0) = (dh_dirent_t){ .dh_size = 0 };

    while (1) {
        const dh_dirent_t * const node = get_dirent(*chain, old_offset);

        if (is_invalid_offset(node)) {
            KERROR(KERROR_ERR, "Invalid offset in deh node");

            kfree(new_chain);
            return -ENOTRECOVERABLE;
        }

        /* Copy if no match or already matched. */
        if (match || strncmp(node->dh_name, name, NAME_MAX + 1) != 0) {
            dh_dirent_t * new_node = get_dirent(new_chain, new_offset);

            memcpy(new_node, node, node->dh_size);
            prev_noffset = new_offset;
            new_offset += node->dh_size;
        } else {
            match = 1;
        }
        old_offset += node->dh_size;

        if (node->dh_link == CH_NOLINK)
            break;
    }

    get_dirent(new_chain, prev_noffset)->dh_link = CH_NOLINK;

    /* Check if new_chain is empty */
    if (get_dirent(new_chain, 0)->dh_size == 0) {
        kfree(new_chain);
        new_chain = NULL;
    }

    /* Replace old chain. */
    kfree(*chain);
    *chain = new_chain;

    return 0;
}

int dh_link(dh_table_t * dir, ino_t vnode_num, const char * name)
{
    size_t name_len = strlenn(name, NAME_MAX + 1) + 1;
    const size_t h = hash_fname(name);
    const size_t entry_size = memalign(DIRENT_SIZE + name_len);
    dh_dirent_t * dea;
    chain_info_t chinfo;
    int retval = 0;

    /* Verify that link doesn't exist */
    if (!dh_lookup(dir, name, NULL)) {
        retval = -EEXIST;
        goto out;
    }

    dea = (*dir)[h];
    if (!dea) { /* Chain array not yet created. */
        dea = kmalloc(entry_size);

        chinfo.i_last = 0;
        chinfo.i_size = 0; /* Set size to 0 to fool the assignment code as
                            * this is the first entry. */
    } else { /* Chain array found. */
        /* Find the last node of the chain and realloc a longer chain. */
        chinfo = find_last_node(dea);
        dea = krealloc(dea, chinfo.i_size + entry_size);
    }
    if (!dea) {
        /* OOM, can't add new entries. */
        retval = -ENOMEM;
        goto out;
    }
    (*dir)[h] = dea;

    /* Create a link */
    get_dirent(dea, chinfo.i_last)->dh_link = CH_LINK;
    /* New node */
    get_dirent(dea, chinfo.i_size)->dh_ino = vnode_num;
    get_dirent(dea, chinfo.i_size)->dh_size = entry_size;
    get_dirent(dea, chinfo.i_size)->dh_link = CH_NOLINK;
    strlcpy(get_dirent(dea, chinfo.i_size)->dh_name, name, name_len + 1);

out:
    return retval;
}

int dh_unlink(dh_table_t * dir, const char * name)
{
    const size_t h = hash_fname(name);
    dh_dirent_t ** dea;
    int err;

    dea = &((*dir)[h]);
    if (!dea)
        return -ENOENT;

    err = rm_node(dea, name);
    if (err)
        return err;

    return 0;
}

void dh_destroy_all(dh_table_t * dir)
{
    size_t i;

    /* Free all dir entry chains. */
    for (i = 0; i < DEHTABLE_SIZE; i++) {
        if ((*dir)[i])
            kfree((*dir)[i]);
    }
}


int dh_lookup(dh_table_t * dir, const char * name, ino_t * vnode_num)
{
    const size_t h = hash_fname(name);
    dh_dirent_t * dea;
    dh_dirent_t * dent;
    int retval = -ENOENT;

    dea = (*dir)[h];
    if (!dea)
        goto out;

    dent = find_node(dea, name);
    if (!dent)
        goto out;

    if (vnode_num)
        *vnode_num = dent->dh_ino;

    retval = 0;
out:
    return retval;
}

int dh_revlookup(dh_table_t * dir, ino_t ino, char * name, size_t name_len)
{
    dh_dir_iter_t it = dh_get_iter(dir);
    dh_dirent_t * de;

    while ((de = dh_iter_next(&it))) {
        if (de->dh_ino == ino) {
            size_t len;

            len = strlcpy(name, de->dh_name, name_len);
            if (len >= name_len)
                return -ENAMETOOLONG;
            return 0;
        }
    }

    return -ENOENT;
}

dh_dir_iter_t dh_get_iter(dh_table_t * dir)
{
    dh_dir_iter_t it = {
        .dir = dir,
        .dea_ind = 0,
        .ch_ind = SIZE_MAX
    };

    return it;
}

dh_dirent_t * dh_iter_next(dh_dir_iter_t * it)
{
    dh_dirent_t * dea; /* Dir entry array (chain). */
    dh_dirent_t * node; /* Dir entry node in chain. */

    /* Already iterated over all nodes? */
    if (!it->dir || it->dea_ind >= DEHTABLE_SIZE)
        return NULL;

    if (it->ch_ind == SIZE_MAX) { /* Get a next chain array */
        size_t i = it->dea_ind;

        it->ch_ind = 0; /* Reset chain index */

        /* Get a dir entry array */
        do {
            dea = (*(it->dir))[i];
            i++;
        } while (!dea && i < DEHTABLE_SIZE);

        it->dea_ind = i - 1;
    } else { /* Still iterating the old chain. */
        dea = (*(it->dir))[it->dea_ind];
    }
    if (!dea)
        return NULL; /* Empty hash table or No more entries. */

    /* Get a node from the chain. */
    node = get_dirent(dea, it->ch_ind);
    if (is_invalid_offset(node))
        return NULL; /* Broken dirent hash table. */

    if (node->dh_link == CH_LINK) {
        it->ch_ind += node->dh_size;
    } else {
        it->dea_ind++;
        it->ch_ind = SIZE_MAX; /* It's quite safe to assume the array is
                                * shorter than SIZE_MAX. */
    }

    return node;
}

size_t dh_nr_entries(dh_table_t * dir)
{
    dh_dir_iter_t it = dh_get_iter(dir);
    size_t n = 0;

    while (dh_iter_next(&it)) {
        n++;
    }

    return n;
}
