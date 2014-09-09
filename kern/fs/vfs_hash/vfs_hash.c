/*-
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <subr_hash.h>
#include <mount.h>
#include <errno.h>
#include <kmalloc.h>
#include <kinit.h>
#include <fs/fs.h>
#include <fs/vfs_hash.h>

static LIST_HEAD(vfs_hash_head, vnode)  * vfs_hash_tbl;
static LIST_HEAD(,vnode)        vfs_hash_side;
static unsigned long            vfs_hash_mask;
static struct mtx               vfs_hash_mtx;

/* TODO */
static int desiredvnodes = 100;

void vfs_hashinit(void) __attribute__((constructor));
void vfs_hashinit(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(proc_init);

    vfs_hash_tbl = hashinit(desiredvnodes, &vfs_hash_mask);
    mtx_init(&vfs_hash_mtx, MTX_TYPE_SPIN);
    LIST_INIT(&vfs_hash_side);

    SUBSYS_INITFINI("vfs_hash ok");
}

unsigned vfs_hash_index(struct vnode * vp)
{
    return vp->vn_hash + vp->sb->sb_hashseed;
}

static struct vfs_hash_head *
vfs_hash_bucket(const struct fs_superblock * mp, unsigned hash)
{
    return &vfs_hash_tbl[(hash + mp->sb_hashseed) & vfs_hash_mask];
}

int vfs_hash_get(const struct fs_superblock * mp, unsigned hash,
                 struct vnode ** vpp, vfs_hash_cmp_t * fn, void * arg)
{
    struct vnode *vp;

    while (1) {
        mtx_lock(&vfs_hash_mtx);
        LIST_FOREACH(vp, vfs_hash_bucket(mp, hash), vn_hashlist) {
            if (vp->vn_hash != hash)
                continue;
            if (vp->sb != mp)
                continue;
            if (fn != NULL && fn(vp, arg))
                continue;
            VN_LOCK(vp);
            mtx_unlock(&vfs_hash_mtx);
            /* TODO Incr ref count by fn call? */
            vp->vn_refcount++;
            *vpp = vp;
            return 0; /* TODO Return or break? */
        }
        if (vp == NULL) {
            mtx_unlock(&vfs_hash_mtx);
            *vpp = NULL;
            return 0;
        }
    }
}

void vfs_hash_remove(struct vnode * vp)
{

    mtx_lock(&vfs_hash_mtx);
    LIST_REMOVE(vp, vn_hashlist);
    mtx_unlock(&vfs_hash_mtx);
}

int vfs_hash_insert(struct vnode * vp, unsigned hash,
                    struct vnode ** vpp, vfs_hash_cmp_t * fn, void * arg)
{
    struct vnode *vp2;

    *vpp = NULL;
    while (1) {
        mtx_lock(&vfs_hash_mtx);
        LIST_FOREACH(vp2,
            vfs_hash_bucket(vp->sb, hash), vn_hashlist) {
            if (vp2->vn_hash != hash)
                continue;
            if (vp2->sb != vp->sb)
                continue;
            if (fn != NULL && fn(vp2, arg))
                continue;
            VN_LOCK(vp2);
            mtx_unlock(&vfs_hash_mtx);
            /* TODO incr refcount */
            mtx_lock(&vfs_hash_mtx);
            LIST_INSERT_HEAD(&vfs_hash_side, vp, vn_hashlist);
            mtx_unlock(&vfs_hash_mtx);
            VN_UNLOCK(vp);
            *vpp = vp2;
            return 0;
        }
        if (vp2 == NULL)
            break;

    }
    vp->vn_hash = hash;
    LIST_INSERT_HEAD(vfs_hash_bucket(vp->sb, hash), vp, vn_hashlist);
    mtx_unlock(&vfs_hash_mtx);

    return 0;
}

void vfs_hash_rehash(struct vnode * vp, unsigned hash)
{

    mtx_lock(&vfs_hash_mtx);
    LIST_REMOVE(vp, vn_hashlist);
    LIST_INSERT_HEAD(vfs_hash_bucket(vp->sb, hash), vp, vn_hashlist);
    vp->vn_hash = hash;
    mtx_unlock(&vfs_hash_mtx);
}
