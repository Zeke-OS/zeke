/**
 *******************************************************************************
 * @file    fz_fs.c
 * @author  Olli Vanhoja
 * @brief   File System wrapper for FatFs.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <autoconf.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/hash.h>
#include <kinit.h>
#include <kstring.h>
#include <fs/vfs_hash.h>
#include <kmalloc.h>
#include <proc.h>
#include "fatfs.h"

static int fatfs_mount(const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb);
static char * format_fpath(struct fatfs_inode * indir, const char * name,
                           size_t name_len);
static int create_inode(struct fatfs_inode ** result, struct fatfs_sb * sb,
                        char * fpath, long vn_hash, int oflags);
static vnode_t * create_root(fs_superblock_t * sb);
static int fatfs_delete_vnode(vnode_t * vnode);
static int fatfs_lookup(vnode_t * dir, const char * name, size_t name_len,
                        vnode_t ** result);
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             long vn_hash, fs_superblock_t * sb);
static int get_mp_stat(vnode_t * vnode, struct stat * st);
static int fresult2errno(int fresult);

static struct fs fatfs_fs = {
    .fsname = FATFS_FSNAME,
    .mount = fatfs_mount,
    .umount = NULL,
    .sbl_head = 0
};

/**
 * Array of all FAT mounts for faster access.
 */
struct fatfs_sb ** fatfs_sb_arr;

const vnode_ops_t fatfs_vnode_ops = {
    .write = fatfs_write,
    .read = fatfs_read,
    .create = fatfs_create,
    .mknod = fatfs_mknod,
    .lookup = fatfs_lookup,
    .link = fatfs_link,
    .unlink = fatfs_unlink,
    .mkdir = fatfs_mkdir,
    .rmdir = fatfs_rmdir,
    .readdir = fatfs_readdir,
    .stat = fatfs_stat,
    .chmod = fatfs_chmod,
    .chown = fatfs_chown
};

GENERATE_INSERT_SB(fatfs_sb, fatfs_fs)

int fatfs_init(void) __attribute__((constructor));
int fatfs_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("fatfs");

    fatfs_sb_arr = kcalloc(configFATFS_MAX_MOUNTS, sizeof(struct fatfs_sb *));
    if (!fatfs_sb_arr)
        return -ENOMEM;

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
    fs_superblock_t * sbp;
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

    sbp = &(fatfs_sb->sbn.sbl_sb);
    fs_fildes_set(&fatfs_sb->ff_devfile, vndev, O_RDWR);
    sbp->vdev_id = DEV_MMTODEV(FATFS_VDEV_MAJOR_ID, fatfs_vdev_minor++);

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
    sbp->fs = &fatfs_fs;
    sbp->mode_flags = mode;
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

    fatfs_sb->sbn.next = NULL; /* SB list */

    /* Add this sb to the list of mounted file systems. */
    insert_superblock(fatfs_sb);

    goto out;
fail:
    fatfs_sb_arr[DEV_MINOR(sbp->vdev_id)] = NULL;
    kfree(fatfs_sb);
out:
    *sb = &(fatfs_sb->sbn.sbl_sb);
    return retval;
}

/**
 * Get kmalloc'd array for full path name.
 */
static char * format_fpath(struct fatfs_inode * indir, const char * name,
                           size_t name_len)
{
    char * fpath;
    size_t fpath_size;
#ifdef configFATFS_DEBUG
    char msgbuf[80];

    ksprintf(msgbuf, sizeof(msgbuf),
             "format_fpath(indir \"%s\", name \"%s\", name_len %u)\n",
             indir->in_fpath, name, name_len);
    KERROR(KERROR_DEBUG, msgbuf);
#endif


    fpath_size = name_len + strlenn(indir->in_fpath, NAME_MAX + 1) + 6;
    fpath = kmalloc(fpath_size);
    if (!fpath)
        return NULL;

    ksprintf(fpath, fpath_size, "%s/%s", indir->in_fpath, name);

#ifdef configFATFS_DEBUG
    ksprintf(msgbuf, sizeof(msgbuf), "Formatted \"%s\" as \"%s\"\n",
             name, fpath);
    KERROR(KERROR_DEBUG, msgbuf);
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
    ino_t num;
    int err = 0, retval = 0;
#ifdef configFATFS_DEBUG
    char msgbuf[80];

    ksprintf(msgbuf, sizeof(msgbuf), "create_inode(fpath \"%s\", vn_hash %u)\n",
             fpath, (uint32_t)vn_hash);
    KERROR(KERROR_DEBUG, msgbuf);
#endif

    in = kcalloc(1, sizeof(struct fatfs_inode));
    if (!in) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "ENOMEM\n");
