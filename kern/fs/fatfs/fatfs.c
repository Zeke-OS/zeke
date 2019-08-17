/**
 *******************************************************************************
 * @file    fatfs.c
 * @author  Olli Vanhoja
 * @brief   File System wrapper for FatFs.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <dirent.h>
#include <fcntl.h>
#include <sys/dev_major.h>
#include <sys/hash.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <kerror.h>
#include <kinit.h>
#include <kstring.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/vfs_hash.h>
#include <kmalloc.h>
#include <proc.h>
#include "fatfs.h"

static int fatfs_mount(fs_t * fs, const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb);
static int fatfs_umount(struct fs_superblock * fs_sb);
static char * format_fpath(struct fatfs_inode * indir, const char * name);
static int create_inode(struct fatfs_inode ** result, struct fatfs_sb * sb,
                        char * fpath, size_t vn_hash, int oflags);
static void finalize_inode(vnode_t * vnode);
static void destroy_vnode(vnode_t * vnode);
static int fatfs_statfs(struct fs_superblock * sb, struct statvfs * st);
static int fatfs_delete_vnode(vnode_t * vnode);
static int fatfs_event_vnode_opened(struct proc_info * p, vnode_t * vnode);
static void fatfs_event_file_closed(struct proc_info * p, file_t * file);
static int fatfs_lookup(vnode_t * dir, const char * name, vnode_t ** result);
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             struct fs_superblock * sb);
static int get_mp_stat(vnode_t * vnode, struct stat * st);
static int fresult2errno(int fresult);

static struct fs fatfs_fs = {
    .fsname = FATFS_FSNAME,
    .fs_majornum = VDEV_MJNR_FATFS,
    .mount = fatfs_mount,
    .sblist_head = SLIST_HEAD_INITIALIZER(),
};

static vfs_hash_ctx_t vfs_hash_ctx;
static uint32_t fatfs_siphash_key[2];

vnode_ops_t fatfs_vnode_ops = {
    .write = fatfs_write,
    .read = fatfs_read,
    .event_vnode_opened = fatfs_event_vnode_opened,
    .event_fd_closed = fatfs_event_file_closed,
    .create = fatfs_create,
    .mknod = fatfs_mknod,
    .lookup = fatfs_lookup,
    .unlink = fatfs_unlink,
    .mkdir = fatfs_mkdir,
    .rmdir = fatfs_rmdir,
    .readdir = fatfs_readdir,
    .stat = fatfs_stat,
    .chmod = fatfs_chmod,
    .chflags = fatfs_chflags,
};

/**
 * Comparator function for vfs_hash.
 */
static int fatfs_vncmp(struct vnode * vp, void * arg)
{
    const struct fatfs_inode * const in = get_inode_of_vnode(vp);
    const char * fpath = (char *)arg;

    return strcmp(in->in_fpath, fpath);
}

int __kinit__ fatfs_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("fatfs");

    fatfs_siphash_key[0] = krandom();
    fatfs_siphash_key[1] = krandom();
    vfs_hash_ctx = vfs_hash_new_ctx("fatfs", configFATFS_DESIREDVNODES,
                                    fatfs_vncmp);
    if (!vfs_hash_ctx)
        return -ENOMEM;

    fs_inherit_vnops(&fatfs_vnode_ops, &nofs_vnode_ops);

    FS_GIANT_INIT(&fatfs_fs.fs_giant);
    fs_register(&fatfs_fs);

    return 0;
}

static vnode_t * create_raw_inode(const struct fs_superblock * sb)
{
    struct fatfs_inode * in;

    in = kzalloc(sizeof(struct fatfs_inode));

    return &in->in_vnode;
}

static vnode_t * create_root(struct fatfs_sb * fatfs_sb)
{
    char * rootpath = fatfs_sb->fpath_root;
    size_t vn_hash;
    struct fatfs_inode * in = NULL;
    int err;

    rootpath[0] = '/';
    vn_hash = halfsiphash32(rootpath, 1, fatfs_siphash_key);
    err = create_inode(&in, fatfs_sb, rootpath, vn_hash,
                       O_DIRECTORY | O_RDWR);
    if (err || unlikely(!in)) {
        KERROR(KERROR_ERR, "Failed to init a root vnode for fatfs (%d)\n", err);
        return NULL;
    }

    /*
     * Note that the refcount +2 should remain so that the root vnode won't be
     * ever freed from the dirty vnode list.
     */
    in->in_fpath = rootpath;
    return &in->in_vnode;
}

