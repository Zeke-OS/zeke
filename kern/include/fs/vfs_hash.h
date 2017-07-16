/*-
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2005 Poul-Henning Kamp
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#pragma once
#ifndef VFS_HASH_H
#define VFS_HASH_H

#include <sys/types/_id_t.h>
#include <sys/types/_size_t.h>

struct vnode;

/**
 * vnode comparator function type.
 */
typedef int vfs_hash_cmp_t(struct vnode * vp, void * arg);


/**
 * Get a new vfs_hash context.
 * @note Not thread-safe.
 * @return The id of the context.
 */
id_t vfs_hash_new_ctx(const char * fsname, int desiredvnodes,
                      vfs_hash_cmp_t * cmp_fn);

/**
 * Get a vnode pointer from vfs_hash.
 * @param cid is the vfs_hash context ID.
 * @retval -EINVAL if cid is invalid.
 */
int vfs_hash_get(id_t cid, const struct fs_superblock * mp,
                 size_t hash, struct vnode ** vpp, void * cmp_arg);

size_t vfs_hash_index(struct vnode * vp);

/**
 * Walkthrough each vnode belonging to the given mp and call a callback cb.
 */
int vfs_hash_foreach(id_t cid, const struct fs_superblock * mp,
                     void (*cb)(struct vnode *));

/**
 * Insert a vnode pointer to vfs_hash.
 * @param cid is the vfs_hash context ID.
 * @retval -EINVAL if cid is invalid.
 */
int vfs_hash_insert(id_t cid, struct vnode * vp, size_t hash,
                    struct vnode ** vpp, void * cmp_arg)
    __attribute__((nonnull(2, 4)));

/**
 * Rehash.
 * @param cid is the vfs_hash context ID.
 * @retval -EINVAL if cid is invalid.
 */
int vfs_hash_rehash(id_t cid, struct vnode * vp, size_t hash);

/**
 * Remove a vnode from the hashmap of a vfs_hash context.
 * @param cid is the vfs_hash context ID.
 * @retval -EINVAL if cid is invalid.
 */
int vfs_hash_remove(id_t cid, struct vnode * vp);

#endif /* VFS_HASH_H */
