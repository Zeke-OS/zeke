/**
 *******************************************************************************
 * @file    fz_fs.c
 * @author  Olli Vanhoja
 * @brief   File System wrapper for FatFs.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/hash.h>
#include <kinit.h>
#include <kstring.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/vfs_hash.h>
#include <fs/dev_major.h>
#include <kmalloc.h>
#include <proc.h>
#include "fatfs.h"

static int fatfs_mount(const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb);
static char * format_fpath(struct fatfs_inode * indir, const char * name);
static int create_inode(struct fatfs_inode ** result, struct fatfs_sb * sb,
                        char * fpath, long vn_hash, int oflags);
static vnode_t * create_root(struct fs_superblock * sb);
static int fatfs_delete_vnode(vnode_t * vnode);
static int fatfs_file_opened(struct proc_info * p, vnode_t * vnode);
static void fatfs_file_closed(struct proc_info * p, file_t * file);
static int fatfs_lookup(vnode_t * dir, const char * name, vnode_t ** result);
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             long vn_hash, struct fs_superblock * sb);
static int get_mp_stat(vnode_t * vnode, struct stat * st);
static int fresult2errno(int fresult);

static struct fs fatfs_fs = {
    .fsname = FATFS_FSNAME,
    .mount = fatfs_mount,
    .umount = NULL,
    .sblist_head = SLIST_HEAD_INITIALIZER(),
};

/**
 * Array of all FAT mounts for faster access.
 */
struct fatfs_sb ** fatfs_sb_arr;

vnode_ops_t fatfs_vnode_ops = {
    .write = fatfs_write,
    .read = fatfs_read,
    .file_opened = fatfs_file_opened,
    .file_closed = fatfs_file_closed,
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

int fatfs_init(void) __attribute__((constructor));
int fatfs_init(void)
{
    int err;

    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("fatfs");

    fatfs_sb_arr = kcalloc(configFATFS_MAX_MOUNTS, sizeof(struct fatfs_sb *));
    if (!fatfs_sb_arr)
        return -ENOMEM;

    fs_inherit_vnops(&fatfs_vnode_ops, &nofs_vnode_ops);

    err = ff_init();
    if (err)
        return err;

    mtx_init(&fatfs_fs.fs_giant, MTX_TYPE_TICKET);
    fs_register(&fatfs_fs);

    return 0;
}

/**
 * Comparer for vfs_hash.
 */
static int fatfs_vncmp(struct vnode * vp, void * arg)
{
    const struct fatfs_inode * const in = get_inode_of_vnode(vp);
    const char * fpath = (char *)arg;

    return strcmp(in->in_fpath, fpath);
}

/**
 * Mount a new fatfs.
 * @param mode      mount flags.
 * @param param     contains optional mount parameters.
 * @param parm_len  length of param string.
 * @param[out] sb   Returns the superblock of the new mount.
 * @return error code, -errno.
 */
static int fatfs_mount(const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb)
{
    static dev_t fatfs_vdev_minor = 0;
    struct fatfs_sb * fatfs_sb = NULL;
    struct fs_superblock * sbp;
    vnode_t * vndev;
    int err, retval = 0;

    /* Get device vnode */
    err = lookup_vnode(&vndev, curproc->croot, source, 0);
    if (err)
        return err;
    if (!S_ISBLK(vndev->vn_mode))
        return -ENOTBLK;

    /* Allocate superblock */
    fatfs_sb = kcalloc(1, sizeof(struct fatfs_sb));
    if (!fatfs_sb)
        return -ENOMEM;

    sbp = &fatfs_sb->sb;
    fs_fildes_set(&fatfs_sb->ff_devfile, vndev, O_RDWR);
    sbp->vdev_id = DEV_MMTODEV(VDEV_MJNR_FATFS, fatfs_vdev_minor++);

    /* Insert sb to fatfs_sb_arr lookup array */
    fatfs_sb_arr[DEV_MINOR(sbp->vdev_id)] = fatfs_sb;

    /* Mount */
    char pdrv = (char)DEV_MINOR(sbp->vdev_id);
    char drive[5];
    ksprintf(drive, sizeof(drive), "%u:", pdrv);
    err = f_mount(&fatfs_sb->ff_fs, drive, 1);
    if (err) {
        retval = fresult2errno(err);
        goto fail;
    }
#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "Initialized a work area for FAT\n");
#endif

    /* Init super block */
    fs_init_superblock(&fatfs_sb->sb, &fatfs_fs);
    /* TODO Detect if target dev is rdonly */
    sbp->mode_flags = mode;
#if configFATFS_READONLY
    sbp->mode_flags |= MNT_RDONLY;
#endif
    sbp->root = create_root(sbp);
    sbp->sb_dev = vndev;
    sbp->sb_hashseed = sbp->vdev_id;
    /* Function pointers to superblock methods */
    sbp->get_vnode = NULL; /* Not implemented for FAT. */
    sbp->delete_vnode = fatfs_delete_vnode;

    if (!sbp->root) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "Root of fatfs not found\n");
#endif
        return -EIO;
    }

    /* Add this sb to the list of mounted file systems. */
    fs_insert_superblock(&fatfs_fs, &fatfs_sb->sb);

    goto out;
