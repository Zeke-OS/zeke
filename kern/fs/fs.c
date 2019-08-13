/**
 *******************************************************************************
 * @file    fs.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <termios.h>
#include <unistd.h>
#include <buf.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/mbr.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <ksignal.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <thread.h>
#include <vm/vm.h>

/*
 * File system global locking.
 */
static mtx_t fslock = MTX_INITIALIZER(MTX_TYPE_SPIN, 0);
#define FS_LOCK()       mtx_lock(&fslock)
#define FS_UNLOCK()     mtx_unlock(&fslock)
#define FS_TESTLOCK()   mtx_test(&fslock)

SYSCTL_NODE(, CTL_VFS, vfs, CTLFLAG_RW, 0,
            "File system");

SYSCTL_DECL(_vfs_limits);
SYSCTL_NODE(_vfs, OID_AUTO, limits, CTLFLAG_RD, 0,
            "File system limits and information");
SYSCTL_NODE(_vfs, OID_AUTO, generic, CTLFLAG_RD, 0,
            "Generic information and config knobs");

SYSCTL_INT(_vfs_limits, OID_AUTO, name_max, CTLFLAG_RD, NULL, NAME_MAX,
           "Limit for the length of a file name component.");
SYSCTL_INT(_vfs_limits, OID_AUTO, path_max, CTLFLAG_RD, NULL, PATH_MAX,
           "Limit for for length of an entire file name.");

/**
 * Linked list of registered file systems.
 */
static SLIST_HEAD(fs_list, fs) fs_list_head =
    SLIST_HEAD_INITIALIZER(fs_list_head);


int fs_register(fs_t * fs)
{
    KERROR_DBG("%s(fs \"%s\")\n", __func__, fs->fsname);

    FS_LOCK();
    mtx_lock(&fs->fs_giant);
    SLIST_INSERT_HEAD(&fs_list_head, fs, _fs_list);
    mtx_unlock(&fs->fs_giant);
    FS_UNLOCK();

    return 0;
}

fs_t * fs_by_name(const char * fsname)
{
    struct fs * fs;

    KASSERT(fsname != NULL, "fsname should be set");

    FS_LOCK();
    SLIST_FOREACH(fs, &fs_list_head, _fs_list) {
        if (strcmp(fs->fsname, fsname) == 0) {
            FS_UNLOCK();
            return fs;
        }
    }
    FS_UNLOCK();

    return NULL;
}

fs_t * fs_iterate(fs_t * fsp)
{
    FS_LOCK();
    if (SLIST_EMPTY(&fs_list_head)) {
        FS_UNLOCK();
        return NULL;
    }

    if (!fsp)
        fsp = SLIST_FIRST(&fs_list_head);
    else
        fsp = SLIST_NEXT(fsp, _fs_list);
    FS_UNLOCK();

    return fsp;
}

/**
 * Get base vnode of a mountpoint.
 * If vn is not a mountpoint nothing is changed;
 * Otherwise the first vnode in a mount stack is returned. The first vnode
 * is the base mountpoint vnode.
 */
static void get_base_vnode(vnode_t ** vnp)
{
    vnode_t * vnode = *vnp;

    vrele(vnode);

    while (vnode->vn_prev_mountpoint != vnode) {
        vnode_t * tmp;

        /*
         * We loop here to get to the first file system mounted on this
         * mountpoint.
         */
        tmp = vnode->vn_prev_mountpoint;
        KASSERT(tmp != NULL, "prev_mountpoint should be always valid");
        vnode = tmp;
    }

    vref(vnode);
    *vnp = vnode;
}

/**
 * Get the top root vnode of a mountpoint.
 * If there is no mounts on this vnode nothing is changed;
 * Otherwise the top most root vnode on a mount stack is returned.
 */
static void get_top_vnode(vnode_t ** vnp)
{
    vnode_t * vnode = *vnp;

    vrele(vnode);

    while (vnode->vn_next_mountpoint != vnode) {
        vnode_t * tmp;

        tmp = vnode->vn_next_mountpoint;
        KASSERT(tmp != NULL, "next_mountpoint should be always valid");
        vnode = tmp;
    }

    vref(vnode);
    *vnp = vnode;
}