#endif
        retval = -ENOMEM;
        goto fail;
    }
    in->in_fpath = fpath;
    vn = &in->in_vnode;

    if (oflags & O_DIRECTORY) {
        /* O_DIRECTORY was specified. */
        /* TODO Maybe get mp stat? */
        fno.fattrib = AM_DIR;
    } else if (oflags & O_CREAT) {
        /* NOOP */
    } else {
        err = f_stat(fpath, &fno);
        if (err)
            goto chk_err;
    }

    /* Try open */
    if (fno.fattrib & AM_DIR) {
        /* it's a directory */
        vn_mode = S_IFDIR;
        err = f_opendir(&in->dp, in->in_fpath);
    } else {
        /* it's a file */
        unsigned char fomode = 0;

        fomode |= (oflags & O_CREAT) ? FA_OPEN_ALWAYS : FA_OPEN_EXISTING;
        fomode |= (oflags & O_RDONLY) ? FA_READ : 0;
        fomode |= (oflags & O_WRONLY) ? ((fno.fattrib & AM_RDO) ?
                                         0 : FA_WRITE) : 0 ;

        vn_mode = S_IFREG;
        err = f_open(&in->fp, in->in_fpath, fomode);
    }
chk_err:
    if (err) {
        retval = fresult2errno(err);
        goto fail;
    }
#ifdef configFATFS_DEBUG
    KERROR(KERROR_DEBUG, "Open ok\n");
#endif

    num = sb->ff_ino++;
    init_fatfs_vnode(vn, num, vn_mode, vn_hash, &(sb->sbn.sbl_sb));

    /* Insert to cache */
    err = vfs_hash_insert(vn, vn_hash, &xvp, fatfs_vncmp, fpath);
    if (err) {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "ENOMEM\n");
#endif
        retval = -ENOMEM;
        goto fail;
    }
    if (xvp) {
        /* TODO No idea what to do now */
        ksprintf(msgbuf, sizeof(msgbuf),
                "create_inode(): Found it during insert: \"%s\"\n",
                fpath);
        KERROR(KERROR_WARN, msgbuf);
    }

    *result = in;
    vref(vn);
    return 0;
fail:
#ifdef configFATFS_DEBUG
    ksprintf(msgbuf, sizeof(msgbuf), "create_inode(): err %i, retval %i\n",
             err, retval);
    KERROR(KERROR_DEBUG, msgbuf);
#endif

    kfree(in);
    return retval;
}

static vnode_t * create_root(fs_superblock_t * sb)
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

    return &in->in_vnode;
}

