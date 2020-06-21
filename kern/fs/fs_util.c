/**
 *******************************************************************************
 * @file    fs_util.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system utils.
 * @section LICENSE
 * Copyright (c) 2019, 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/types.h>
#include <buf.h>
#include <hal/core.h>
#include <kerror.h>
#include <klocks.h>
#include <kmalloc.h>
#include <kstring.h>
#include <fs/fs.h>
#include <fs/fs_util.h>

void fs_init_superblock(struct fs_superblock * sb, struct fs * fs)
{
    sb->fs = fs;
    sb->root = NULL;
}

void fs_insert_superblock(struct fs * fs, struct fs_superblock * new_sb)
{
    mtx_t * lock = &fs->fs_giant;

    mtx_lock(lock);
    SLIST_INSERT_HEAD(&fs->sblist_head, new_sb, _sblist);
    mtx_unlock(lock);
}

void fs_remove_superblock(struct fs * fs, struct fs_superblock * sb)
{
    mtx_t * lock = &fs->fs_giant;

    mtx_lock(lock);
    SLIST_REMOVE(&fs->sblist_head, sb, fs_superblock, _sblist);
    mtx_unlock(lock);
}

struct fs_superblock *
fs_iterate_superblocks(fs_t * fs, struct fs_superblock * sb)
{
    mtx_t * lock = &fs->fs_giant;

    mtx_lock(lock);

    if (SLIST_EMPTY(&fs->sblist_head)) {
        mtx_unlock(lock);
        return NULL;
    }

    /* Get the next sb and skip filesytems that are inherited for fs */
    do {
        if (!sb)
            sb = SLIST_FIRST(&fs->sblist_head);
        else
            sb = SLIST_NEXT(sb, _sblist);
    } while (sb && DEV_MAJOR(sb->vdev_id) != fs->fs_majornum);

    mtx_unlock(lock);

    return sb;
}

vnode_t * fs_create_pseudofs_root(fs_t * newfs, int majornum)
{
    int err;
    vnode_t * rootnode;

    /*
     * We use a little trick here and create a temporary vnode that will be
     * destroyed after succesful mount.
     */

    rootnode = kzalloc(sizeof(vnode_t));
    if (!rootnode)
        return NULL;

    /* Temp root dir */
    rootnode->vn_next_mountpoint = rootnode;
    vrefset(rootnode, 1);
    mtx_init(&rootnode->vn_lock, VN_LOCK_TYPE, VN_LOCK_OPT);

    err = fs_mount(rootnode, "", "ramfs", 0, "", 1);
    if (err) {
        KERROR(KERROR_ERR,
               "Unable to create a pseudo fs root vnode for %s (%i)\n",
               newfs->fsname, err);

        return NULL;
    }
    rootnode = rootnode->vn_next_mountpoint;
    kfree(rootnode->vn_prev_mountpoint);
    rootnode->vn_prev_mountpoint = rootnode;
    rootnode->vn_next_mountpoint = rootnode;

    newfs->fs_majornum = majornum;
    newfs->sblist_head = rootnode->sb->fs->sblist_head;
    rootnode->sb->fs = newfs;

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
    vnode->vn_next_mountpoint = vnode;
    vnode->vn_prev_mountpoint = vnode;
    vnode->sb = sb;
    vnode->vnode_ops = (vnode_ops_t *)vnops;
    mtx_init(&vnode->vn_lock, VN_LOCK_TYPE, VN_LOCK_OPT);
}

void fs_vnode_cleanup(vnode_t * vnode)
{
    struct buf * var;
    struct buf * nxt;

    KASSERT(vnode != NULL, "vnode can't be null.");

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

void fs_parse_parm(char * parm, const char * names[],
                   void * parsed, size_t parsed_size)
{
    const size_t max_name = 80;
    char * s = parm;
    char * w = ";";
    char * lasts;
    char ** pa = (char **)parsed;

    memset(parsed, 0, parsed_size);

    for (s = kstrtok(s, w, &lasts); s; s = kstrtok(0, w, &lasts)) {
        const char * cut = strstr(s, "=");
        const ssize_t end = (ptrdiff_t)cut - (ptrdiff_t)(s);
        const char ** name_p = names;
        size_t i;

        for (i = 0; *name_p && i < parsed_size / sizeof(void *); i++) {
            if (!strncmp(*name_p, s, end > 0 ? end : max_name)) {
                if (end > 0) {
                    pa[i] = s + end + 1;
                } else {
                    pa[i] = "y";
                }
            }

            name_p++;
        }
    }
}