int fs_mount(vnode_t * target, const char * source, const char * fsname,
             uint32_t flags, const char * parm, int parm_len)
{
    fs_t * fs = NULL;
    struct fs_superblock * sb;
    vnode_t * root;
    istate_t s;
    int err;

     KERROR_DBG("%s(target \"%p\", source \"%s\", fsname \"%s\", "
                "flags %x, parm \"%s\", parm_len %d)\n",
                __func__, target, source, fsname, flags, parm, parm_len);
    KASSERT(target, "target must be set");

    if (fsname) {
        fs = fs_by_name(fsname);
    } else {
        /* TODO Try to determine the type of the fs */
    }
    if (!fs)
        return -ENOTSUP; /* fs doesn't exist. */

    KERROR_DBG("Found fs: %s\n", fsname);

    if (!fs->mount) {
        KERROR_DBG("fs %s isn't mountable\n", fsname);

        return -ENOTSUP; /* Not a mountable file system. */
    }

    err = fs->mount(fs, source, flags, parm, parm_len, &sb);
    if (err)
        return err;
    KASSERT((uintptr_t)sb > configKERNEL_START, "sb isn't a stack address");
    KASSERT(sb->root, "sb->root must be set");

    vref(target);
    get_top_vnode(&target);
    root = sb->root;

    /* We test for int mask to avoid blocking in kinit. */
    s = get_interrupt_state();
    if (!(s & PSR_INT_MASK)) {
        VN_LOCK(root);
        VN_LOCK(target);
    }

    sb->mountpoint = target;
    target->vn_next_mountpoint = root;
    root->vn_prev_mountpoint = target;
    root->vn_next_mountpoint = root;

    if (!(s & PSR_INT_MASK)) {
        VN_UNLOCK(target);
        VN_UNLOCK(root);
    }

    /* TODO inherit permissions */

    KERROR_DBG("Mount OK\n");

    return 0;
}

int fs_umount(struct fs_superblock * sb)
{
    vnode_t * root;
    vnode_t * prev;
    vnode_t * next;

    KERROR_DBG("%s(sb:%p)\n", __func__, sb);
    KASSERT(sb, "sb is set");
    KASSERT(sb->fs && sb->root && sb->root->vn_prev_mountpoint &&
            sb->root->vn_prev_mountpoint->vn_next_mountpoint,
            "Sanity check");

    if (!sb->umount)
        return -ENOTSUP;

    root = sb->root;
    if (root->vn_prev_mountpoint == root)
        return -EINVAL; /* Can't unmount rootfs */

    /*
     * Reverse the mount process to unmount.
     */
    VN_LOCK(root);
    prev = root->vn_prev_mountpoint;
    next = root->vn_next_mountpoint;
    KASSERT(root != prev, "FS can't handle umount if root == prev");

    VN_LOCK(prev);
    if (next && next != root) {
        VN_LOCK(next);
        prev->vn_next_mountpoint = root->vn_next_mountpoint;
        next->vn_prev_mountpoint = prev;
        VN_UNLOCK(next);
    } else {
        prev->vn_next_mountpoint = prev;
    }
    VN_UNLOCK(prev);
    root->vn_next_mountpoint = root;
    root->vn_prev_mountpoint = root;
    VN_UNLOCK(root);

    return sb->umount(sb);
}