/**
 * Mount a new fatfs.
 * @param mode      mount flags.
 * @param param     contains optional mount parameters.
 * @param parm_len  length of param string.
 * @param[out] sb   Returns the superblock of the new mount.
 * @return error code, -errno.
 */
static int fatfs_mount(fs_t * fs, const char * source, uint32_t mode,
                       const char * parm, int parm_len,
                       struct fs_superblock ** sb)
{
    static dev_t fatfs_vdev_minor;
    struct fatfs_sb * fatfs_sb = NULL;
    vnode_t * vndev;
    int err, retval = 0;

    /* Get device vnode */
    err = lookup_vnode(&vndev, curproc->croot, source, 0);
    if (err) {
        KERROR_DBG("fatfs source not found\n");
        return err;
    }
    if (!S_ISBLK(vndev->vn_mode)) {
        retval = -ENOTBLK;
        goto fail;
    }

    /*
     * Fail if mouting a read-only device but
     * mount mode is not MNT_READONLY.
     */
    if (!((mode & MNT_RDONLY) == MNT_RDONLY) &&
        !((vndev->vn_mode & S_IWUSR) == S_IWUSR)) {
        retval = -EROFS;
        goto fail;
    }

    /* Allocate superblock */
    fatfs_sb = kzalloc(sizeof(struct fatfs_sb));
    if (!fatfs_sb) {
        retval = -ENOMEM;
        goto fail;
    }

    fs_fildes_set(&fatfs_sb->ff_devfile, vndev, O_RDWR);
    fatfs_sb->sb.vdev_id = DEV_MMTODEV(VDEV_MJNR_FATFS, fatfs_vdev_minor++);

    /*
     * We create initialize the inpool with the same number of desired vnodes
     * as the vfs_hash hashmap even though latter one is system global and
     * inpool is per super block.
     */
    retval = inpool_init(&fatfs_sb->inpool, &fatfs_sb->sb,
                         create_raw_inode, destroy_vnode, finalize_inode,
                         configFATFS_DESIREDVNODES);
    if (retval) {
        goto fail;
    }

    /* Mount */
    err = f_mount(&fatfs_sb->ff_fs, 0);
    if (err) {
        retval = fresult2errno(err);
        KERROR_DBG("Can't init a work area for FAT (%d)\n", retval);
        goto fail;
    }
    KERROR_DBG("Initialized a work area for FAT\n");

#if (_FS_NOFSINFO == 0) /* Commit full scan of free clusters */
    DWORD nclst;

    f_getfree(&fatfs_sb->ff_fs, &nclst);
#endif

    /* Init super block */
    fs_init_superblock(&fatfs_sb->sb, fs);
    fatfs_sb->sb.mode_flags = mode;
    fatfs_sb->sb.sb_dev = vndev;
    fatfs_sb->sb.sb_hashseed = fatfs_sb->sb.vdev_id;
    fatfs_sb->sb.root = create_root(fatfs_sb);
    /* Function pointers to superblock methods */
    fatfs_sb->sb.statfs = fatfs_statfs;
    fatfs_sb->sb.delete_vnode = fatfs_delete_vnode;
    fatfs_sb->sb.umount = fatfs_umount;
    if (!fatfs_sb->sb.root) {
        KERROR(KERROR_ERR, "Root of fatfs not found\n");
        retval = -EIO;
        goto fail;
    }

    fs_insert_superblock(fs, &fatfs_sb->sb);

fail:
    if (retval && fatfs_sb) {
        kfree(fatfs_sb);
    } else {
        *sb = &fatfs_sb->sb;
    }
    if (retval)
        vrele(vndev);
    return retval;
}

static int fatfs_umount(struct fs_superblock * fs_sb)
{
    struct fatfs_sb * fatfs_sb = get_ffsb_of_sb(fs_sb);

    /* TODO sync open vnodes and release them */

    /*
     * TODO Check that there is no more references to any vnodes of
     * this super block before destroying everything related to it.
     */
    fs_remove_superblock(fs_sb->fs, &fatfs_sb->sb);
    f_umount(&fatfs_sb->ff_fs);
    vrele(fatfs_sb->ff_devfile.vnode);
    inpool_destroy(&fatfs_sb->inpool);
    kfree(fatfs_sb);

    return 0;
}

