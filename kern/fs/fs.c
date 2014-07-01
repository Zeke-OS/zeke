/**
 *******************************************************************************
 * @file    fs.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL 1
#include <kinit.h>
#include <kerror.h>
#include <kstring.h>
#include <kmalloc.h>
#include <vm/vm.h>
#include <proc.h>
#include <sched.h>
#include <ksignal.h>
#include <errno.h>
#include <fcntl.h>
#include <fs/mbr.h>
#include <fs/fs.h>

mtx_t fslock;
#define FS_LOCK()       mtx_spinlock(&fslock)
#define FS_UNLOCK()     mtx_unlock(&fslock)
#define FS_TESTLOCK()   mtx_test(&fslock)
#define FS_LOCK_INIT()  mtx_init(&fslock, MTX_DEF | MTX_SPIN)


/**
 * Linked list of registered file systems.
 */
static fsl_node_t * fsl_head;
/* TODO fsl as an array with:
 * struct { size_t count; fst_t * fs_array; };
 * Then iterator can just iterate over fs_array
 */


void fs_init(void)
{
    SUBSYS_INIT();

    FS_LOCK_INIT();

    SUBSYS_INITFINI("fs OK");
}

/**
 * Register a new file system driver.
 * @param fs file system control struct.
 */
int fs_register(fs_t * fs)
{
    fsl_node_t * new;
    int retval = -1;

    FS_LOCK();

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
    FS_UNLOCK();
    return retval;
}

int lookup_vnode(vnode_t ** result, vnode_t * root, const char * str, int oflags)
{
    char * path;
    char * nodename;
    char * lasts;
    int retval = 0;

    if (!(result && root && root->vnode_ops && str))
        return -EINVAL;

    path = kstrdup(str, PATH_MAX);
    if (!path)
        return -ENOMEM;

    if (!(nodename = kstrtok(path, PATH_DELIMS, &lasts))) {
        retval = -EINVAL;
        goto out;
    }

    /*
     * Start looking for a vnode.
     * We don't care if root is not a directory because lookup will spot it
     * anyway.
     */
    *result = root;
    do {
        vnode_t * vnode;

        if (!strcmp(nodename, "."))
            continue;

        retval = (*result)->vnode_ops->lookup(*result, nodename,
                strlenn(nodename, FS_FILENAME_MAX), &vnode);
        if (retval) {
            goto out;
        }
        /* TODO - soft links but if O_NOFOLLOW we should fail on link and return
         *        -ELOOP
         */
        *result = vnode->vn_mountpoint;
    } while ((nodename = kstrtok(0, PATH_DELIMS, &lasts)));

    if ((oflags & O_DIRECTORY) && !S_ISDIR((*result)->vn_mode)) {
        retval = -ENOTDIR;
        goto out;
    }

out:
    kfree(path);
    return retval;
}

int fs_namei_proc(vnode_t ** result, char * path)
{
    vnode_t * start;
    int oflags = 0;

    if (path[0] == '/') {
        path++;
        start = curproc->croot;
    } else {
        start = curproc->cwd;
    }

    if (path[strlenn(path, PATH_MAX)] == '/')
        oflags = O_DIRECTORY;

    return lookup_vnode(result, start, path, oflags);
}

int fs_mount(vnode_t * target, const char * source, const char * fsname,
        uint32_t flags, const char * parm, int parm_len)
{
    fs_t * fs = 0;
    struct fs_superblock * sb;
    vnode_t * dotdot;

    if (lookup_vnode(&dotdot, target, "..", O_DIRECTORY)) {
        /* We could return -ENOLINK but usually this means that we are mounting
         * to some pseudo vnode, so we just ignore .. for now */
        dotdot = 0;
    }

    if (fsname) {
        fs = fs_by_name(fsname);
    } else {
        /* TODO Try to determine type of the fs */
    }
    if (!fs)
        return -ENOTSUP; /* fs doesn't exist. */

    sb = fs->mount(source, flags, parm, parm_len);
    if (!sb)
        return -ENODEV;

    if (dotdot) {
        /* TODO - Make .. of the new mount to point prev dir of the mp
         *      - inherit mode from original target dir?
         */
        //fs_link(sb->root, dotdot, "..", 3);
    }

    sb->mountpoint = target;
    target->vn_mountpoint = sb->root;

    return 0;
}

fs_t * fs_by_name(const char * fsname)
{
    fsl_node_t * node = fsl_head;

    do {
        if (!strcmp(node->fs->fsname, fsname))
            break;
    } while ((node = node->next) != 0);

    return (node != 0) ? node->fs : 0;
}