int lookup_vnode(vnode_t ** result, vnode_t * root, const char * str, int oflags)
{
    char * path;
    char * nodename;
    char * lasts;
    int retval = 0;

    KERROR_DBG("%s(result %p, root %pV, str \"%s\", oflags %x)\n",
               __func__, result, root, str, oflags);

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
    vref(root);
    *result = root;
    do {
        vnode_t * vnode;

        if (!strcmp(nodename, "."))
            continue;

again:  /* Get vnode by name in this dir. */
        vnode = NULL;
        retval = (*result)->vnode_ops->lookup(*result, nodename, &vnode);
        vrele(*result);
        KASSERT((retval == 0 && vnode != NULL) || (retval != 0),
                "vnode should be valid if !retval");
        if (retval != 0 && retval != -EDOM) {
            goto out;
        } else if (!vnode) {
            retval = -ENOENT;
            goto out;
        }

        /*
         * If retval == -EDOM the result and vnode are equal so we are at
         * the root of the physical file system and trying to exit its
         * mountpoint, this requires some additional processing as follows.
         */
        if (retval == -EDOM && !strcmp(nodename, "..") &&
            vnode->vn_prev_mountpoint != vnode) {
            /* Get prev dir of prev fs sb from mount point. */
            vref(vnode);
            get_base_vnode(&vnode);
            *result = vnode;

            /* Restart from the begining to get the actual prev dir. */
            goto again;
        } else {
            /*
             * TODO soft links and O_NOFOLLOW for lookup_vnode()
             * - soft links support
             * - if O_NOFOLLOW we should fail on soft link and return
             *   (-ELOOP)
             */

            /* Go to the last mountpoint. */
            get_top_vnode(&vnode);
            *result = vnode;
        }
        retval = 0;

        KASSERT(*result != NULL, "vfs is in inconsistent state");
    } while ((nodename = kstrtok(0, PATH_DELIMS, &lasts)));

    if ((oflags & O_DIRECTORY) && !S_ISDIR((*result)->vn_mode)) {
        vrele(*result);
        *result = NULL;
        retval = -ENOTDIR;
        goto out;
    }

out:
    KERROR_DBG("%s: result %pV\n", __func__, (result) ? *result : NULL);

    if (retval && retval != -EDOM) {
        *result = NULL;
    }
    kfree(path);
    return retval;
}

int fs_namei_proc(vnode_t ** result, int fd, const char * path, int atflags)
{
    vnode_t * start;
    int oflags = atflags & AT_SYMLINK_NOFOLLOW;
    int retval;

    KERROR_DBG("%s(result %p, fd %d, path \"%s\", atflags %x)\n",
               __func__, result, fd, path, atflags);

    if (path[0] == '\0')
        return -EINVAL;

    if (path[0] == '/') { /* Absolute path */
        path++;
        start = curproc->croot;

        /* Short circuit: Caller requested '/' */
        if (path[0] == '\0') {
            *result = start;
            vref(start);

            return 0;
        }
    } else if (atflags & AT_FDARG && fd != AT_FDCWD) { /* AT_FDARG */
        file_t * file;

        file = fs_fildes_ref(curproc->files, fd, 1);
        if (!file)
            return -EBADF;
        start = file->vnode;

        /* Short circuit: Caller requested '.' and AT_FDARG */
        if (path[0] == '.' && path[1] == '\0') {
            *result = start;
            vref(start);
            fs_fildes_ref(curproc->files, fd, -1);

            return 0;
        }
    } else { /* Implicit AT_FDCWD */
        start = curproc->cwd;
    }
    KASSERT(start, "Start is set");
    KASSERT(start->vnode_ops, "vnode ops of start is valid");

    if (path[strlenn(path, PATH_MAX)] == '/') {
        oflags |= O_DIRECTORY;
    }

    retval = lookup_vnode(result, start, path, oflags);

    if (atflags & AT_FDARG)
        fs_fildes_ref(curproc->files, fd, -1);

    return retval;
}

int chkperm(struct stat * stat, const struct cred * cred, int oflags)
{
    const uid_t euid = curproc->cred.euid;
    const gid_t egid = curproc->cred.egid;
    const int is_grp_member = stat->st_gid == egid ||
                              priv_grp_is_member(cred, stat->st_gid);

    oflags &= O_ACCMODE;

    if (oflags & R_OK) {
        mode_t req_perm = 0;

        if (stat->st_uid == euid)
            req_perm |= S_IRUSR;
        if (is_grp_member)
            req_perm |= S_IRGRP;
        req_perm |= S_IROTH;

        if (!(req_perm & stat->st_mode) ||
            priv_check(cred, PRIV_VFS_READ)) {
            return -EPERM;
        }
    }

    if (oflags & W_OK) {
        mode_t req_perm = 0;

        if (stat->st_uid == euid)
            req_perm |= S_IWUSR;
        if (is_grp_member)
            req_perm |= S_IWGRP;
        req_perm |= S_IWOTH;

        if (!(req_perm & stat->st_mode) ||
            priv_check(cred, PRIV_VFS_WRITE)) {
            return -EPERM;
        }
    }

    if ((oflags & X_OK) || (S_ISDIR(stat->st_mode))) {
        mode_t req_perm = 0;

        if (stat->st_uid == euid)
            req_perm |= S_IXUSR;
        if (is_grp_member)
            req_perm |= S_IXGRP;
        req_perm |= S_IXOTH;

        if (!(req_perm & stat->st_mode) ||
            priv_check(cred, PRIV_VFS_EXEC)) {
            return -EPERM;
        }
    }

    return 0;
}