fail:
    fatfs_sb_arr[DEV_MINOR(sbp->vdev_id)] = NULL;
    kfree(fatfs_sb);
out:
    *sb = &(fatfs_sb->sb);
    return retval;
}

/**
 * Get kmalloc'd array for full path name.
 */
static char * format_fpath(struct fatfs_inode * indir, const char * name)
{
    char * fpath;
    size_t fpath_size;

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG,
             "format_fpath(indir \"%s\", name \"%s\")\n",
             indir->in_fpath, name);
#endif

    fpath_size = strlenn(name, NAME_MAX + 1) +
                 strlenn(indir->in_fpath, NAME_MAX + 1) + 6;
    fpath = kmalloc(fpath_size);
    if (!fpath)
        return NULL;

    ksprintf(fpath, fpath_size, "%s/%s", indir->in_fpath, name);

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "Formatted \"%s\" as \"%s\"\n",
           name, fpath);
#endif

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
                        char * fpath, long vn_hash, int oflags)
{
    struct fatfs_inode * in = NULL;
    FILINFO fno;
    vnode_t * vn;
    vnode_t * xvp;
    mode_t vn_mode;
    ino_t inum;
    int err = 0, retval = 0;

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "create_inode(fpath \"%s\", vn_hash %u)\n",
             fpath, (uint32_t)vn_hash);
#endif

    in = kcalloc(1, sizeof(struct fatfs_inode));
    if (!in) {
        retval = -ENOMEM;
        goto fail;
    }
    in->in_fpath = fpath;
    vn = &in->in_vnode;

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
        err = f_stat(fpath, &fno);
        if (err) {
            retval = fresult2errno(err);
            goto fail;
        }
    }

    /* Try open */
    if (fno.fattrib & AM_DIR) {
        /* it's a directory */
        vn_mode = S_IFDIR;
        err = f_opendir(&in->dp, in->in_fpath);
        if (err) {
            retval = fresult2errno(err);
            goto fail;
        }
        inum = in->dp.ino;
    } else {
        /* it's a file */
        unsigned char fomode = 0;

        fomode |= (oflags & O_CREAT) ? FA_OPEN_ALWAYS : FA_OPEN_EXISTING;
        fomode |= (oflags & O_RDONLY) ? FA_READ : 0;
        fomode |= (oflags & O_WRONLY) ? ((fno.fattrib & AM_RDO) ?
                                         0 : FA_WRITE) : 0;

        vn_mode = S_IFREG;
        err = f_open(&in->fp, in->in_fpath, fomode);
        if (err) {
            retval = fresult2errno(err);
            goto fail;
        }
        inum = in->fp.ino;
    }

#ifdef configFATFS_DEBUG
    if (oflags & O_CREAT)
        KERROR(KERROR_DEBUG, "ff: Create & open ok\n");
    else
        KERROR(KERROR_DEBUG, "ff: Open ok\n");
#endif

    init_fatfs_vnode(vn, inum, vn_mode, vn_hash, &(sb->sb));

    /* Insert to the cache */
    err = vfs_hash_insert(vn, vn_hash, &xvp, fatfs_vncmp, fpath);
    if (err) {
        retval = -ENOMEM;
        goto fail;
    }
    if (xvp) {
        /* TODO No idea what to do now */
        KERROR(KERROR_WARN,
                "create_inode(): Found it during insert: \"%s\"\n",
                fpath);
    }

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "create_inode(): ok\n");
#endif

    *result = in;
    vrefset(vn, 1); /* Ref for caller. */
    return 0;
fail:
#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "create_inode(): err %i, retval %i\n",
           err, retval);
#endif

    kfree(in);
    return retval;
}