/**
 * Get kmalloc'd array for full path name.
 */
static char * format_fpath(struct fatfs_inode * indir, const char * name)
{
    char * fpath;
    size_t fpath_size;

    KERROR_DBG("%s(indir \"%s\", name \"%s\")\n",
               __func__, indir->in_fpath, name);

    fpath_size = strlenn(name, NAME_MAX + 1) +
                 strlenn(indir->in_fpath, NAME_MAX + 1) + 6;
    fpath = kmalloc(fpath_size);
    if (!fpath)
        return NULL;

    ksprintf(fpath, fpath_size, "%s/%s", indir->in_fpath, name);

    KERROR_DBG("Formatted \"%s\" as \"%s\"\n", name, fpath);

    return fpath;
}

/**
 * Create a inode.
 * @param fpath won't be duplicated.
 * @param oflags O_CREAT, O_DIRECTORY, O_RDONLY, O_WRONLY and O_RDWR
 *               currently supported.
 *               O_WRONLY/O_RDWR creates in write mode if possible, so this
 *               should be always verified with stat.
 */
static int create_inode(struct fatfs_inode ** result, struct fatfs_sb * sb,
                        char * fpath, size_t vn_hash, int oflags)
{
    struct fatfs_inode * in = NULL;
    FILINFO fno;
    vnode_t * vn;
    vnode_t * xvp;
    mode_t vn_mode;
    ino_t inum;
    int err = 0, retval = 0;

    KERROR_DBG("%s(fpath \"%s\", vn_hash %u)\n",
               __func__, fpath, (uint32_t)vn_hash);

    vn = inpool_get_next(&sb->inpool);
    if (!vn) {
        retval = -ENOMEM;
        goto fail;
    }
    in = get_inode_of_vnode(vn);
    in->in_fpath = fpath;

    in->open_count = ATOMIC_INIT(0);

    memset(&fno, 0, sizeof(fno));

    if (oflags & O_DIRECTORY) {
        /* O_DIRECTORY was specified. */
        /* TODO Maybe get mp stat? */
        fno.fattrib = AM_DIR;
    } else if (oflags & O_CREAT) {
        if (sb->sb.mode_flags & MNT_RDONLY)
            return -EROFS;
    } else {
        err = f_stat(&sb->ff_fs, fpath, &fno);
        if (err) {
            retval = fresult2errno(err);
            goto fail;
        }
    }

    /* Try open */
    if (fno.fattrib & AM_DIR) {
        /* it's a directory */
        vn_mode = S_IFDIR;
        err = f_opendir(&in->dp, &sb->ff_fs, in->in_fpath);
        if (err) {
            KERROR_DBG("%s: Can't open a dir (err: %d)\n",
                       __func__, err);
            retval = fresult2errno(err);
            goto fail;
        }
        inum = in->dp.ino;
    } else {
        /* it's a file */
        unsigned char fomode = 0;

        fomode |= (oflags & O_CREAT) ? FA_OPEN_ALWAYS : FA_OPEN_EXISTING;
        /* The kernel should always have RW if possible. */
        if (sb->sb.mode_flags & MNT_RDONLY) {
            fomode |= FA_READ;
        } else {
            fomode |= FA_READ | FA_WRITE;
        }

        vn_mode = S_IFREG;
        err = f_open(&in->fp, &sb->ff_fs, in->in_fpath, fomode);
        if (err) {
#ifdef configFATFS_DEBUG
            FS_KERROR_FS(KERROR_DEBUG, sb->sb.fs,
                         "Can't open a file (err: %d)\n",
                         err);
#endif
            retval = fresult2errno(err);
            goto fail;
        }
        inum = in->fp.ino;
    }

#ifdef configFATFS_DEBUG
    if (oflags & O_CREAT)
        FS_KERROR_FS(KERROR_DEBUG, sb->sb.fs, "Create & open ok\n");
    else
        FS_KERROR_FS(KERROR_DEBUG, sb->sb.fs, "Open ok\n");
#endif

    init_fatfs_vnode(vn, inum, vn_mode, &sb->sb);

    /* Insert to the cache */
    err = vfs_hash_insert(vfs_hash_ctx, vn, vn_hash, &xvp, fpath);
    if (err) {
        retval = -ENOMEM;
        goto fail;
    }
    if (xvp) {
        FS_KERROR_FS(KERROR_ERR, sb->sb.fs, "Found it during insert: \"%s\"\n",
                     fpath);
        retval = ENOTRECOVERABLE;
        goto fail;
    }

#ifdef configFATFS_DEBUG
    FS_KERROR_FS(KERROR_DEBUG, sb->sb.fs, "ok\n");
#endif

    vrefset(vn, 2);
    inpool_insert_dirty(&sb->inpool, vn);

    *result = in;
    return 0;
fail:
#ifdef configFATFS_DEBUG
    FS_KERROR_FS(KERROR_DEBUG, sb->sb.fs, "retval %i\n", retval);
#endif

    inpool_insert_clean(&sb->inpool, vn); /* Return it back to the pool. */
    return retval;
}

