/**
 *******************************************************************************
 * @file    sysfs.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system: sysfs.
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

/** @addtogroup sysfs
 * @{
 */

#include <fs/fs.h>

fs_t sysfs_cb = {
    .fsname = "sysfs",
    .mount = 0,
    .umount = 0,
    .sbl_head = &sysfs_sbl
};

superblock_lnode_t sysfs_sbl = {
    .sb = 0,
    .next = 0
};

vnode_t sysfs_root = {
    .vnode_num = 0,
    .refcount = 1,
    .len = 0,
    .mutex = 0,
    .mode = 0,
    .sb = 0,
    .vnode_ops = &sysfs_vnode_ops
};

/* TODO */
vnode_ops_t sysfs_vnode_ops;

sysfs_init(void)
{
    sysfs_root.dev = DEV_MMTODEV(1, fs_get_pfs_minor());
}

fs_superblock_t * sysfs_mount(const char * mpoint, uint32_t mode,
        int parm_len, char * parm)
{
    fs_superblock_t * sb = 0;

    if (sysfs_root->sb != 0)
        goto out; /* sysfs can be mounted only once. */

    sb = kmalloc(sizeof(fs_superblock_t));
    if (sb == 0)
        goto out;
    sb->mount_point = kmalloc(strlenn(mpoint, SIZE_MAX) + 1);
    if (sb->mtpt_path == 0) {
        goto free_sb;
    }

    sb->mode_flags = mode;
    sb->root = &sysfs_root;
    strcpy(sb->mtpt_path, mpoint);
    sb->lookup_vnode = 0; /* TODO */
    sb->lookup_file = 0;
    sb->delete_vnode = 0;

    sysfs_root.sb = sb;
    sysfs_sbl.sb = sb;

out:
    return sb;
free_sb:
    kfree(sb);
    return 0;
}

/**
 * @}
 */

/**
 * @}
 */