static vnode_t * create_root(struct fs_superblock * sb)
{
    char * rootpath;
    long vn_hash;
    struct fatfs_inode * in;
    int err;

    rootpath = kmalloc(5);
    if (!rootpath)
        return NULL;

    ksprintf(rootpath, sizeof(rootpath), "%u:", DEV_MINOR(sb->vdev_id));
    vn_hash = hash32_str(rootpath, 0);

    err = create_inode(&in, get_ffsb_of_sb(sb), rootpath, vn_hash,
                       O_DIRECTORY | O_RDWR);
    if (err) {
        KERROR(KERROR_ERR, "Failed to get a root for fatfs\n");
        return NULL;
    }

    in->in_fpath = rootpath;

    vrefset(&in->in_vnode, 1);
    return &in->in_vnode;
}

static int fatfs_delete_vnode(vnode_t * vnode)
{
    /* TODO Implementation of fatfs_delete_vnode() */
#if 0
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    char * dirname;
    char * name;

    /* TODO Remove only from memory in there is links or refs */

    dirname = kstrdup(in->in_fpath, PATH_MAX);
    name = kstrrchr(in->in_fpath, '/');
    if (!name)
        return -EINVAL;
    name = '\0';
    name++;

    vrele_nunlink(vnode);
    fatfs_unkink(dir, name);
    kfree(dirname);
#endif

    return -ENOTSUP;
}

static int fatfs_file_opened(struct proc_info * p, vnode_t * vnode)
{
    struct fatfs_inode * in = get_inode_of_vnode(vnode);

    atomic_inc(&in->open_count);

    return 0;
}

static void fatfs_file_closed(struct proc_info * p, file_t * file)
{
    struct fatfs_inode * in = get_inode_of_vnode(file->vnode);

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
    long vn_hash;
    struct vnode * vn = NULL;
    int err, retval = 0;

    KASSERT(dir != NULL, "dir must be set");

    /* Format full path */
    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    /*
     * Emulate . and ..
     */
    if (name[0] == '.' && name[1] != '.') {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "Lookup emulating \".\"\n");
#endif
        (void)vref(dir);
        *result = dir;

        kfree(in_fpath);
        return 0;
    } else if (name[0] == '.' && name[1] == '.' && name[2] != '.') {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "Lookup emulating \"..\"\n");
#endif
        if (VN_IS_FSROOT(dir)) {
            *result = dir->sb->mountpoint;

            kfree(in_fpath);
            return -EDOM;
        } else {
            size_t i = strlenn(in_fpath, NAME_MAX) - 4;

            while (in_fpath[i] != '/')
                i--;
            in_fpath[i] = '\0';
        }
    }

    /*
     * Lookup from vfs_hash
     */
    vn_hash = hash32_str(in_fpath, 0);
    err = vfs_hash_get(
            dir->sb,        /* FS superblock */
            vn_hash,        /* Hash */
            &vn,            /* Retval */
            fatfs_vncmp,    /* Comparator */
            in_fpath        /* Compared fpath */
          );
    if (err) {
        retval = -EIO;
        goto fail;
    }
    if (vn) { /* found it in vfs_hash */
        *result = vn;
        retval = 0;
    } else { /* not cached */
        struct fatfs_inode * in;

#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "vn not in vfs_hash\n");
#endif

        /*
         * Create a inode and fetch data from the device.
         * This also vrefs.
         */
        err = create_inode(&in, sb, in_fpath, vn_hash, O_RDWR);
        if (err) {
            retval = err;
            goto fail;
        }

        in_fpath = NULL; /* shall not be freed. */
        *result = &in->in_vnode;
        retval = 0;
    }

fail:
    kfree(in_fpath);
    return retval;
}

ssize_t fatfs_write(file_t * file, const void * buf, size_t count)
{
#ifdef configFATFS_READONLY
    return -ENOTSUP;
#else
    struct fatfs_inode * in = get_inode_of_vnode(file->vnode);
    size_t count_out;
    int err;
    ssize_t retval;

    if (!S_ISREG(file->vnode->vn_mode))
        return -EOPNOTSUPP;

    err = f_lseek(&in->fp, file->seek_pos);
    if (err)
        return -EIO;

    err = f_write(&in->fp, buf, count, &count_out);
    if (err)
        return fresult2errno(err);

    file->seek_pos = f_tell(&in->fp);
    retval = count_out;

    return retval;
#endif
}