/**
 * Sync inode and destroy cached data linked to the inode.
 */
static void finalize_inode(vnode_t * vnode)
{
    struct fatfs_inode * in = get_inode_of_vnode(vnode);

    KERROR_DBG("%s(in %p), %s\n", __func__, in, in->in_fpath);

    vrele_nunlink(vnode); /* If called by inpool */
    vfs_hash_remove(vfs_hash_ctx, &in->in_vnode);

    /*
     * We use a negative value of vn_len to mark a deleted directory entry,
     * if the entry is already deleted it's also properly closed.
     */
    if (vnode->vn_len >= 0) {
        if (!S_ISDIR(vnode->vn_mode))
            f_sync(&in->fp);
    }

    kfree(in->in_fpath);
    memset(in, 0, sizeof(*in));
}

/**
 * This function is called when a pooled inode should be freed.
 */
static void destroy_vnode(vnode_t * vnode)
{
    struct fatfs_inode * in = get_inode_of_vnode(vnode);

    KERROR_DBG("%s(vnode %pV), in: %p\n", __func__, vnode, in);

    /* TODO Free the inode, currently something fails and the kernel freezes. */
#if 0
    kfree(in);
#endif
}

/**
 * Get fatfs statistics.
 */
static int fatfs_statfs(struct fs_superblock * sb, struct statvfs * st)
{
    struct fatfs_sb * ffsb = get_ffsb_of_sb(sb);
    FATFS * fat = &ffsb->ff_fs;
    DWORD nclst_free, vsn;
    int err;

    err = fresult2errno(f_getfree(fat, &nclst_free));
    if (err) {
        return err;
    }

    err = f_getlabel(fat, NULL, &vsn);
    if (err) {
        return err;
    }

    *st = (struct statvfs){
        .f_bsize = fat->ssize,
        .f_frsize = fat->ssize * fat->csize,
        .f_blocks = fat->n_fatent,
        .f_bfree = nclst_free,
        .f_bavail = nclst_free,
        .f_files = 0,
        .f_ffree = 0,
        .f_favail = 0,
        .f_fsid = vsn,
        .f_flag = sb->mode_flags,
        .f_namemax = NAME_MAX + 1,
    };
    strlcpy(st->fsname, sb->fs->fsname, sizeof(st->fsname));

    return 0;
}

/**
 * Delete a vnode from the cache.
 */
static int fatfs_delete_vnode(vnode_t * vnode)
{
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    struct fatfs_sb * sb = get_ffsb_of_sb(vnode->sb);

#ifdef configFATFS_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, vnode, "%s\n", in->in_fpath);
#endif

    /*
     * We need to decrement the refcount if the function was called in order to
     * sync diry vnodes.
     */
    vrele_nunlink(vnode);

    if (vrefcnt(vnode) > 0) {
        if (!S_ISDIR(vnode->vn_mode))
            f_sync(&in->fp);
    } else {
        finalize_inode(vnode);
        /* Recycle the inode */
        inpool_insert_clean(&sb->inpool, vnode);
    }

    return 0;
}

static int fatfs_event_vnode_opened(struct proc_info * p, vnode_t * vnode)
{
    struct fatfs_inode * in = get_inode_of_vnode(vnode);

    atomic_inc(&in->open_count);

    return 0;
}