void fs_init_sb_iterator(sb_iterator_t * it)
{
    it->curr_fs = fsl_head;
    it->curr_sb = fsl_head->fs->sbl_head;
}

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
 * Get next pfs minor number.
 */
unsigned int fs_get_pfs_minor(void)
{
    unsigned int static pfs_minor = 0;
    unsigned int retval = pfs_minor;

    pfs_minor++;
    return retval;
}

int fs_fildes_set(file_t * fildes, vnode_t * vnode, int oflags)
{
    if (!fildes || !vnode)
        return -1;

    mtx_init(&fildes->lock, MTX_DEF | MTX_SPIN);
    fildes->vnode = vnode;
    fildes->oflags = oflags;
    fildes->refcount = 1;

    return 0;
}

static int chkperm_cproc(struct stat * stat, int oflags)
{
    gid_t euid = curproc->euid;
    uid_t egid = curproc->egid;

    oflags &= O_ACCMODE;

    if (oflags & O_RDONLY) {
        mode_t req_perm = 0;

        if (stat->st_uid == euid)
            req_perm |= S_IRUSR;
        if (stat->st_gid == egid)
            req_perm |= S_IRGRP;
        req_perm |= S_IROTH;

        if (!(req_perm & stat->st_mode))
            return -EPERM;
    }

    if (oflags & O_WRONLY) {
        mode_t req_perm = 0;

        if (stat->st_uid == euid)
            req_perm |= S_IWUSR;
        if (stat->st_gid == egid)
            req_perm |= S_IWGRP;
       req_perm |= S_IWOTH;

       if (!(req_perm & stat->st_mode))
           return -EPERM;
    }

    if ((oflags & O_EXEC) || (S_ISDIR(stat->st_mode))) {
        mode_t req_perm = 0;

        if (stat->st_uid == euid)
            req_perm |= S_IXUSR;
        if (stat->st_gid == egid)
            req_perm |= S_IXGRP;
       req_perm |= S_IXOTH;

       if (!(req_perm & stat->st_mode))
           return -EPERM;
    }

    return 0;
}

int fs_fildes_create_cproc(vnode_t * vnode, int oflags)
{
    int retval;
    struct stat stat;
    file_t * new_fildes;
    files_t * files;

    files = curproc->files;
    if (curproc->euid == 0)
        goto perms_ok;

    retval = vnode->vnode_ops->stat(vnode, &stat);
    if (retval) {
        return retval;
    }

    /* Check if user perms gives access */
    if (chkperm_cproc(&stat, oflags))
        return -EPERM;

perms_ok:
    /* Check other oflags */
    if ((oflags & O_DIRECTORY) && (!S_ISDIR(vnode->vn_mode))) {
        return -ENOTDIR;
    }

    new_fildes = kcalloc(1, sizeof(file_t));
    if (!new_fildes) {
        return -ENOMEM;
    }

    for (int i = 0; i < files->count; i++) {
        if (!(files->fd[i])) {
            fs_fildes_set(files->fd[i], vnode, oflags);
            return 0;
        }
    }

    return -EMFILE;
}

void fs_fildes_ref(file_t * fildes, int count)
{
    int free = 0;

    mtx_spinlock(&fildes->lock);
    fildes->refcount += count;
    if (fildes->refcount <= 0) {
        free = 1;
    }
    mtx_unlock(&fildes->lock);

    /* The following is normally unsafe but in the case of file descriptors it
     * should be safe to assume that there is always only one process that wants
     * to free a filedes.
     */
    if (free)
        kfree(fildes);
}

/* TODO This function should not use set_errno(), errno should be only set in
 *      actual syscall related functions. */
ssize_t fs_write_cproc(int fildes, const void * buf, size_t nbyte,
        off_t * offset)
{
    proc_info_t * cpr = curproc;
    vnode_t * vnode;
    size_t retval = -1;

    /* Check that fildes exists. */
    if (fildes >= cpr->files->count && !(cpr->files->fd[fildes])) {
        set_errno(EBADF);
        goto out;
    }

    /* Check that the fildes is open for writing. */
    if (!(cpr->files->fd[fildes]->oflags & O_WRONLY)) {
        set_errno(EBADF);
        goto out;
    }

    vnode = cpr->files->fd[fildes]->vnode;
    /* Check if there is a vnode linked with the file and it can be written. */
    if (!vnode || !(vnode->vnode_ops) || !(vnode->vnode_ops->write)) {
        set_errno(EBADF);
        goto out;
    }

    retval = vnode->vnode_ops->write(vnode, offset, buf, nbyte);
    if (retval < nbyte) /* TODO How to determine other errno types? */
        set_errno(ENOSPC);
out:
    return retval;
}