ssize_t fatfs_read(file_t * file, void * buf, size_t count)
{
    struct fatfs_inode * in = get_inode_of_vnode(file->vnode);
    size_t count_out;
    int err;
    ssize_t retval;

    if (!S_ISREG(file->vnode->vn_mode))
        return -EOPNOTSUPP;

    err = f_lseek(&in->fp, file->seek_pos);
    if (err)
        return -EIO;

    err = f_read(&in->fp, buf, count, &count_out);
    if (err)
        return fresult2errno(err);

    file->seek_pos = f_tell(&in->fp);
    retval = count_out;

    return retval;
}

int fatfs_create(vnode_t * dir, const char * name, mode_t mode,
                 vnode_t ** result)
{
    return fatfs_mknod(dir, name, (mode & ~S_IFMT) | S_IFREG, NULL, result);
}

int fatfs_unlink(vnode_t * dir, const char * name)
{
#if !configFATFS_READONLY
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    char * in_fpath;
    FRESULT err;
    int retval;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    /* TODO May need checks if file/dir is opened */

    err = f_unlink(in_fpath);
    if (err)
        retval = fresult2errno(err);
    else
        retval = 0;

    kfree(in_fpath);
    return retval;
#else
    return -EROFS;
#endif
}

int fatfs_mknod(vnode_t * dir, const char * name, int mode, void * specinfo,
                vnode_t ** result)
{
#if !configFATFS_READONLY
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    struct fatfs_inode * res = NULL;
    char * in_fpath;
    int err;

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG,
           "fatfs_mknod(dir %p, name \"%s\", mode %u, specinfo %p, result %p)\n",
           dir, name, mode, specinfo, result);
#endif

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    if ((mode & S_IFMT) != S_IFREG)
        return -ENOTSUP; /* FAT only suports regular files. */

    if (specinfo)
        return -EINVAL; /* specinfo not supported. */

    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    err = create_inode(&res, get_ffsb_of_sb(dir->sb), in_fpath,
                       hash32_str(in_fpath, 0), O_CREAT);
    if (err) {
        kfree(in_fpath);
        return fresult2errno(err);
    }

    if (result)
        *result = &res->in_vnode;
    fatfs_chmod(*result, mode);

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "mkdod() ok\n");
#endif

    return 0;
#else
    return -EROFS;
#endif
}

int fatfs_mkdir(vnode_t * dir,  const char * name, mode_t mode)
{
#if !configFATFS_READONLY
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    char * in_fpath;
    FRESULT err;
    int retval;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    in_fpath = format_fpath(indir, name);
    if (!in_fpath)
        return -ENOMEM;

    err = f_mkdir(in_fpath);
    if (err)
        retval = fresult2errno(err);
    else
        retval = 0;

    kfree(in_fpath);
    return retval;
#else
    return -EROFS;
#endif
}

int fatfs_rmdir(vnode_t * dir,  const char * name)
{
    int err;
    vnode_t * result;
    mode_t mode;
    FRESULT ferr;

    /* TODO Should fail if name is a mount point */

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    err = fatfs_lookup(dir, name, &result);
    if (err)
        return err;
    mode = result->vn_mode;
    vrele_nunlink(result);
    if (!S_ISDIR(mode))
        return -ENOTDIR;

    ferr = fatfs_unlink(dir, name);

    return fresult2errno(err);
}

int fatfs_readdir(vnode_t * dir, struct dirent * d, off_t * off)
{
    struct fatfs_inode * in = get_inode_of_vnode(dir);
    FILINFO fno;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    /* Emulate . and .. */
    if (*off == DIRENT_SEEK_START) {
        f_readdir(&in->dp, NULL); /* Rewind */

        strlcpy(d->d_name, ".", sizeof(d->d_name));
        d->d_ino = dir->vn_num;
        d->d_type = DT_DIR;
        *off = DIRENT_SEEK_START + 1;

        return 0;
    } else if (*off == DIRENT_SEEK_START + 1) {
        strlcpy(d->d_name, "..", sizeof(d->d_name));
        /* TODO Set d members to proper values */
        d->d_ino = 0; /* TODO ino should be properly set */
        d->d_type = DT_DIR;
        *off = DIRENT_SEEK_START + 2;

        return 0;
    }

#if configFATFS_LFN
    fno.lfname = d->d_name;
    fno.lfsize = NAME_MAX + 1;
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
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    FILINFO fno;
    struct fatfs_sb * fat_sb = get_ffsb_of_sb(vnode->sb);
    struct stat mp_stat = { .st_uid = 0, .st_gid = 0 };
    size_t blksize = fat_sb->ff_fs.ssize;
    int err;

    memset(&fno, 0, sizeof(fno));

    err = get_mp_stat(vnode, &mp_stat);
    if (err) {
        if (err == -EINPROGRESS) {
#ifdef configFATFS_DEBUG
            KERROR(KERROR_WARN, "vnode->sb->mountpoint should be set\n");
#endif
        } else {
#ifdef configFATFS_DEBUG
            KERROR(KERROR_WARN,
                   "get_mp_stat() returned error (%d)\n",
                   err);
#endif

            return err;
        }
    }

    /* Can't stat FAT root */
    if (vnode == vnode->sb->root) {
        memcpy(buf, &mp_stat, sizeof(struct stat));

        return 0;
    }

    err = f_stat(in->in_fpath, &fno);
    if (err) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG,
                 "f_stat(fpath \"%s\", fno %p) failed\n",
                 in->in_fpath, &fno);