int chkperm_curproc(struct stat * stat, int oflags)
{
    return chkperm(stat, &curproc->cred, oflags);
}

int chkperm_vnode_curproc(vnode_t * vnode, int oflags)
{
    return chkperm_vnode(vnode, &curproc->cred, oflags);
}

int chkperm_vnode(vnode_t * vnode, struct cred * cred, int oflags)
{
    struct stat stat;
    int err;

    KASSERT(vnode != NULL, "vnode should be set\n");

    err = vnode->vnode_ops->stat(vnode, &stat);
    if (err)
        return err;

    err = chkperm(&stat, cred, oflags);

    return err;
}

/**
 * Automatically called destructor for file descriptors.
 */
static void fs_fildes_dtor(struct kobj * obj)
{
    file_t * file = containerof(obj, struct file, f_obj);
    vnode_t * vn = file->vnode;

    KERROR_DBG("%s(%p), vnode %pV\n", __func__, obj, vn);

    if (file->oflags & O_KFREEABLE)
        kfree(file);
    vrele(vn);
}

int fs_fildes_set(file_t * fildes, vnode_t * vnode, int oflags)
{
    if (!(fildes && vnode))
        return -1;

    fildes->vnode = vnode;
    fildes->oflags = oflags;
    kobj_init(&fildes->f_obj, fs_fildes_dtor);

    return 0;
}

int fs_fildes_create_curproc(vnode_t * vnode, int oflags)
{
    file_t * new_fildes;
    int is_dir;
    struct stat stat_buf;
    int retval;

    KERROR_DBG("%s(vnode %pV, oflags %x)\n", __func__, vnode, oflags);

    if (!vnode)
        return -EINVAL;
    vref(vnode);

    is_dir = S_ISDIR(vnode->vn_mode);

    if (priv_check(&curproc->cred, PRIV_VFS_ADMIN) == 0)
        goto perms_ok;

    /* Check if user perms gives access */
    retval = chkperm_vnode_curproc(vnode, oflags);
    if (retval)
        goto out;

perms_ok:
    /* Check other oflags */
    if ((oflags & O_DIRECTORY) && !is_dir) {
        retval = -ENOTDIR;
        goto out;
    }

    retval = vnode->vnode_ops->stat(vnode, &stat_buf);
    if (retval) {
        goto out;
    }

    /*
     * File opened event call, if this fails we must cancel the
     * file open procedure.
     */
    retval = vnode->vnode_ops->event_vnode_opened(curproc, vnode);
    if (retval < 0)
        goto out;

    new_fildes = kzalloc(sizeof(file_t));
    if (!new_fildes) {
        retval = -ENOMEM;
        goto out;
    }

    if (S_ISDIR(vnode->vn_mode))
        new_fildes->seek_pos = DIRENT_SEEK_START;

    int fd = fs_fildes_curproc_next(new_fildes, 0);
    if (fd < 0) {
        kfree(new_fildes);
        retval = fd;
        goto out;
    }
    fs_fildes_set(curproc->files->fd[fd], vnode, oflags);
    new_fildes->oflags |= O_KFREEABLE;

    /*
     * Set O_EXEC_ALTPCAP to mark the file is allowed to alter the bounding
     * capabilities set on exec().
     * TODO UF_SYSTEM is FAT specific, perhaps in the future there should be an
     * unified capabilities interface in the VFS stat().
     * TODO How to prevent a regular user from exploiting this by mounting an
     * sd? Do we need a uid=0 check for the superblock?
     */
    if (!is_dir && stat_buf.st_flags & UF_SYSTEM) {
        new_fildes->oflags |= O_EXEC_ALTPCAP;
    }

    /*
     * File descriptor ready, make an event call to the fs.
     */
    vnode->vnode_ops->event_fd_created(curproc, new_fildes);

    retval = fd;
out:
    if (retval < 0)
        vrele(vnode);
    return retval;
}

