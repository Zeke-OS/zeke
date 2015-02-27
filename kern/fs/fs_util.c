/**
 *******************************************************************************
 * @file    fs_util.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system utils.
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

#include <stddef.h>
#include <sys/param.h>
#include <kmalloc.h>
#include <buf.h>
#include <fs/fs.h>
#include <fs/fs_util.h>

vnode_t * fs_create_pseudofs_root(fs_t * newfs, int majornum)
{
    int err;
    vnode_t * rootnode;

    /*
     * We use a little trick here and create a temporary vnode that will be
     * destroyed after succesful mount.
     */

    rootnode = kcalloc(1, sizeof(vnode_t));
    if (!rootnode)
        return NULL;

    /* Temp root dir */
    rootnode->vn_mountpoint = rootnode;
    vrefset(rootnode, 1);
    mtx_init(&rootnode->vn_lock, VN_LOCK_MODES);

    err = fs_mount(rootnode, "", "ramfs", 0, "", 1);
    if (err) {
        KERROR(KERROR_ERR,
               "Unable to create a pseudo fs root vnode for %s (%i)\n",
               newfs->fsname, err);

        return NULL;
    }
    rootnode = rootnode->vn_mountpoint;
    kfree(rootnode->vn_prev_mountpoint);
    rootnode->vn_prev_mountpoint = rootnode;
    rootnode->vn_mountpoint = rootnode;

    /* TODO The following is something we'd like to do but can't at the moment,
     * it would allow us for example printing error and debug messages with a
     * proper fsname.
     */
#if 0
    newfs->sbl_head = rootnode->sb->fs->sbl_head;
    rootnode->sb->fs = newfs;
#endif
    rootnode->sb->vdev_id = DEV_MMTODEV(majornum, 0);

    return rootnode;
}

void fs_inherit_vnops(vnode_ops_t * dest_vnops, const vnode_ops_t * base_vnops)
{
    void ** dest = (void **)dest_vnops;
    void ** base = (void **)base_vnops;
    const size_t fn_count = sizeof(vnode_ops_t) / sizeof(void *);

    for (size_t i = 0; i < fn_count; i++) {
        if (!dest[i])
            dest[i] = base[i];
    }
}

void fs_vnode_init(vnode_t * vnode, ino_t vn_num, struct fs_superblock * sb,
                   const vnode_ops_t * const vnops)
{
    vnode->vn_num = vn_num;
    vnode->vn_refcount = ATOMIC_INIT(0);
    vnode->vn_mountpoint = vnode;
    vnode->vn_prev_mountpoint = vnode;
    vnode->sb = sb;
    vnode->vnode_ops = (vnode_ops_t *)vnops;
    mtx_init(&vnode->vn_lock, VN_LOCK_MODES);
}

void fs_vnode_cleanup(vnode_t * vnode)
{
    struct buf * var, * nxt;

#ifdef configFS_DEBUG
    KASSERT(vnode != NULL, "vnode can't be null.");
#endif

    /* Release associated buffers. */
    if (!SPLAY_EMPTY(&vnode->vn_bpo.sroot)) {
        for (var = SPLAY_MIN(bufhd_splay, &vnode->vn_bpo.sroot);
                var != NULL; var = nxt) {
            nxt = SPLAY_NEXT(bufhd_splay, &vnode->vn_bpo.sroot, var);
            SPLAY_REMOVE(bufhd_splay, &vnode->vn_bpo.sroot, var);
            if (!(var->b_flags & B_DONE))
                var->b_flags |= B_DELWRI;
            brelse(var);
        }
   }
}