static int fatfs_delete_vnode(vnode_t * vnode)
{
    /* TODO */
    return -ENOTSUP;
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
static int fatfs_lookup(vnode_t * dir, const char * name, size_t name_len,
                        vnode_t ** result)
{
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    struct fatfs_sb * sb = get_ffsb_of_sb(dir->sb);
    char * in_fpath;
    long vn_hash;
    struct vnode * vn = NULL;
    /* Following variables are used only if not cached. */
    struct fatfs_inode * in;
    /* --- */
    int err, retval = 0;

    /* Format full path */
    in_fpath = format_fpath(indir, name, name_len);
    if (!in_fpath)
        return -ENOMEM;

    /*
     * Emulate . and ..
     */
    if (name[0] == '.' && name[1] != '.') {
#ifdef configFATFS_DEBUG
        KERROR(KERROR_DEBUG, "Lookup emulating \".\"\n");
#endif
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
            return 0;
        } else {
            size_t i = strlenn(in_fpath, NAME_MAX);

            while (in_fpath[i] != '/') i--;
            in_fpath[i] = '\0';
        }
    }

    /* Lookup from vfs_hash */
    vn_hash = hash32_str(in_fpath, 0);
    err = vfs_hash_get(
            dir->sb,        /* FS superblock */
            vn_hash,        /* Hash */
            &vn,            /* Retval */
            fatfs_vncmp,    /* Comparer */
            in_fpath        /* Compared fpath */
          );
    if (err) {
        retval = -EIO;
        goto fail;
    }
    if (vn) {
        /* Found it */
        kfree(in_fpath);
        vref(vn);
        *result = vn;

        return 0;
    }

    /*
     * Create a inode and fetch data from the device.
     */
    err = create_inode(&in, sb, in_fpath, vn_hash, O_RDWR);
    if (err) {
        retval = err;
        goto fail;
    }
    /* Vn is already referenced so just return. */
    *result = &in->in_vnode;

    return 0;
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

int fatfs_create(vnode_t * dir, const char * name, size_t name_len, mode_t mode,
                 vnode_t ** result)
{
    return fatfs_mknod(dir, name, name_len, (mode & ~S_IFMT) | S_IFREG, NULL,
                       result);
}

int fatfs_mknod(vnode_t * dir, const char * name, size_t name_len, int mode,
                void * specinfo, vnode_t ** result)
{
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    struct fatfs_inode * res = NULL;
    char * in_fpath;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    if ((mode & S_IFMT) != S_IFREG)
        return -ENOTSUP; /* FAT only suports regular files. */

    if (specinfo)
        return -EINVAL; /* specinfo not supported. */


    in_fpath = format_fpath(indir, name, name_len);
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

    return 0;
}

int fatfs_link(vnode_t * dir, vnode_t * vnode, const char * name,
               size_t name_len)
{
    return -ENOTSUP;
}

int fatfs_unlink(vnode_t * dir, const char * name, size_t name_len)
{
    return -ENOTSUP;
}

int fatfs_mkdir(vnode_t * dir,  const char * name, size_t name_len, mode_t mode)
{
    return -ENOTSUP;
}

int fatfs_rmdir(vnode_t * dir,  const char * name, size_t name_len)
{
    return -ENOTSUP;
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

        strlcpy(d->d_name, ".", NAME_MAX);
        d->d_ino = dir->vn_num;
        d->d_type = DT_DIR;
        *off = DIRENT_SEEK_START + 1;

        return 0;
    } else if (*off == DIRENT_SEEK_START + 1) {
        strlcpy(d->d_name, "..", NAME_MAX);
        /* TODO */
        d->d_ino = 0;
        d->d_type = DT_DIR;
        *off = DIRENT_SEEK_START + 2;

        return 0;
    }

#if configFATFS_USE_LFN
    fno.lfname = d->d_name;
    fno.lfsize = NAME_MAX + 1;
#endif

    err = f_readdir(&in->dp, &fno);
    if (err)
        return fresult2errno(err);

    if (fno.fname[0] == '\0')
        return -ESPIPE;

    d->d_ino = 0; /* TODO */
    d->d_type = (fno.fattrib & AM_DIR) ? DT_DIR : DT_REG;
#if configFATFS_USE_LFN
    if (!*fno.lfname)
#endif
        strlcpy(d->d_name, fno.fname, 13);

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

    err = get_mp_stat(vnode, &mp_stat);
    if (err) {
        if (err == -EINPROGRESS) {
#ifdef configFATFS_DEBUG
            KERROR(KERROR_WARN, "vnode->sb->mountpoint should be set\n");
#endif
        } else {
#ifdef configFATFS_DEBUG
            char msgbuf[60];

            ksprintf(msgbuf, sizeof(msgbuf),
                     "get_mp_stat() returned error (%d)\n",
                     err);
            KERROR(KERROR_WARN, msgbuf);
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
        char msgbuf[80];

        ksprintf(msgbuf, sizeof(msgbuf),
                 "f_stat(fpath \"%s\", fno %p) failed\n",
                 in->in_fpath, &fno);
        KERROR(KERROR_DEBUG, msgbuf);
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
    /* TODO */
    return -ENOTSUP;
}

int fatfs_chown(vnode_t * vnode, uid_t owner, gid_t group)
{
    return -ENOTSUP;
}

/**
 * Initialize fatfs vnode data.
 * @param vnode is the target vnode to be initialized.
 */
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             long vn_hash, fs_superblock_t * sb)
{
    struct stat stat;
#ifdef configFATFS_DEBUG
    char msgbuf[120];

    ksprintf(msgbuf, sizeof(msgbuf),
             "init_fatfs_vnode(vnode %p, inum %l, mode %o, vn_hash %u, sb %p)\n",
             vnode, (uint64_t)inum, mode, (uint32_t)vn_hash, sb);
    KERROR(KERROR_DEBUG, msgbuf);
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