int fs_fildes_curproc_next(file_t * new_file, int start)
{
    files_t * files = curproc->files;

    if (!new_file)
        return -EBADF;

    if (start > files->count - 1)
        return -EMFILE;

    for (int i = start; i < files->count; i++) {
        if (!(files->fd[i])) {
            curproc->files->fd[i] = new_file;
            return i;
        }
    }

    return -ENFILE;
}

/**
 * Check if the given fd number is in the valid range.
 */
static int fs_fildes_is_in_range(files_t * files, int fd)
{
    return (fd >= 0 && fd < files->count);
}

file_t * fs_fildes_ref(files_t * files, int fd, int count)
{
    file_t * file;
    int orig_refcount;

    KASSERT(files != NULL, "files should be set");

    if (!fs_fildes_is_in_range(files, fd))
        return NULL;

    file = files->fd[fd];
    if (!file)
        return NULL;

    orig_refcount = kobj_refcnt(&file->f_obj);
    if (orig_refcount <= 0) {
        files->fd[fd] = NULL;
        file = NULL;
    } else if (count > 0) {
        if (kobj_ref_v(&file->f_obj, count)) {
            files->fd[fd] = NULL;
            file = NULL;
        }
    } else if (count < 0) {
        count = min(orig_refcount, -count);

        kobj_unref_p(&file->f_obj, count);
        if (count == orig_refcount) {
            files->fd[fd] = NULL;
            file = NULL;
        }
    }

    return file;
}

int fs_fildes_close(struct proc_info * p, int fildes)
{
    file_t * file = fs_fildes_ref(p->files, fildes, 1);
    if (!file)
        return -EBADF;

    file->vnode->vnode_ops->event_fd_closed(p, file);

    fs_fildes_ref(p->files, fildes, -2);

    /* File pointer is set to NULL for closed files regardless of refcount. */
    p->files->fd[fildes] = NULL;

    return 0;
}

void fs_fildes_close_all(struct proc_info * p, int fildes_begin)
{
    int start, fdstop, i;

    KASSERT(p->files, "files is expected to always exist");

    start = p->files->count - 1;
    fdstop = fildes_begin;
    if (!fs_fildes_is_in_range(p->files, fdstop))
        return;

    for (i = start; i > fdstop; i--) {
        fs_fildes_close(p, i);
    }
}

void fs_fildes_close_exec(struct proc_info * p)
{
    int end, i;

    KASSERT(p->files, "files is expected to always exist");

    end =  p->files->count;
    for (i = 0; i < end; i++) {
        file_t * file = p->files->fd[i];

        if (file && file->oflags & O_CLOEXEC) {
            KERROR_DBG("%s(%d): Close O_CLOEXEC fd %d\n", __func__, p->pid, i);

            fs_fildes_close(p, i);
        }
    }
}

int fs_fildes_isatty(int fd)
{
    file_t * file;
    struct termios termios;
    int err = 0;

    file = fs_fildes_ref(curproc->files, fd, 1);
    if (!file)
        return -ENOTTY;

    KASSERT(file->vnode, "vnode is set");
    err = file->vnode->vnode_ops->ioctl(file, IOCTL_GTERMIOS,
                                        &termios, sizeof(termios));
    if (err) {
        err = -ENOTTY;
        goto out;
    }
    /* Otherwise the vnode must be a TTY. */

out:
    fs_fildes_ref(curproc->files, fd, -1);
    return err;
}

files_t * fs_alloc_files(size_t nr_files, mode_t umask)
{
    files_t * files;

    files = kzalloc(SIZEOF_FILES(nr_files));
    if (!files)
        return NULL;

    files->count = nr_files;
    files->umask = umask;

    return files;
}