static void fatfs_event_file_closed(struct proc_info * p, file_t * file)
{
    struct fatfs_inode * in = get_inode_of_vnode(file->vnode);

    /*
     * Sync on close.
     */
    f_sync(&in->fp);

    atomic_dec(&in->open_count);
}

/**
 * Lookup for a vnode (file/dir) in FatFs.
 * First lookup form vfs_hash and if not found then read it from ff which will
 * probably read it via devfs interface. After the vnode has been created it
 * will be added to the vfs hashmap. In ff terminology all files and directories
 * that are in hashmap are also open on a file/dir handle, thus we'll have to
 * make sure we don't have too many vnodes in cache that have no references, to
 * avoid hitting any ff hard limits.
 */
static int fatfs_lookup(vnode_t * dir, const char * name, vnode_t ** result)
{
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    struct fatfs_sb * sb = get_ffsb_of_sb(dir->sb);
    char * in_fpath;
    size_t in_fpath_len;
    size_t vn_hash;
    struct vnode * vn = NULL;
    int retval = 0;

    KASSERT(dir != NULL, "dir must be set");

    /* Format full path */
    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    in_fpath_len = strlenn(in_fpath, NAME_MAX + 1);

    /*
     * Emulate . and ..
     */
    if (name[0] == '.' && name[1] != '.') {
#ifdef configFATFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, dir, "Lookup emulating \".\"\n");
#endif
        (void)vref(dir);
        *result = dir;

        kfree(in_fpath);
        return 0;
    } else if (name[0] == '.' && name[1] == '.' && name[2] != '.') {
#ifdef configFATFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, dir, "Lookup emulating \"..\"\n");
#endif
        if (VN_IS_FSROOT(dir)) {
            /*
             * No ref is taken since we are returning an error and the caller
             * has at least one ref anyway, so it's safe.
             */
            *result = dir;

            kfree(in_fpath);
            return -EDOM;
        } else {
            size_t i = in_fpath_len - 4;

            while (in_fpath[i] != '/') {
                i--;
            }
            in_fpath[i] = '\0';
        }
    }

    /* Short circuit for root */
    if (in_fpath[0] == '/' && in_fpath[1] == '\0') {
        kfree(in_fpath);
        *result = dir->sb->root;
        vref(*result);

        return 0;
    }

    /*
     * Lookup from vfs_hash cache
     */
    vn_hash = halfsiphash32(in_fpath, in_fpath_len, fatfs_siphash_key);
    retval = vfs_hash_get(vfs_hash_ctx,
                          dir->sb,      /* FS superblock */
                          vn_hash,      /* Hash */
                          &vn,          /* Retval */
                          in_fpath      /* Compared fpath */
                         );
    if (retval) {
#ifdef configFATFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, NULL,
                        "Call to vfs_hash_get() failed (%d)\n",
                        retval);
#endif
        retval = -EIO;
    } else if (vn) { /* vnode found in vfs_hash */
#ifdef configFATFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, vn, "vn found in vfs_hash (%p)\n", vn);
#endif

        *result = vn;
        retval = 0;
    } else { /* not cached */
        struct fatfs_inode * in = NULL;

        KERROR_DBG("%s: vn not in vfs_hash\n", __func__);

        /*
         * Create a inode and fetch data from the device.
         * This also vrefs.
         */
        retval = create_inode(&in, sb, in_fpath, vn_hash, O_RDWR);
        if (!retval) {
            KASSERT(in != NULL, "in must be set");
            in_fpath = NULL; /* shall not be freed. */
            *result = &in->in_vnode;
        }
    }

    kfree(in_fpath);
    return retval;
}

ssize_t fatfs_read(file_t * file, struct uio * uio, size_t count)
{
    void * buf;
    struct fatfs_inode * in = get_inode_of_vnode(file->vnode);
    size_t count_out;
    int err;

    if (!S_ISREG(file->vnode->vn_mode))
        return -EOPNOTSUPP;

    err = f_lseek(&in->fp, file->seek_pos);
    if (err)
        return -EIO;

    err = uio_get_kaddr(uio, &buf);
    if (err)
        return err;

    err = f_read(&in->fp, buf, count, &count_out);
    if (err)
        return fresult2errno(err);

    file->seek_pos = f_tell(&in->fp);
    return count_out;
}

