/*-
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <sys/param.h>
#include <sys/queue.h>
#include <stddef.h>
#include <stdint.h>
#include <subr_hash.h>
#include <mount.h>
#include <errno.h>
#include <fs/fs.h>
#include <fs/vfs_hash.h>
#include <kinit.h>
#include <klocks.h>
#include <kmalloc.h>

struct vfs_hash_ctx {
    const char * ctx_fsname;
    LIST_HEAD(vfs_hash_head, vnode) * ctx_hash_tbl;
    LIST_HEAD(,vnode) ctx_hash_side;
    unsigned long ctx_hash_mask;
    vfs_hash_cmp_t * ctx_cmp_fn;
    mtx_t ctx_lock;
};

static id_t next_ctx_id;
static struct vfs_hash_ctx vfs_ctx[configVFS_HASH_CTX_MAX];

static inline struct vfs_hash_ctx * vfs_hash_get_ctx(id_t cid)
{
    if (0 <= cid || cid < next_ctx_id) {
        return vfs_ctx + cid;
    }
    return NULL;
}

id_t vfs_hash_new_ctx(const char * fsname, int desiredvnodes,
                      vfs_hash_cmp_t * cmp_fn)
{
    id_t id;
    struct vfs_hash_ctx * ctx;

    if (desiredvnodes <= 0) {
        return -EINVAL;
    }

    if (next_ctx_id >= (int)num_elem(vfs_ctx)) {
        return -ENOBUFS;
    }

    id = next_ctx_id++;
    ctx = vfs_ctx + id;

    ctx->ctx_fsname = fsname;
    ctx->ctx_hash_tbl = hashinit(desiredvnodes, &ctx->ctx_hash_mask);
    LIST_INIT(&ctx->ctx_hash_side);
    ctx->ctx_cmp_fn = cmp_fn;
    mtx_init(&ctx->ctx_lock, MTX_TYPE_SPIN, MTX_OPT_DEFAULT);

    return id;
}

unsigned vfs_hash_index(struct vnode * vp)
{
    return vp->vn_hash + vp->sb->sb_hashseed;
}

static struct vfs_hash_head *
vfs_hash_bucket(struct vfs_hash_ctx * ctx, const struct fs_superblock * mp,
                unsigned hash)
{
    return &ctx->ctx_hash_tbl[(hash + mp->sb_hashseed) & ctx->ctx_hash_mask];
}

int vfs_hash_get(id_t cid, const struct fs_superblock * mp,
                 unsigned hash, struct vnode ** vpp, void * cmp_arg)
{
    struct vnode * vp;
    struct vfs_hash_ctx * ctx = vfs_hash_get_ctx(cid);

    if (!ctx) {
        return -EINVAL;
    }

    while (1) {
        mtx_lock(&ctx->ctx_lock);
        LIST_FOREACH(vp, vfs_hash_bucket(ctx, mp, hash), vn_hashlist) {
            if (vp->vn_hash != hash)
                continue;
            if (vp->sb != mp)
                continue;
            if (ctx->ctx_cmp_fn && ctx->ctx_cmp_fn(vp, cmp_arg))
                continue;
            //VN_LOCK(vp);
            mtx_unlock(&ctx->ctx_lock);
            vref(vp);
            *vpp = vp;
            return 0; /* TODO Return or break? */
        }
        if (vp == NULL) {
            mtx_unlock(&ctx->ctx_lock);
            *vpp = NULL;
            return 0;
        }
    }
}

int vfs_hash_remove(id_t cid, struct vnode * vp)
{
    struct vfs_hash_ctx * ctx = vfs_hash_get_ctx(cid);

    if (!ctx) {
        return -EINVAL;
    }

    mtx_lock(&ctx->ctx_lock);
    LIST_REMOVE(vp, vn_hashlist);
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}

int vfs_hash_insert(id_t cid, struct vnode * vp, unsigned hash,
                    struct vnode ** vpp, void * cmp_arg)
{
    struct vnode * vp2;
    struct vfs_hash_ctx * ctx = vfs_hash_get_ctx(cid);

    if (!ctx) {
        return -EINVAL;
    }

    *vpp = NULL;
    while (1) {
        mtx_lock(&ctx->ctx_lock);
        LIST_FOREACH(vp2, vfs_hash_bucket(ctx, vp->sb, hash), vn_hashlist) {
            if (vp2->vn_hash != hash)
                continue;
            if (vp2->sb != vp->sb)
                continue;
            if (!ctx->ctx_cmp_fn && ctx->ctx_cmp_fn(vp2, cmp_arg))
                continue;
            //VN_LOCK(vp2);
            mtx_unlock(&ctx->ctx_lock);
            /* TODO incr refcount of vp2 */
            mtx_lock(&ctx->ctx_lock);
            LIST_INSERT_HEAD(&ctx->ctx_hash_side, vp, vn_hashlist);
            mtx_unlock(&ctx->ctx_lock);
            //VN_UNLOCK(vp);
            *vpp = vp2;
            return 0;
        }
        if (vp2 == NULL)
            break;

    }
    vp->vn_hash = hash;
    LIST_INSERT_HEAD(vfs_hash_bucket(ctx, vp->sb, hash), vp, vn_hashlist);
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}

int vfs_hash_rehash(id_t cid, struct vnode * vp, unsigned hash)
{
    struct vfs_hash_ctx * ctx = vfs_hash_get_ctx(cid);

    if (!ctx) {
        return -EINVAL;
    }

    mtx_lock(&ctx->ctx_lock);
    LIST_REMOVE(vp, vn_hashlist);
    LIST_INSERT_HEAD(vfs_hash_bucket(ctx, vp->sb, hash), vp, vn_hashlist);
    vp->vn_hash = hash;
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}