/**
 * Get directory vnode of a target file and the actual directory entry name.
 * @param[in]   pathname    is a path to the target.
 * @param[out]  dir         is the directory containing the entry.
 * @param[out]  filename    is the actual file name / directory entry name.
 * @param[in]   flag        0 = file should exist; O_CREAT = file should not
 *                          exist.
 */
static int getvndir(const char * pathname, vnode_t ** dir, char ** filename,
                    int flag)
{
    vnode_t * vn_file;
    kmalloc_autofree char * path = NULL;
    kmalloc_autofree char * name = NULL;
    int err;

    if (pathname[0] == '\0') {
        return -EINVAL;
    }

    err = fs_namei_proc(&vn_file, -1, pathname, AT_FDCWD);
    if (err == 0)
        vrele(vn_file);
    if (flag & O_CREAT) { /* File should not exist */
        if (err == 0) {
            return -EEXIST;
        } else if (err != -ENOENT) {
            return err;
        }
    } else if (err) { /* File should exist */
        return err;
    }

    err = parsenames(pathname, &path, &name);
    if (err) {
        return err;
    }

    kpalloc(name); /* Incr ref count */
    *filename = name;

    return fs_namei_proc(dir, -1, path, AT_FDCWD);
}

int fs_creat_curproc(const char * pathname, mode_t mode, vnode_t ** result)
{
    kmalloc_autofree char * name = NULL;
    vnode_autorele vnode_t * dir = NULL;
    int retval = 0;

    KERROR_DBG("%s(pathname \"%s\", mode %u)\n",
               __func__, pathname, (unsigned)mode);

    retval = getvndir(pathname, &dir, &name, O_CREAT);
    if (retval)
        return retval;

    /* We know that the returned vnode is a dir so we can just call mknod() */
    *result = NULL;
    mode &= ~S_IFMT; /* Filter out file type bits */
    mode &= ~curproc->files->umask;
    retval = dir->vnode_ops->create(dir, name, mode, result);

    KERROR_DBG("%s() result: %p\n", __func__, *result);

    return retval;
}

int fs_link_curproc(int fd1, const char * path1,
                    int fd2, const char * path2,
                    int atflags)
{
    kmalloc_autofree char * targetname = NULL;
    vnode_autorele vnode_t * vn_src = NULL;
    vnode_autorele vnode_t * vndir_dst = NULL;
    int err;

    /* Get the source vnode */
    err = fs_namei_proc(&vn_src, fd1, path1, AT_FDARG);
    if (err) {
        return err;
    }

    /* Check write access to the source vnode */
    err = chkperm_vnode_curproc(vn_src, O_WRONLY);
    if (err) {
        return err;
    }

     /* TODO fd2 and atflags support for link */
    if (fd2 != AT_FDCWD) {
        return -ENOTSUP;
    }

    /* Get vnode of the target directory */
    err = getvndir(path2, &vndir_dst, &targetname, O_CREAT);
    if (err) {
        return err;
    }

    if (vn_src->sb->vdev_id != vndir_dst->sb->vdev_id) {
        /*
         * The link named by path2 and the file named by path1 are
         * on different file systems.
         */
        return -EXDEV;
    }

    err = chkperm_vnode_curproc(vndir_dst, O_WRONLY);
    if (err) {
        return err;
    }

    return vndir_dst->vnode_ops->link(vndir_dst, vn_src, targetname);
}

int fs_unlink_curproc(int fd, const char * path, int atflags)
{
    kmalloc_autofree char * dirpath = NULL;
    kmalloc_autofree char * filename = NULL;
    struct stat stat;
    vnode_autorele vnode_t * dir = NULL;
    int err;

    {
        vnode_t * fnode;

        err = fs_namei_proc(&fnode, fd, path, atflags);
        if (err) {
            return err;
        }

        /* unlink() is prohibited on directories for non-root users. */
        err = fnode->vnode_ops->stat(fnode, &stat);
        vrele(fnode);
        if (err) {
            return err;
        }
        if (S_ISDIR(stat.st_mode) && curproc->cred.euid != 0) {
            return -EPERM;
        }
    }

    err = parsenames(path, &dirpath, &filename);
    if (err) {
        return err;
    }

    /* Get the vnode of the containing directory */
    if (fs_namei_proc(&dir, fd, dirpath, atflags)) {
        return -ENOENT;
    }

    /*
     * We need a write access to the containing directory to allow removal of
     * a directory entry.
     */
    err = chkperm_vnode_curproc(dir, O_WRONLY);
    if (err) {
        if (err == -EPERM)
            err = -EACCES;
        return err;
    }

    return dir->vnode_ops->unlink(dir, filename);
}