ssize_t fatfs_write(file_t * file, struct uio * uio, size_t count)
{
    void * buf;
    struct fatfs_inode * in = get_inode_of_vnode(file->vnode);
    size_t count_out;
    int err;

    if (!S_ISREG(file->vnode->vn_mode))
        return -EOPNOTSUPP;

    err = f_lseek(&in->fp, file->seek_pos);
    if (err)
        return -EIO;

    err = uio_get_kaddr(uio, &buf);
    if (err)
        return err;

    err = f_write(&in->fp, buf, count, &count_out);
    if (err)
        return fresult2errno(err);

    file->seek_pos = f_tell(&in->fp);

    return count_out;
}

int fatfs_create(vnode_t * dir, const char * name, mode_t mode,
                 vnode_t ** result)
{
    return fatfs_mknod(dir, name, (mode & ~S_IFMT) | S_IFREG, NULL, result);
}

int fatfs_unlink(vnode_t * dir, const char * name)
{
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    kmalloc_autofree char * in_fpath = NULL;
    vnode_autorele vnode_t * vnode = NULL;
    struct fatfs_inode * in;
    FATFS * fs;
    int retval;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    if (fatfs_lookup(dir, name, &vnode)) {
        retval = -ENOTDIR;
        goto out;
    }

    if (atomic_read(&get_inode_of_vnode(vnode)->open_count) != 0) {
        retval = -EBUSY;
        goto out;
    }

    in = get_inode_of_vnode(vnode);
    if (S_ISDIR(vnode->vn_mode)) {
        fs = in->dp.fs;
    } else {
        fs = in->fp.fs;
    }
    retval = fresult2errno(f_unlink(fs, in->in_fpath));
    if (retval)
        goto out;

    vnode->vn_len = -1; /* Mark deleted by setting len to a negative value. */
    vrele_nunlink(vnode);

    retval = 0;
out:
    kfree(in_fpath);
    return retval;
}

int fatfs_mknod(vnode_t * dir, const char * name, int mode, void * specinfo,
                vnode_t ** result)
{
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    struct fatfs_inode * res = NULL;
    char * in_fpath;
    size_t in_fpath_len;
    int err;

    KERROR_DBG("%s(dir %p, name \"%s\", mode %u, specinfo %p, result %p)\n",
               __func__, dir, name, mode, specinfo, result);

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    if ((mode & S_IFMT) != S_IFREG)
        return -ENOTSUP; /* FAT only suports regular files. */

    if (specinfo)
        return -EINVAL; /* specinfo not supported. */

    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    in_fpath_len = strlenn(in_fpath, NAME_MAX + 1);
    err = create_inode(&res, get_ffsb_of_sb(dir->sb), in_fpath,
                       halfsiphash32(in_fpath, in_fpath_len, fatfs_siphash_key),
                       O_CREAT);
    if (err) {
        kfree(in_fpath);
        return fresult2errno(err);
    }
    KASSERT(res != NULL, "res must be set");

    if (result)
        *result = &res->in_vnode;
    fatfs_chmod(&res->in_vnode, mode);

#ifdef configFATFS_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, dir, "ok\n");
#endif

    return 0;
}

int fatfs_mkdir(vnode_t * dir,  const char * name, mode_t mode)
{
    struct fatfs_sb * ffsb = get_ffsb_of_sb(dir->sb);
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    char * in_fpath;
    FRESULT err;
    int retval = 0;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    err = f_mkdir(&ffsb->ff_fs, in_fpath);
    if (err)
        retval = fresult2errno(err);

    kfree(in_fpath);
    return retval;
}

int fatfs_rmdir(vnode_t * dir,  const char * name)
{
    int err;
    vnode_t * result;
    vnode_t * nmp;
    mode_t mode;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    err = fatfs_lookup(dir, name, &result);
    if (err)
        return err;
    mode = result->vn_mode;
    nmp = result->vn_next_mountpoint;
    vrele_nunlink(result);

    if (!S_ISDIR(mode))
        return -ENOTDIR;
    if (nmp != result)
        return -EBUSY;

    return fatfs_unlink(dir, name);
}

