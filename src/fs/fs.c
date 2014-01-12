/**
 *******************************************************************************
 * @file    fs.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system.
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

#include <kstring.h>
#include <kmalloc.h>
#include <sched.h>
#include <ksignal.h>
#include <syscalldef.h>
#include <syscall.h>
#define KERNEL_INTERNAL 1
#include <errno.h>
#include "fs.h"

/**
 * Linked list of registered file systems.
 */
static fsl_node_t * fsl_head;
/* TODO fsl as an array with:
 * struct { size_t count; fst_t * fs_array; };
 * Then iterator can just iterate over fs_array
 */


static fs_t * find_fs(const char * fsname);

/**
 * Register a new file system driver.
 * @param fs file system control struct.
 */
int fs_register(fs_t * fs)
{
    fsl_node_t * new;
    int retval = -1;

    if (fsl_head == 0) { /* First entry */
        fsl_head = kmalloc(sizeof(fsl_node_t));
        new = fsl_head;
    } else {
        new = kmalloc(sizeof(fsl_node_t));
    }
    if (new == 0)
        goto out;

    new->fs = fs;
    if (fsl_head == new) { /* First entry */
        new->next = 0;
    } else {
        fsl_node_t * node = fsl_head;

        while (node->next != 0) {
            node = node->next;
        }
        node->next = new;
    }
    retval = 0;

out:
    return retval;
}

/**
 * Lookup for vnode by path.
 * @param[out]  vnode   is the target where vnode struct is stored.
 * @param       str     is the path.
 * @return Returns zero if vnode was found or error code.
 */
int lookup_vnode(vnode_t ** vnode, const char * str)
{
    int retval = 0;

    return retval;
}

int fs_mount(const char * mount_point, const char * fsname, uint32_t mode,
        int parm_len, char * parm)
{
    vnode_t * vnode_mp;
    fs_t * fs;
    int retval = 0;

    /* TODO handle dot & dot-dot so that mount point is always fqp
     * or should we return error if mp is not a fqp already? */

    /* Find the mount point and accept if found */
    if (lookup_vnode(&vnode_mp, mount_point)) {
        retval = -1; /* Path doesn't exist. */
        goto out;
    }

    fs = find_fs(fsname);
    if (fs == 0) {
        retval = -2; /* fs doesn't exist. */
        goto out;
    }

    fs->mount(mount_point, mode, parm_len, parm);

out:
    return retval;
}

/**
 * Find registered file system by name.
 * @param fsname is the name of the file system.
 * @return The file system structure.
 */
static fs_t * find_fs(const char * fsname)
{
    fsl_node_t * node = fsl_head;

    do {
        if (strcmp(node->fs->fsname, fsname))
            break;
    } while ((node = node->next) != 0);

    return (node != 0) ? node->fs : 0;
}

/**
 * Initialize a file system superblock iterator.
 * Iterator is used to iterate over all superblocks of mounted file systems.
 * @param it is an untilitialized superblock iterator struct.
 */
void fs_init_sb_iterator(sb_iterator_t * it)
{
    it->curr_fs = fsl_head;
    it->curr_sb = fsl_head->fs->sbl_head;
}

/**
 * Iterate over superblocks of mounted file systems.
 * @param it is the iterator struct.
 * @return The next superblock or 0.
 */
fs_superblock_t * fs_next_sb(sb_iterator_t * it)
{
    fs_superblock_t * retval = (it->curr_sb != 0) ? &(it->curr_sb->sbl_sb) : 0;

    if (retval == 0)
        goto out;

    it->curr_sb = it->curr_sb->next;
    if (it->curr_sb == 0) {
        while(1) {
            it->curr_fs = it->curr_fs->next;
            if (it->curr_fs == 0)
                break;
            it->curr_sb = it->curr_fs->fs->sbl_head;
            if (it->curr_sb != 0)
                break;
        }
    }

out:
    return retval;
}

/**
 * Get next free pseudo fs minor code.
 */
unsigned int fs_get_pfs_minor(void)
{
    unsigned int static pfs_minor = 0;
    unsigned int retval = pfs_minor;

    pfs_minor++;
    return retval;
}

/**
 * fs syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uint32_t fs_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_FS_CREAT:
        current_thread->errno = ENOSYS;
        return -1;

    case SYSCALL_FS_OPEN:
        current_thread->errno = ENOSYS;
        return -2;

    case SYSCALL_FS_CLOSE:
        current_thread->errno = ENOSYS;
        return -3;

    case SYSCALL_FS_READ:
        current_thread->errno = ENOSYS;
        return -4;

    case SYSCALL_FS_WRITE:
        current_thread->errno = ENOSYS;
        return -5;

    case SYSCALL_FS_LSEEK:
        current_thread->errno = ENOSYS;
        return -6;

    case SYSCALL_FS_DUP:
        current_thread->errno = ENOSYS;
        return -7;

    case SYSCALL_FS_LINK:
        current_thread->errno = ENOSYS;
        return -8;

    case SYSCALL_FS_UNLINK:
        current_thread->errno = ENOSYS;
        return -9;

    case SYSCALL_FS_STAT:
        current_thread->errno = ENOSYS;
        return -10;

    case SYSCALL_FS_FSTAT:
        current_thread->errno = ENOSYS;
        return -11;

    case SYSCALL_FS_ACCESS:
        current_thread->errno = ENOSYS;
        return -12;

    case SYSCALL_FS_CHMOD:
        current_thread->errno = ENOSYS;
        return -13;

    case SYSCALL_FS_CHOWN:
        current_thread->errno = ENOSYS;
        return -14;

    case SYSCALL_FS_UMASK:
        current_thread->errno = ENOSYS;
        return -15;

    case SYSCALL_FS_IOCTL:
        current_thread->errno = ENOSYS;
        return -16;

    case SYSCALL_FS_MOUNT:
        current_thread->errno = ENOSYS;
        return -17;

    default:
        return (uint32_t)NULL;
    }
}

/**
 * @}
 */