#endif
        return fresult2errno(err);
    }

    buf->st_dev = vnode->sb->vdev_id;
    buf->st_ino = vnode->vn_num;
    buf->st_mode = vnode->vn_mode;
    buf->st_nlink = 1; /* Always one link on FAT. */
    buf->st_uid = mp_stat.st_uid;
    buf->st_gid = mp_stat.st_gid;
    buf->st_size = fno.fsize;
    /* TODO Times */
#if 0
    buf->st_atime;
    buf->st_mtime;
    buf->st_ctime;
    buf->st_birthtime;
#endif
    buf->st_flags = fattrib2uflags(fno.fattrib);
    buf->st_blksize = blksize;
    buf->st_blocks = fno.fsize / blksize + 1; /* Best guess. */

    return 0;
}

int fatfs_chmod(vnode_t * vnode, mode_t mode)
{
#if !configFATFS_READONLY
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    BYTE attr = 0;
    const BYTE mask = AM_RDO;
    FRESULT fresult;

    if (!(mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
        attr |= AM_RDO;

    fresult = f_chmod(in->in_fpath, attr, mask);

    return fresult2errno(fresult);
#else
    return -EROFS;
#endif
}

/*
 * Note: Thre is practically two ways to set AM_RDO, either
 * by using chmod() or by this chflags().
 */
int fatfs_chflags(vnode_t * vnode, fflags_t flags)
{
#if !configFATFS_READONLY
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    BYTE attr = 0;
    const BYTE mask = AM_RDO | AM_ARC | AM_SYS | AM_HID;
    FRESULT fresult;

    if (flags & UF_SYSTEM)
        attr |= AM_SYS;
    if (flags & UF_ARCHIVE)
        attr |= AM_ARC;
    if (flags & UF_READONLY)
        attr |= AM_RDO;
    if (flags & UF_HIDDEN)
        attr |= AM_HID;

    fresult = f_chmod(in->in_fpath, attr, mask);

    return fresult2errno(fresult);
#else
    return -EROFS;
#endif
}

/**
 * Initialize fatfs vnode data.
 * @param vnode is the target vnode to be initialized.
 */
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             long vn_hash, struct fs_superblock * sb)
{
    struct stat stat;

#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG,
           "init_fatfs_vnode(vnode %p, inum %l, mode %o, vn_hash %u, sb %p)\n",
           vnode, (uint64_t)inum, mode, (uint32_t)vn_hash, sb);
#endif

    fs_vnode_init(vnode, inum, sb, &fatfs_vnode_ops);

    vnode->vn_hash = vn_hash;
    /* TODO Correct modes */
    if (S_ISDIR(mode))
        mode |= S_IRWXU | S_IXGRP | S_IXOTH;
    vnode->vn_mode = mode | S_IRUSR | S_IRGRP | S_IROTH;

    fatfs_stat(vnode, &stat);
    vnode->vn_len = stat.st_size;
}

/**
 * Get mountpoint stat.
 */
static int get_mp_stat(vnode_t * vnode, struct stat * st)
{
     struct fs_superblock * sb;
     vnode_t * mp;

#ifdef configFATFS_DEBUG
    KASSERT(vnode, "Vnode was given");
    KASSERT(vnode->sb, "Superblock is set");
#endif

    sb = vnode->sb;
    mp = sb->mountpoint;

    if (!mp) {
        /* We are probably mounting and mountpoint is not yet set. */
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "mp not set\n");
#endif
        return -EINPROGRESS;
    }

#ifdef configFATFS_DEBUG
    KASSERT(mp->vnode_ops->stat, "stat() is defined");
#endif

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