int fatfs_readdir(vnode_t * dir, struct dirent * d, off_t * off)
{
    struct fatfs_inode * in = get_inode_of_vnode(dir);
    FILINFO fno;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    if (*off == DIRENT_SEEK_START) { /* Emulate . */
        f_readdir(&in->dp, NULL); /* Rewind */

        strlcpy(d->d_name, ".", sizeof(d->d_name));
        d->d_ino = dir->vn_num;
        d->d_type = DT_DIR;
        *off = DIRENT_SEEK_START + 1;
    } else if (*off == DIRENT_SEEK_START + 1) { /* Emulate .. */
        strlcpy(d->d_name, "..", sizeof(d->d_name));
        d->d_ino = 0; /* TODO ino should be properly set */
        d->d_type = DT_DIR;
        *off = DIRENT_SEEK_START + 2;
    } else { /* Normal dir entry */
        int err;

#if configFATFS_LFN
        fno.lfname = d->d_name;
#endif

        err = f_readdir(&in->dp, &fno);
        if (err)
            return fresult2errno(err);

        if (fno.fname[0] == '\0')
            return -ESPIPE;

        d->d_ino = fno.ino;
        d->d_type = (fno.fattrib & AM_DIR) ? DT_DIR : DT_REG;
#if configFATFS_LFN
        if (!*fno.lfname)
#endif
            strlcpy(d->d_name, fno.fname, sizeof(d->d_name));
    }

    return 0;
}

static fflags_t fattrib2uflags(unsigned fattrib)
{
    fflags_t flags = 0;

    flags |= (fattrib & AM_RDO) ? UF_READONLY : 0;
    flags |= (fattrib & AM_HID) ? UF_HIDDEN : 0;
    flags |= (fattrib & AM_ARC) ? UF_ARCHIVE : 0;
    flags |= (fattrib & AM_SYS) ? UF_SYSTEM : 0;

    return flags;
}

int fatfs_stat(vnode_t * vnode, struct stat * buf)
{
    struct fatfs_sb * ffsb = get_ffsb_of_sb(vnode->sb);
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    FILINFO fno;
    struct stat mp_stat;
    size_t blksize = ffsb->ff_fs.ssize;
    int err;

    KASSERT(buf != NULL, "buf should be set");
    memset(&mp_stat, 0, sizeof(struct stat));
    memset(&fno, 0, sizeof(fno));

    err = get_mp_stat(vnode, &mp_stat);
    if (err) {
        if (err == -EINPROGRESS) {
#ifdef configFATFS_DEBUG
            FS_KERROR_VNODE(KERROR_WARN, vnode,
                            "vnode->sb->mountpoint should be set\n");
#endif
        } else {
            KERROR_DBG("get_mp_stat() returned error (%d)\n", err);

            return err;
        }
    }

    if (vnode == vnode->sb->root) {
        /* Can't stat FAT root */
        memcpy(buf, &mp_stat, sizeof(struct stat));
    } else if (in->in_fpath[0] != '\0') {
        err = f_stat(&ffsb->ff_fs, in->in_fpath, &fno);
        if (err) {
            KERROR_DBG("%s(fs %p, fpath \"%s\", fno %p) failed\n",
                       __func__, &ffsb->ff_fs, in->in_fpath, &fno);
            return fresult2errno(err);
        }

        memset(buf, 0, sizeof(struct stat));
        buf->st_dev = vnode->sb->vdev_id;
        buf->st_ino = vnode->vn_num;
        buf->st_mode = vnode->vn_mode;
        buf->st_nlink = 1; /* Always one link on FAT. */
        buf->st_uid = mp_stat.st_uid;
        buf->st_gid = mp_stat.st_gid;
        buf->st_size = fno.fsize;
        buf->st_atim = fno.fatime;
        buf->st_mtim = fno.fmtime;
        buf->st_ctim = fno.fmtime;
        buf->st_birthtime = fno.fbtime;
        buf->st_flags = fattrib2uflags(fno.fattrib);
        buf->st_blksize = blksize;
        buf->st_blocks = fno.fsize / blksize + 1; /* Best guess. */
    } else {
        return -EINVAL;
    }

    return 0;
}