int fs_mkdir_curproc(const char * pathname, mode_t mode)
{
    kmalloc_autofree char * name = NULL;
    vnode_autorele vnode_t * dir = NULL;
    int err = 0;

    err = getvndir(pathname, &dir, &name, O_CREAT);
    if (err) {
        return err;
    }

    /* Check that we have a permission to write this dir. */
    err = chkperm_vnode_curproc(dir, O_WRONLY);
    if (err) {
        return err;
    }

    mode &= ~S_IFMT; /* Filter out file type bits */
    mode &= ~curproc->files->umask;
    return dir->vnode_ops->mkdir(dir, name, mode);
}

int fs_rmdir_curproc(const char * pathname)
{
    kmalloc_autofree char * name = NULL;
    vnode_autorele vnode_t * dir = NULL;
    int err;

    err = getvndir(pathname, &dir, &name, 0);
    if (err) {
        return err;
    }

    /* Removing dot or dotdot is not ok. */
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        return -EINVAL;
    }

    /* Check that we have a permission to write this dir. */
    err = chkperm_vnode_curproc(dir, O_WRONLY);
    if (err) {
        return err;
    }

    return dir->vnode_ops->rmdir(dir, name);
}

int fs_utimes_curproc(int fildes, const struct timespec times[2])
{
    vnode_t * vnode;
    file_t * file;
    int retval = 0;

    file = fs_fildes_ref(curproc->files, fildes, 1);
    if (!file)
        return -EBADF;
    vnode = file->vnode;

    if (!((file->oflags & O_WRONLY) || !chkperm_vnode_curproc(vnode, W_OK))) {
        retval = -EPERM;
        goto out;
    }

    retval = vnode->vnode_ops->utimes(vnode, times);

out:
    fs_fildes_ref(curproc->files, fildes, -1);

    return retval;
}

int fs_chmod_curproc(int fildes, mode_t mode)
{
    vnode_t * vnode;
    file_t * file;
    int retval = 0;

    file = fs_fildes_ref(curproc->files, fildes, 1);
    if (!file)
        return -EBADF;
    vnode = file->vnode;

    if (!((file->oflags & O_WRONLY) || !chkperm_vnode_curproc(vnode, W_OK))) {
        retval = -EPERM;
        goto out;
    }

    retval = vnode->vnode_ops->chmod(vnode, mode);

out:
    fs_fildes_ref(curproc->files, fildes, -1);

    return retval;
}

int fs_chflags_curproc(int fildes, fflags_t flags)
{
    vnode_t * vnode;
    file_t * file;
    int retval = 0;

    file = fs_fildes_ref(curproc->files, fildes, 1);
    if (!file)
        return -EBADF;
    vnode = file->vnode;

    if (!((file->oflags & O_WRONLY) || !chkperm_vnode_curproc(vnode, W_OK))) {
        retval = -EPERM;
        goto out;
    }
    retval = priv_check(&curproc->cred, PRIV_VFS_SYSFLAGS);
    if (retval)
        goto out;

    retval = vnode->vnode_ops->chflags(vnode, flags);

out:
    fs_fildes_ref(curproc->files, fildes, -1);

    return retval;
}

int fs_chown_curproc(int fildes, uid_t owner, gid_t group)
{
    vnode_t * vnode;
    file_t * file;
    int retval = 0;

    file = fs_fildes_ref(curproc->files, fildes, 1);
    if (!file)
        return -EBADF;
    vnode = file->vnode;

    if (!((file->oflags & O_WRONLY) || !chkperm_vnode_curproc(vnode, W_OK))) {
        retval = -EPERM;
        goto out;
    }

    retval = vnode->vnode_ops->chown(vnode, owner, group);

out:
    fs_fildes_ref(curproc->files, fildes, -1);

    return retval;
}

