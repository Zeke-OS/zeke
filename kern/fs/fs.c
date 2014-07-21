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

static int parse_filepath(const char * pathname, char ** path, char ** name);

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
     * Start looking up for a vnode.
     * We don't care if root is not a directory because lookup will spot it
     * anyway.
     */
    *result = root;
    do {
        vnode_t * vnode;

        if (!strcmp(nodename, "."))
            continue;

again:  /* Get vnode by name in this dir. */
        retval = (*result)->vnode_ops->lookup(*result, nodename,
                strlenn(nodename, FS_FILENAME_MAX) + 1, &vnode);
        if (retval) {
            goto out;
        }

        if (!strcmp(nodename, "..") && (vnode->vn_prev_mountpoint != vnode)) {
            /* Get prev dir of prev fs sb from mount point. */
            while (vnode->vn_prev_mountpoint != vnode) {
                /* We loop here to get to the first file system mounted on this
                 * mountpoint.
                 */
                vnode = vnode->vn_prev_mountpoint;
            }
            *result = vnode;
            goto again; /* Start from begining to actually get to the prev dir.
                         */
        } else {
            /* TODO - soft links support
             *      - if O_NOFOLLOW we should fail on soft link and return
             *        (-ELOOP)
             */

            /* Go to the last mountpoint. */
            while (vnode != vnode->vn_mountpoint) {
                vnode = vnode->vn_mountpoint;
            }
            *result = vnode;
        }
#if configDEBUG != 0
        if (*result == 0)
            panic("vfs is in inconsistent state");
#endif
    } while ((nodename = kstrtok(0, PATH_DELIMS, &lasts)));

    if ((oflags & O_DIRECTORY) && !S_ISDIR((*result)->vn_mode)) {
        retval = -ENOTDIR;
        goto out;
    }

out:
    kfree(path);
    return retval;
}

int fs_namei_proc(vnode_t ** result, const char * path)
{
    vnode_t * start;
    int oflags = 0;

    /* TODO This should return -EACCES if last char is '.' */

    if (path[0] == '\0')
        return -EINVAL;

    if (path[0] == '/') {
        path++;
        start = curproc->croot;
    } else {
        start = curproc->cwd;
    }

    if (path[strlenn(path, PATH_MAX)] == '/') {
        oflags = O_DIRECTORY;
    }

    return lookup_vnode(result, start, path, oflags);
}