int fatfs_chmod(vnode_t * vnode, mode_t mode)
{
    struct fatfs_sb * ffsb = get_ffsb_of_sb(vnode->sb);
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    uint8_t attr = 0;
    const uint8_t mask = AM_RDO;
    int err;

    if (!(mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
        attr |= AM_RDO;

    err = fresult2errno(f_chmod(&ffsb->ff_fs, in->in_fpath, attr, mask));
    if (!err)
        vnode->vn_mode = mode;

    return err;
}

/*
 * Note: Thre is practically two ways to set AM_RDO, either
 * by using chmod() or by this chflags().
 */
int fatfs_chflags(vnode_t * vnode, fflags_t flags)
{
    struct fatfs_sb * ffsb = get_ffsb_of_sb(vnode->sb);
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    uint8_t attr = 0;
    const uint8_t mask = AM_RDO | AM_ARC | AM_SYS | AM_HID;
    FRESULT fresult;

    if (flags & UF_SYSTEM)
        attr |= AM_SYS;
    if (flags & UF_ARCHIVE)
        attr |= AM_ARC;
    if (flags & UF_READONLY)
        attr |= AM_RDO;
    if (flags & UF_HIDDEN)
        attr |= AM_HID;

    fresult = f_chmod(&ffsb->ff_fs, in->in_fpath, attr, mask);

    return fresult2errno(fresult);
}

/**
 * Initialize fatfs vnode data.
 * @param vnode is the target vnode to be initialized.
 */
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             struct fs_superblock * sb)
{
    struct stat stat;

    KERROR_DBG("%s(vnode %p, inum %llu, mode %o, sb %p)\n",
               __func__, vnode, (long long)inum, mode, sb);

    fs_vnode_init(vnode, inum, sb, &fatfs_vnode_ops);

#if 0
    if (S_ISDIR(mode))
        mode |= S_IRWXU | S_IXGRP | S_IXOTH;
#endif
    /*
     * TODO Set +x for all files as we don't have a way to store this
     * information yet.
     */
    mode |= S_IXUSR | S_IXGRP | S_IXOTH;

    vnode->vn_mode = mode | S_IRUSR | S_IRGRP | S_IROTH;
    memset(&stat, 0, sizeof(struct stat));
    if (fatfs_stat(vnode, &stat) == 0) {
        vnode->vn_len = stat.st_size;
        if ((stat.st_flags & UF_READONLY) == 0)
            vnode->vn_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
    } else {
        KERROR_DBG("%s failed\n", __func__);
    }
}

/**
 * Get mountpoint stat.
 */
static int get_mp_stat(vnode_t * vnode, struct stat * st)
{
     struct fs_superblock * sb;
     vnode_t * mp;

    KASSERT(vnode, "Vnode was given");
    KASSERT(vnode->sb, "Superblock is set");

    sb = vnode->sb;
    mp = sb->mountpoint;

    if (!mp) {
        /* We are probably mounting and mountpoint is not yet set. */
#ifdef configFATFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, vnode, "mp not set\n");
#endif
        return -EINPROGRESS;
    }

    KASSERT(mp->vnode_ops->stat, "stat() is defined");

    return mp->vnode_ops->stat(mp, st);
}

static int fresult2errno(int fresult)
{
    switch (fresult) {
    case FR_DISK_ERR:
    case FR_INVALID_OBJECT:
    case FR_INT_ERR:
        return -EIO;
    case FR_NOT_ENABLED:
        return -ENODEV;
    case FR_NO_FILESYSTEM:
        return -ENXIO;
    case FR_NO_FILE:
    case FR_NO_PATH:
        return -ENOENT;
    case FR_DENIED:
        return -EACCES;
    case FR_EXIST:
        return -EEXIST;
    case FR_WRITE_PROTECTED:
        return -EPERM;
    case FR_NOT_READY:
        return -EBUSY;
    case FR_INVALID_NAME:
    case FR_INVALID_DRIVE:
    case FR_MKFS_ABORTED:
    case FR_INVALID_PARAMETER:
        return -EINVAL;
    case FR_TIMEOUT:
        return -EWOULDBLOCK;
    case FR_NOT_ENOUGH_CORE:
        return -ENOMEM;
    case FR_TOO_MANY_OPEN_FILES:
        return -ENFILE;
    default:
        if (fresult != 0) /* Unknown error */
            return -EIO;
    }

    return 0;
}