int vrefcnt(struct vnode * vnode)
{
    int retval;

    retval = atomic_read(&vnode->vn_refcount);

    return retval;
}

void vrefset(vnode_t * vnode, int refcnt)
{
    atomic_set(&vnode->vn_refcount, refcnt);
}

int vref(vnode_t * vnode)
{
    int prev;

    prev = atomic_read(&vnode->vn_refcount);
    if (prev < 0) {
#ifdef configFS_VREF_DEBUG
        FS_KERROR_VNODE(KERROR_ERR, vnode,
                        "Failed, vnode will be freed soon or it's orphan (%d)\n",
                        prev);
#endif
        return -ENOLINK;
    }

#ifdef configFS_VREF_DEBUG
    prev =
#endif
           atomic_inc(&vnode->vn_refcount);

#ifdef configFS_VREF_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, vnode, "%d\n", prev);
#endif

    return 0;
}

void vrele(vnode_t * vnode)
{
    int prev;

    if (!vnode)
        return;

    prev = atomic_dec(&vnode->vn_refcount);

#ifdef configFS_VREF_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, vnode, "%d\n", prev);
#endif

    if (prev <= 1)
        vnode->sb->delete_vnode(vnode);
}

void vrele_nunlink(vnode_t * vnode)
{
    if (!vnode)
        return;

    atomic_dec(&vnode->vn_refcount);
}

void vput(vnode_t * vnode)
{
    int prev;

    if (!vnode)
        return;

    KASSERT(mtx_test(&vnode->vn_lock), "vnode should be locked");

    prev = atomic_dec(&vnode->vn_refcount);
    VN_UNLOCK(vnode);
    if (prev <= 1)
        vnode->sb->delete_vnode(vnode);
}

void vunref(vnode_t * vnode)
{
    int prev;

    if (!vnode)
        return;

    KASSERT(mtx_test(&vnode->vn_lock), "vnode should be locked");

    prev = atomic_dec(&vnode->vn_refcount);
    if (prev <= 1)
        vnode->sb->delete_vnode(vnode);
}

static int ksprintf_fmt_fs(KSPRINTF_FMTFUN_ARGS)
{
    int n;
    fs_t * fs = *(fs_t **)value_p;

    if (fs) {
        n = strlcpy(str, fs->fsname, min(maxlen, sizeof(fs->fsname)));
    } else {
        const char null_str[] = {'(', 'n', 'u', 'l', 'l', ')'};

        n = min(maxlen, sizeof(null_str));
        memcpy(str, null_str, n);
    }

    return n;
}

static const struct ksprintf_formatter ksprintf_fmt_fs_st = {
    .flags = KSPRINTF_FMTFLAG_p,
    .specifier = 'p',
    .p_specifier = 'F',
    .func = ksprintf_fmt_fs,
};
KSPRINTF_FORMATTER(ksprintf_fmt_fs_st);

static int ksprintf_fmt_vnode(KSPRINTF_FMTFUN_ARGS)
{
    int n;
    vnode_t * vnode = *(vnode_t **)value_p;

    if (vnode) {
        fs_t * fs = (vnode->sb) ? fs = vnode->sb->fs : NULL;

        n = ksprintf_fmt_fs(str, &fs, sizeof(fs_t **), maxlen);
        if (n == maxlen)
            return n;
        str += n;
        n++;
        maxlen -= n;
        *str++ = ':';

        if (maxlen < ui64_chcnt(vnode->vn_num))
            return n;
        n += uitoa64(str, vnode->vn_num);
    } else {
        const char nil_str[] = {'(', 'n', 'u', 'l', 'l', ')', ':',  '-', '1'};

        n = min(maxlen, sizeof(nil_str));
        memcpy(str, nil_str, n);
    }

    return n;
}

static const struct ksprintf_formatter ksprintf_fmt_vnode_st = {
    .flags = KSPRINTF_FMTFLAG_p,
    .specifier = 'p',
    .p_specifier = 'V',
    .func = ksprintf_fmt_vnode,
};
KSPRINTF_FORMATTER(ksprintf_fmt_vnode_st);
