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
    size_t ctx_hash_mask;
    vfs_hash_cmp_t * ctx_cmp_fn;
    mtx_t ctx_lock;
};

vfs_hash_ctx_t vfs_hash_new_ctx(const char * fsname, unsigned desiredvnodes,
                                vfs_hash_cmp_t * cmp_fn)
{
    struct vfs_hash_ctx * ctx;

    ctx = kmalloc(sizeof(struct vfs_hash_ctx));
    if (!ctx)
        return NULL;

    ctx->ctx_fsname = fsname;
    ctx->ctx_hash_tbl = hashinit(desiredvnodes, &ctx->ctx_hash_mask);
    ctx->ctx_cmp_fn = cmp_fn;
    mtx_init(&ctx->ctx_lock, MTX_TYPE_SPIN, MTX_OPT_DEFAULT);

    return ctx;
}

size_t vfs_hash_index(struct vnode * vp)
{
    return vp->vn_hash + vp->sb->sb_hashseed;
}

static inline struct vfs_hash_head *
vfs_hash_bucket(struct vfs_hash_ctx * ctx,
                const struct fs_superblock * mp,
                size_t hash)
{
    return &ctx->ctx_hash_tbl[(hash + mp->sb_hashseed) & ctx->ctx_hash_mask];
}

int vfs_hash_get(vfs_hash_ctx_t ctx, const struct fs_superblock * mp,
                 size_t hash, struct vnode ** vpp, void * cmp_arg)
{
    struct vnode * vp;

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

int vfs_hash_remove(vfs_hash_ctx_t ctx, struct vnode * vp)
{

    mtx_lock(&ctx->ctx_lock);
    LIST_REMOVE(vp, vn_hashlist);
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}

int vfs_hash_foreach(vfs_hash_ctx_t ctx, const struct fs_superblock * mp,
                     void (*cb)(struct vnode *))
{
    struct vfs_hash_head * bucket;

    if (!ctx) {
        return -EINVAL;
    }

    mtx_lock(&ctx->ctx_lock);
    HASH_FOREACH_BUCKET_BEGIN(ctx->ctx_hash_tbl, mp->sb_hashseed, bucket) {
        struct vnode * vp;
        struct vnode * vp_tmp;

        LIST_FOREACH_SAFE(vp, bucket, vn_hashlist, vp_tmp) {
            if (vp->sb != mp)
                continue;
            mtx_unlock(&ctx->ctx_lock);
            cb(vp);
            mtx_lock(&ctx->ctx_lock);
        }
    } HASH_FOREACH_BUCKET_END();
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}

int vfs_hash_insert(vfs_hash_ctx_t ctx, struct vnode * vp, size_t hash,
                    struct vnode ** vpp, void * cmp_arg)
{
    struct vnode * vp2;

    *vpp = NULL;
    mtx_lock(&ctx->ctx_lock);
    LIST_FOREACH(vp2, vfs_hash_bucket(ctx, vp->sb, hash), vn_hashlist) {
        if (vp2->vn_hash != hash)
            continue;
        if (vp2->sb != vp->sb)
            continue;
        if (ctx->ctx_cmp_fn && ctx->ctx_cmp_fn(vp2, cmp_arg))
            continue;
        mtx_unlock(&ctx->ctx_lock);
        /* TODO incr refcount of vp2 */
        *vpp = vp2;
        return 0;
    }
    vp->vn_hash = hash;
    LIST_INSERT_HEAD(vfs_hash_bucket(ctx, vp->sb, hash), vp, vn_hashlist);
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}

int vfs_hash_rehash(vfs_hash_ctx_t ctx, struct vnode * vp, size_t hash)
{
    mtx_lock(&ctx->ctx_lock);
    LIST_REMOVE(vp, vn_hashlist);
    LIST_INSERT_HEAD(vfs_hash_bucket(ctx, vp->sb, hash), vp, vn_hashlist);
    vp->vn_hash = hash;
    mtx_unlock(&ctx->ctx_lock);

    return 0;
}
