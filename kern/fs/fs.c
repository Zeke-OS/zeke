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
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
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


static fs_t * find_fs(const char * fsname);
static ssize_t fs_write_cproc(int fildes, const void * buf, size_t nbyte,
        off_t * offset);


void fs_init(void) __attribute__((constructor));
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

/**
 * Lookup for vnode by path.
 * @param[out]  vnode   is the target where vnode struct is stored.
 * @param       str     is the path.
 * @return Returns zero if vnode was found or error code.
 */
int lookup_vnode(vnode_t ** vnode, const char * str)
{
    int retval = 0;

    /* TODO */

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

file_t * fs_fildes_create(vnode_t * vnode)
{
    file_t * fildes;

    if (!(fildes = kcalloc(1, sizeof(file_t))))
        return 0;

    mtx_init(&fildes->lock, MTX_DEF | MTX_SPIN);
    fildes->vnode = vnode;
    fildes->refcount = 1;

    return fildes;
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

/**
 * Walks the file system for a process and tries to locate and lock vnode
 * corresponding to a given path.
 */
struct vnode * fs_namei_proc(char * path)
{
    char * _path = kstrdup(path, 1024); /* TODO max ok? */
    char * lasts;
    char * name;
    vnode_t * cur = curproc->croot;
    vnode_t * res;

    if (!(name = kstrtok(_path, PATH_DELIMS, &lasts))) {
        kfree(_path);
        return 0;
    }

    do {
        if (cur->vnode_ops->lookup(cur, name, FS_FILENAME_MAX, &res)) {
            kfree(_path);
            return 0; /* vnode not found */
        }
        /* TODO Check if vnode is dir or symlink and there is yet path left.
         * - if file is a link to a next dir then continue by setting a new value to cur
         * - if file is not a link and there is still path left we'll exit by the next iteration
         * - decrement refcount of cur
         */
    } while ((name = kstrtok(0, PATH_DELIMS, &lasts)));
    kfree(_path);

    return res;
}

/**
 * Write to a open file of the current process.
 * Error code is written to the errno of the current thread.
 * @param fildes    is the file descriptor.
 * @param buf       is the buffer.
 * @param nbytes    is the amount of bytes to be writted.
 * @return  Return the number of bytes actually written to the file associated
 *          with fildes; Otherwise, -1 is returned and errno set to indicate the
 *          error.
 * @throws EBADF    The fildes argument is not a valid file descriptor open for
 *                  writing.
 * @throws ENOSPC   There was no free space remaining on the device containing
 *                  the file.
 */
static ssize_t fs_write_cproc(int fildes, const void * buf, size_t nbyte,
        off_t * offset)
{
    proc_info_t * cpr = (proc_info_t *)curproc;
    vnode_t * vnode;
    size_t retval = -1;

    /* Check that fildes exists */
    if (fildes >= cpr->files->count && !(cpr->files->fd[fildes])) {
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

/**
 * fs syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uintptr_t fs_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_FS_CREAT:
        set_errno(ENOSYS);
        return -1;

    case SYSCALL_FS_OPEN:
        set_errno(ENOSYS);
        return -2;

    case SYSCALL_FS_CLOSE:
        set_errno(ENOSYS);
        return -3;

    case SYSCALL_FS_READ:
        set_errno(ENOSYS);
        return -4;

    case SYSCALL_FS_WRITE:
    {
        struct _fs_write_args fswa;
        char * buf;
        uintptr_t retval;

        /* Args */
        if (!useracc(p, sizeof(struct _fs_write_args), VM_PROT_READ)) {
            /* No permission to read/write */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
        }
        copyin(p, &fswa, sizeof(struct _fs_write_args));

        /* Buffer */
        if (!useracc(fswa.buf, fswa.nbyte, VM_PROT_READ)) {
            /* No permission to read/write */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
        }
        buf = kmalloc(fswa.nbyte);
        copyin(fswa.buf, buf, fswa.nbyte);

        retval = (uintptr_t)fs_write_cproc(fswa.fildes, buf, fswa.nbyte,
                &(fswa.offset));
        kfree(buf);
        return retval;
    }

    case SYSCALL_FS_LSEEK:
        set_errno(ENOSYS);
        return -6;

    case SYSCALL_FS_DUP:
        set_errno(ENOSYS);
        return -7;

    case SYSCALL_FS_LINK:
        set_errno(ENOSYS);
        return -8;

    case SYSCALL_FS_UNLINK:
        set_errno(ENOSYS);
        return -9;

    case SYSCALL_FS_STAT:
        set_errno(ENOSYS);
        return -10;

    case SYSCALL_FS_FSTAT:
        set_errno(ENOSYS);
        return -11;

    case SYSCALL_FS_ACCESS:
        set_errno(ENOSYS);
        return -12;

    case SYSCALL_FS_CHMOD:
        set_errno(ENOSYS);
        return -13;

    case SYSCALL_FS_CHOWN:
        set_errno(ENOSYS);
        return -14;

    case SYSCALL_FS_UMASK:
        set_errno(ENOSYS);
        return -15;

    case SYSCALL_FS_IOCTL:
        set_errno(ENOSYS);
        return -16;

    case SYSCALL_FS_MOUNT:
        set_errno(ENOSYS);
        return -17;

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