int fs_mount(vnode_t * target, const char * source, const char * fsname,
        uint32_t flags, const char * parm, int parm_len)
{
    fs_t * fs = 0;
    struct fs_superblock * sb;
    int err;

    if (fsname) {
        fs = fs_by_name(fsname);
    } else {
        /* TODO Try to determine type of the fs */
    }
    if (!fs)
        return -ENOTSUP; /* fs doesn't exist. */

    err = fs->mount(source, flags, parm, parm_len, &sb);
    if (err)
        return err;

    sb->mountpoint = target;
    sb->root->vn_prev_mountpoint = target->vn_mountpoint;
    target->vn_mountpoint = sb->root;

    /* TODO inherit perms */

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
        while (1) {
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
    static unsigned int pfs_minor = 0;
    unsigned int retval = pfs_minor;

    pfs_minor++;
    return retval;
}

int chkperm_cproc(struct stat * stat, int oflags)
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

int chkperm_vnode_cproc(vnode_t * vnode, int oflags)
{
    struct stat stat;
    int err;

    err = vnode->vnode_ops->stat(vnode, &stat);
    if (err)
        return err;

    err = chkperm_cproc(&stat, oflags);

    return err;
}

int fs_fildes_set(file_t * fildes, vnode_t * vnode, int oflags)
{
    if (!(fildes && vnode))
        return -1;

    mtx_init(&fildes->lock, MTX_DEF | MTX_SPIN);
    fildes->vnode = vnode;
    fildes->oflags = oflags;
    fildes->refcount = 1;

    return 0;
}

int fs_fildes_create_cproc(vnode_t * vnode, int oflags)
{
    file_t * new_fildes;
    int err;

    if (!vnode)
        return -EINVAL;

    if (curproc->euid == 0)
        goto perms_ok;

    /* Check if user perms gives access */
    err = chkperm_vnode_cproc(vnode, oflags);
    if (err)
        return err;

perms_ok:
    /* Check other oflags */
    if ((oflags & O_DIRECTORY) && (!S_ISDIR(vnode->vn_mode)))
        return -ENOTDIR;

    new_fildes = kcalloc(1, sizeof(file_t));
    if (!new_fildes) {
        return -ENOMEM;
    }

    if (S_ISDIR(vnode->vn_mode))
        new_fildes->seek_pos = DSEEKPOS_MAGIC;

    int fd = fs_fildes_cproc_next(new_fildes, 0);
    if (fd < 0) {
        kfree(new_fildes);
        return fd;
    }
    fs_fildes_set(curproc->files->fd[fd], vnode, oflags);

    return fd;
}

int fs_fildes_cproc_next(file_t * new_file, int start)
{
    files_t * files = curproc->files;
#if 0
    int new_count;
#endif

    if (!new_file)
        return -EBADF;

    if (start > files->count - 1)
        return -EMFILE;
#if 0
retry:
#endif
    for (int i = start; i < files->count; i++) {
        if (!(files->fd[i])) {
            curproc->files->fd[i] = new_file;
            return i;
        }
    }

    /* TODO Until we have a good idea how to handle concurrency on this it's
     * better to follow a static limit. */
#if 0
    /* Extend fd array */
    new_count = files->count + files->count / 2;
    files = krealloc(files, SIZEOF_FILES(new_count));
    if (files) {
        for (int i = files->count; i < new_count; i++)
            files->fd[i] = NULL;

        files->count = new_count;
        curproc->files = files;

        goto retry;
    }
#endif

    return -ENFILE;
}

file_t * fs_fildes_ref(files_t * files, int fd, int count)
{
    file_t * fildes;
    int free = 0;

    if (!files || (fd >= files->count))
        return 0;

    fildes = files->fd[fd];
    if (!fildes)
        return 0;

    mtx_spinlock(&fildes->lock);
    fildes->refcount += count;
    if (fildes->refcount <= 0) {
        free = 1;
    }
    mtx_unlock(&fildes->lock);

    /* The following is normally unsafe but in the case of file descriptors it
     * should be safe to assume that there is always only one process that wants
     * to free a filedes concurrently (the owener itself).
     */
    if (free) {
        kfree(fildes);
        files->fd[fd] = 0;
        return 0;
    }

    return fildes;
}

int fs_fildes_close_cproc(int fildes)
{
    if (!fs_fildes_ref(curproc->files, fildes, 0)) {
        return -EBADF;
    }

    fs_fildes_ref(curproc->files, fildes, -1);
    curproc->files->fd[fildes] = NULL;

    return 0;
}

/**
 * @param oper  is the file operation, either O_RDONLY or O_WRONLY.
 */
ssize_t fs_readwrite_cproc(int fildes, void * buf, size_t nbyte, int oper)
{
    vnode_t * vnode;
    file_t * file;
    ssize_t retval = -1;

    file = fs_fildes_ref(curproc->files, fildes, 1);
    if (!file)
        return -EBADF;
    vnode = file->vnode;

    /* Check that file is opened for correct mode, the vnode exist and we have
     * ops struct for the vnode. */
    if (!(file->oflags & oper) || !vnode || !(vnode->vnode_ops)) {
        retval = -EBADF;
        goto out;
    }

    if (((oper & O_ACCMODE) == O_RDONLY) && !(vnode->vnode_ops->read)) {
        retval = -EOPNOTSUPP;
        goto out;
    } else if (((oper & O_ACCMODE) == O_WRONLY) && !(vnode->vnode_ops->write)) {
        retval = -EOPNOTSUPP;
        goto out;
    } else if ((oper & O_ACCMODE) == (O_RDONLY | O_WRONLY)) {
        /* Invalid operation code */
        retval = -ENOTSUP;
        goto out;
    }

    if (oper & O_RDONLY) {
        retval = vnode->vnode_ops->read(file, buf, nbyte);
    } else {
        retval = vnode->vnode_ops->write(file, buf, nbyte);
        if (retval == 0)
            retval = -EIO;
    }
    if (retval > 0)
        file->seek_pos += retval;

out:
    fs_fildes_ref(curproc->files, fildes, -1);
    return retval;
}

/**
 * Parse path and file name from a complete path.
 * <path>/<name>
 * @param pathname  is a complete path to a file or directory.
 * @param path[out] is the expected dirtory part of the path.
 * @param name[out] is the file or directory name parsed from the full path.
 * @return 0 if succeed; Otherwise a negative errno is returned.
 */
static int parse_filepath(const char * pathname, char ** path, char ** name)
{
    char * path_act;
    char * fname;
    size_t i = strlenn(pathname, PATH_MAX);

    path_act = kmalloc(PATH_MAX);
    if (!path_act) {
        return -ENOMEM;
    }

    fname = kmalloc(FS_FILENAME_MAX);
    if (!fname) {
        kfree(path_act);
        return -ENOMEM;
    }

    strncpy(path_act, pathname, PATH_MAX);
    while (path_act[i] != '/') {
        path_act[i--] = '\0';
        if ((i == 0) &&
            (!(path_act[0] == '/') ||
             !(path_act[0] == '.' && path_act[1] == '/'))) {
            path_act[0] = '.';
            path_act[1] = '/';
            path_act[2] = '\0';
            i--; /* little trick */
            break;
        }
    }

    for (int j = 0; j < FS_FILENAME_MAX;) {
        i++;
        if (pathname[i] == '/')
            continue;

        fname[j] = pathname[i];
        if (fname[j] == '\0')
            break;
        j++;
    }

    *path = path_act;
    *name = fname;

    return 0;
}

int fs_creat_cproc(const char * pathname, mode_t mode, vnode_t ** result)
{
    char * path = 0;
    char * name = 0;
    vnode_t * dir;
    int retval = 0;

    retval = fs_namei_proc(&dir, pathname);
    if (retval == 0) {
        retval = -EEXIST;
        goto out;
    } else if (retval != -ENOENT) {
        goto out;
    }

    retval = parse_filepath(pathname, &path, &name);
    if (retval)
        goto out;

    if (fs_namei_proc(&dir, path)) {
        retval = -ENOENT;
        goto out;
    }

    /* We know that the returned vnode is a dir so we can just call mknod() */
    *result = 0;
    mode &= ~S_IFMT; /* Filter out file type bits */
    retval = dir->vnode_ops->create(dir, name, FS_FILENAME_MAX, mode, result);

out:
    kfree(path);
    kfree(name);
    return retval;
}

int fs_link_curproc(const char * path1, size_t path1_len,
        const char * path2, size_t path2_len)
{
    char * dirpath = 0;
    char * filename = 0;
    vnode_t * dir;
    vnode_t * fnode;
    int err;

    err = fs_namei_proc(&fnode, path1);
    if (err)
        return err;

    err = fs_namei_proc(&fnode, path2);
    if (err == 0) {
        err = -EEXIST;
        goto out;
    } else if (retval != -ENOENT) {
        goto out;
    }

    err = parse_filepath(path, &dirpath, &filename);
    if (err)
        goto out;

    /* Get the vnode of the containing directory */
    if (fs_namei_proc(&dir, dirpath)) {
        err = -ENOENT;
        goto out;
    }

    /* TODO */

    /* Get the vnode of the target directory */

    /* Check write permission to the target directory and source vnode */

    //(*link)(vnode_t * dir, vnode_t * vnode, const char * name, size_t name_len)
}

int fs_unlink_curproc(const char * path, size_t path_len)
{
    char * dirpath = 0;
    char * filename = 0;
    struct stat stat;
    vnode_t * dir;
    vnode_t * fnode;
    int err;

    /* TODO Incorrect?? */
    err = fs_namei_proc(&fnode, path);
    if (err)
        return err;

    /* unlink() is prohibited on directories for non-root users. */
    err = fnode->vnode_ops->stat(fnode, &stat);
    if (err)
        goto out;
    if (S_ISDIR(stat.st_mode) && curproc->euid != 0) {
        err = -EPERM;
        goto out;
    }

    err = parse_filepath(path, &dirpath, &filename);
    if (err)
        goto out;

    /* Get the vnode of the containing directory */
    if (fs_namei_proc(&dir, dirpath)) {
        err = -ENOENT;
        goto out;
    }

    /* We need write access to the containing directory to allow removal of
     * a directory entry. */
    err = chkperm_vnode_cproc(dir, O_WRONLY);
    if (err) {
        if (err == -EPERM)
            err = -EACCES;
        goto out;
    }

    if (!dir->vnode_ops->unlink) {
        err = -EACCES;
        goto out;
    }

    err = dir->vnode_ops->unlink(dir, filename, path_len);

out:
    kfree(dirpath);
    kfree(filename);
    return err;
}

int fs_unlinkat_curproc(int fd, const char * path, int flag)
{
    /* TODO */
    return -ENOTSUP;
}

int fs_mkdir_curproc(const char * pathname, mode_t mode)
{
    char * path = 0;
    char * name = 0;
    vnode_t * dir;
    int retval = 0;

    if (pathname[0] == '\0') {
        retval = -EINVAL;
        goto out;
    }

    retval = fs_namei_proc(&dir, pathname);
    if (retval == 0) {
        retval = -EEXIST;
        goto out;
    } else if (retval != -ENOENT) {
        goto out;
    }

    retval = parse_filepath(pathname, &path, &name);
    if (retval)
        goto out;

    if (fs_namei_proc(&dir, path)) {
        retval = -ENOENT;
        goto out;
    }

    /* Check that we have a permission to write this dir. */
    retval = chkperm_vnode_cproc(dir, O_WRONLY);
    if (retval)
        goto out;

    mode &= ~S_IFMT; /* Filter out file type bits */
    retval = dir->vnode_ops->mkdir(dir, name, FS_FILENAME_MAX, mode);

    /* TODO Set owner */

out:
    kfree(path);
    kfree(name);
    return retval;
}

int fs_rmdir_curproc(const char * pathname)
{
    char * path = 0;
    char * name = 0;
    vnode_t * dir;
    int retval;

    retval = fs_namei_proc(&dir, pathname);
    if (retval) {
        goto out;
    }

    retval = parse_filepath(pathname, &path, &name);
    if (retval)
        goto out;

    if (fs_namei_proc(&dir, path)) {
        retval = -ENOENT;
        goto out;
    }

    /* Check that we have a permission to write this dir. */
    retval = chkperm_vnode_cproc(dir, O_WRONLY);
    if (retval)
        goto out;

    retval = dir->vnode_ops->rmdir(dir, name, FS_FILENAME_MAX);

out:
    kfree(path);
    kfree(name);
    return retval;
}
