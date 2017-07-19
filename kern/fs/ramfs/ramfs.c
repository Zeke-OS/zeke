/**
 *******************************************************************************
 * @file    ramfs.c
 * @author  Olli Vanhoja
 * @brief   ramfs - a temporary file system stored in RAM.
 * @section LICENSE
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

#include <dirent.h>
#include <errno.h>
#include <machine/atomic.h>
#include <stdint.h>
#include <sys/dev_major.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <kerror.h>
#include <kinit.h>
#include <libkern.h>
#include <kstring.h>
#include <kmalloc.h>
#include <buf.h>
#include <proc.h>
#include <fs/dehtable.h>
#include <fs/inpool.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/vfs_hash.h>
#include <fs/ramfs.h>

/**
 * inode pool size.
 * Defines maximum (and initial) size of inode pool
 * and initial size of inode array.
 */
#define RAMFS_INODE_POOL_SIZE   ((configRAMFS_DESIREDVNODES >> 3) + 5)

#define RFS_DOT "."
#define RFS_DOTDOT ".."

/**
 * inode struct.
 */
typedef struct ramfs_inode {
    vnode_t     in_vnode;   /*!< vnode for this inode. */
    nlink_t     in_nlink;   /*!< Number of links to the file. */
    uid_t       in_uid;     /*!< User ID of file. */
    gid_t       in_gid;     /*!< Group ID of file. */
    struct timespec in_atime;   /*!< Time of last access. */
    struct timespec in_mtime;   /*!< Time of last data modification. */
    struct timespec in_ctime;   /*!< Time of last status change. */
    struct timespec in_birthtime;
    blksize_t   in_blksize; /*!< Preferred I/O block size for this object.
                                 This is allowed to vary from file to file. */
    blkcnt_t    in_blocks;  /*!< Number of blocks allocated for this object. */

    union {
        /**
         * Data array.
         * This is a pointer to an array of pointer pointing to the actual
         * blocks of data. Blocks are fragments of data of the stored file.
         * in_blksize and in_blocks can be used to calculate the size of this
         * file. The size derived from those variables might not correspond to
         * the size indicated by in_vnode->len but the size is always at least
         * in_vnode->len in case of ramfs.
         */
        struct buf ** data;
        dh_table_t * dir;
    } in;
    rwlock_t in_lock;
} ramfs_inode_t;

/**
 * ramfs superblock struct.
 */
typedef struct ramfs_sb {
    struct fs_superblock sb;            /*!< Superblock node. */
    inpool_t ramfs_ipool;               /*!< inode pool. */
    ino_t next_inum;                    /*!< Next free inode number. */
    atomic_t nr_inodes;
    int ramfs_flags;
} ramfs_sb_t;

#define RAMFS_SB_FLAG_DYING 0x1    /*!< SB is being umounted */

#define RAMFS_SB_IS_HEALTHY(_x_) \
    (!(((_x_)->ramfs_flags & RAMFS_SB_FLAG_DYING) == RAMFS_SB_FLAG_DYING))

/**
 * Data pointer.
 * Data pointer to a block of data stored in vnode (regular file).
 */
struct ramfs_dp {
    char * p;   /*!< Pointer to a data in file. */
    size_t len; /*!< Length of block pointed by p. */
};

/* Private */
static void ramfs_init_sb(fs_t * fs, ramfs_sb_t * ramfs_sb, uint32_t mode);
static vnode_t * create_root(ramfs_sb_t * ramfs_sb);
static void destroy_superblock(ramfs_sb_t * ramfs_sb);
static vnode_t * ramfs_raw_create_inode(const struct fs_superblock * sb);
static void init_inode(ramfs_inode_t * inode, ramfs_sb_t * ramfs_sb,
                       ino_t * num);
static void destroy_vnode(vnode_t * vnode);
static void destroy_inode(ramfs_inode_t * inode);
static void destroy_inode_data(ramfs_inode_t * inode);
static int insert_inode(ramfs_inode_t * inode);
static struct ramfs_dp get_dp_by_offset(ramfs_inode_t * inode, off_t offset);

/**
 * Get the vnode struct linked to a vnode number.
 * @param[in] sb        is the superblock.
 * @param[in] vnode_num is the vnode number.
 * @param[out] vnode    is a pointer to the vnode, can be NULL.
 * @return Returns 0 if no error; Otherwise value other than zero.
 */
static int ramfs_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                           struct vnode ** vnode);


/**
 * Get ramfs_sb of a generic superblock that belongs to ramfs.
 * @param _sb_ is a pointer to a superblock pointing some ramfs mount.
 * @return Returns a pointer to the ramfs_sb ob of the sb.
 */
#define get_rfsb_of_sb(_sb_) \
    (containerof(_sb_, ramfs_sb_t, sb))

/**
 * Get corresponding inode of given vnode.
 * @param vn    is a pointer to a vnode.
 * @return Returns a pointer to the inode.
 */
#define get_inode_of_vnode(vn) \
    (containerof(vn, ramfs_inode_t, in_vnode))

/**
 * Vnode operations implemented for ramfs.
 */
vnode_ops_t ramfs_vnode_ops = {
    .read = ramfs_read,
    .write = ramfs_write,
    .event_vnode_opened = ramfs_event_vnode_opened,
    .create = ramfs_create,
    .mknod = ramfs_mknod,
    .lookup = ramfs_lookup,
    .revlookup = ramfs_revlookup,
    .link = ramfs_link,
    .unlink = ramfs_unlink,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .readdir = ramfs_readdir,
    .stat = ramfs_stat,
    .chmod = ramfs_chmod,
    .chown = ramfs_chown
};

static atomic_t ramfs_vdev_minor = ATOMIC_INIT(0);
static vfs_hash_ctx_t vfs_hash_ctx; /*!< vfs_hash context. */
static uint32_t ramfs_siphash_key[2];

int __kinit__ ramfs_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("ramfs");

    ramfs_siphash_key[0] = krandom();
    ramfs_siphash_key[1] = krandom();
    vfs_hash_ctx = vfs_hash_new_ctx("ramfs", configRAMFS_DESIREDVNODES,
                                    NULL /* No comparator needed */);
    if (!vfs_hash_ctx)
        return -ENOMEM;

    /*
     * This must be static as it's referenced and used in the file system via
     * the fs object system.
     */
    static fs_t ramfs_fs = {
        .fsname = RAMFS_FSNAME,
        .fs_majornum = VDEV_MJNR_RAMFS,
        .mount = ramfs_mount,
        .sblist_head = SLIST_HEAD_INITIALIZER(),
    };

    fs_inherit_vnops(&ramfs_vnode_ops, &nofs_vnode_ops);
    FS_GIANT_INIT(&ramfs_fs.fs_giant);
    fs_register(&ramfs_fs);

    return 0;
}

/*
 * Set initial values for timespec structs in inode.
 */
static void init_times(ramfs_inode_t * inode)
{
    struct timespec ts;

    getrealtime(&ts);

    inode->in_atime = ts;
    inode->in_mtime = ts;
    inode->in_ctime = ts;
    inode->in_birthtime = ts;
}

static void ramfs_vnode_accessed(vnode_t * vnode)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);

    if (!((vnode->sb->mode_flags & MNT_NOATIME) == MNT_NOATIME)) {
        getrealtime(&inode->in_atime);
    }
}

static void ramfs_vnode_modified(vnode_t * vnode)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);
    struct timespec ts;

    getrealtime(&ts);
    inode->in_mtime = ts;
    inode->in_ctime = ts;
}

static void ramfs_vnode_changed(vnode_t * vnode)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);
    struct timespec ts;

    getrealtime(&ts);
    inode->in_ctime = ts;
}

int ramfs_mount(fs_t * fs, const char * source, uint32_t mode,
                const char * parm, int parm_len, struct fs_superblock ** sb)
{
    ramfs_sb_t * ramfs_sb;
    int err, retval;

#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "%s()\n", __func__);
#endif

    ramfs_sb = kzalloc(sizeof(ramfs_sb_t));
    if (!ramfs_sb) {
        retval = -ENOMEM;
        goto out;
    }
    ramfs_init_sb(fs, ramfs_sb, mode);

    /* Initialize the inode pool. */
#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "Initialize the inode pool\n");
#endif
    err = inpool_init(&ramfs_sb->ramfs_ipool, &ramfs_sb->sb,
            ramfs_raw_create_inode, destroy_vnode, NULL,
            RAMFS_INODE_POOL_SIZE);
    if (err) {
        retval = -ENOMEM;
        goto free_ramfs_sb;
    }

    /* Set vdev number */
    dev_t vdev_id = atomic_inc(&ramfs_vdev_minor);
    ramfs_sb->sb.vdev_id =
        DEV_MMTODEV(VDEV_MJNR_RAMFS, vdev_id);
    /* Optimally this should be done in ramfs_init_sb (). */
    ramfs_sb->sb.sb_hashseed = vdev_id;

    /* Create the root inode */
#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "Create the root inode\n");
#endif
    ramfs_sb->sb.root = create_root(ramfs_sb);

    fs_insert_superblock(fs, &ramfs_sb->sb);

    retval = 0;
    goto out;
free_ramfs_sb:
    destroy_superblock(ramfs_sb);
out:
    *sb = &ramfs_sb->sb;
    return retval;
}

int ramfs_umount(struct fs_superblock * fs_sb)
{
    ramfs_sb_t * rsb = get_rfsb_of_sb(fs_sb);
    fs_t * fs = fs_sb->fs;
    mtx_t * lock = &fs->fs_giant;

    mtx_lock(lock);
    if (!RAMFS_SB_IS_HEALTHY(rsb)) {
        mtx_unlock(lock);
        return -EBUSY;
    }
    rsb->ramfs_flags = RAMFS_SB_FLAG_DYING;
    mtx_unlock(lock);

    /*
     * TODO Check that there is no more references to any vnodes of
     * this super block before destroying everything related to it.
     */
    fs_remove_superblock(fs, &rsb->sb);
    destroy_superblock(rsb); /* Destroy all data. */

    return 0;
}

int ramfs_statfs(struct fs_superblock * sb, struct statvfs * st)
{
    ramfs_sb_t * rsb = get_rfsb_of_sb(sb);
    const ino_t inodes_max = (ino_t)1 << (ino_t)(sizeof(size_t) * 8);
    const ino_t inodes_free = inodes_max - atomic_read(&rsb->nr_inodes);

    *st = (struct statvfs){
        .f_bsize = MMU_PGSIZE_COARSE,
        .f_frsize = MMU_PGSIZE_COARSE,
        .f_blocks = 0,
        .f_bfree = 0,
        .f_bavail = 0,
        .f_files = inodes_max,
        .f_ffree = inodes_free,
        .f_favail = inodes_free,
        .f_fsid = 0,
        .f_flag = sb->mode_flags,
        .f_namemax = NAME_MAX + 1,
    };
    strlcpy(st->fsname, sb->fs->fsname, sizeof(st->fsname));

    return 0;
}

static int ramfs_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                           vnode_t ** vnode)
{
    size_t vn_hash;
    struct vnode * vn = NULL;
    int err;

    vn_hash = halfsiphash32(vnode_num, sizeof(ino_t), ramfs_siphash_key);
    err = vfs_hash_get(vfs_hash_ctx, sb, vn_hash, &vn, NULL);
    if (err || !vn) {
#ifdef configRAMFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, NULL, "inode doesn't exist\n");
#endif
        return -ENOENT;
    }

    if (vnode) {
        if (vref(vn))
            return -ENOENT; /* vnode was removed during the op. */
        *vnode = vn;
    }

    return 0;
}

int ramfs_delete_vnode(vnode_t * vnode)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);
    vnode_t * vn_tmp;
    int refcount;

#ifdef configRAMFS_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, vnode, "%s(%u)\n",
                    __func__, (unsigned)vnode->vn_num);
#endif

    if (inode->in_nlink > 0) {
#ifdef configRAMFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, vnode, "\tNot removing, (nlink: %d)\n",
                        (int)inode->in_nlink);
#endif
        return 0;
    }

    vrele_nunlink(vnode);
    refcount = vrefcnt(&inode->in_vnode);
    if (refcount > 1) {
#ifdef configRAMFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, vnode, "\tNot removing, (refcount: %d)\n",
                        refcount);
#endif
        return 0;
    }

    destroy_inode_data(inode);
    vn_tmp = &inode->in_vnode;

    vfs_hash_remove(vfs_hash_ctx, vn_tmp);

    /* Recycle this inode */
    inpool_insert_clean(&(get_rfsb_of_sb(vn_tmp->sb)->ramfs_ipool), vn_tmp);

    return 0;
}

ssize_t ramfs_read(file_t * file, struct uio * uio, size_t count)
{
    size_t bytes_rd = 0;

    switch (file->vnode->vn_mode & S_IFMT) {
    case S_IFREG: /* file is a regular file. */
        bytes_rd = ramfs_rd_regular(file->vnode, &file->seek_pos, uio, count);
        break;
    case S_IFDIR:
        return -EISDIR;
    default: /* file type not supported. */
        return -EOPNOTSUPP;
    }

    file->seek_pos += bytes_rd;
    return bytes_rd;
}

ssize_t ramfs_write(file_t * file, struct uio * uio, size_t count)
{
    size_t bytes_wr = 0;

    switch (file->vnode->vn_mode & S_IFMT) {
    case S_IFREG: /* File is a regular file. */
        bytes_wr = ramfs_wr_regular(file->vnode, &file->seek_pos, uio, count);
        break;
    default: /* File type not supported. */
        return -EOPNOTSUPP;
    }

    ramfs_vnode_modified(file->vnode);

    file->seek_pos += bytes_wr;
    return bytes_wr;
}

int ramfs_event_vnode_opened(struct proc_info * p, vnode_t * vnode)
{
    ramfs_vnode_accessed(vnode);

    return 0;
}

static void init_inode_attr(ramfs_inode_t * inode, mode_t mode)
{
    inode->in_vnode.vn_mode = mode;
    inode->in_vnode.vn_len = 0;
    /* TODO other flags etc. */
#if 0
    inode->in_vnode->mutex =
#endif
    /* One ref for ramfs and one ref for the caller. */
    vrefset(&inode->in_vnode, 2);

    inode->in_nlink = 0;
    inode->in_uid = curproc->cred.euid;
    inode->in_gid = curproc->cred.egid; /* RFE or to egid of the parent dir */
    init_times(inode);
    inode->in_blocks = 0;
    inode->in_blksize = MMU_PGSIZE_COARSE;
}

int ramfs_create(vnode_t * dir, const char * name, mode_t mode,
                 vnode_t ** result)
{
    ramfs_sb_t * ramfs_sb;
    vnode_t * vnode;
    ramfs_inode_t * inode;
    int err;

#ifdef configRAMFS_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, dir, "%s(name \"%s\", mode %u)\n",
                    __func__, name, mode);
#endif

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    ramfs_sb = get_rfsb_of_sb(dir->sb);
    if (!RAMFS_SB_IS_HEALTHY(ramfs_sb))
        return -EROFS; /* fs is beign umounted or it's broken. */

    /*
     * Get a fresh inode for the file.
     */
    vnode = inpool_get_next(&ramfs_sb->ramfs_ipool);
    if (!vnode)
        return -ENOSPC;

    inode = get_inode_of_vnode(vnode);

    /* Init the file data section. */
    init_inode_attr(inode, S_IFREG | mode);
    err = ramfs_set_filesize(&inode->in_vnode, 1 * MMU_PGSIZE_COARSE);
    if (err) {
#ifdef configRAMFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, dir,
                        "ramfs_set_filesize() failed on inode creation\n");
#endif
        destroy_inode(inode);
        return err;
    }

    /* Create a directory entry. */
    insert_inode(inode); /* Insert into the lookup table of the super block. */
    err = ramfs_link(dir, vnode, name);
    if (err) { /* Hard link creation failed. */
#ifdef configRAMFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, dir,
                        "ramfs_link() failed on inode creation\n");
#endif
        destroy_inode(inode);
        return err;
    }

    ramfs_vnode_modified(dir);

    *result = vnode;
    return 0;
}

int ramfs_mknod(vnode_t * dir, const char * name, int mode, void * specinfo,
                vnode_t ** result)
{
    int err;

    err = ramfs_create(dir, name, mode, result);
    if (err)
        return err;

    (*result)->vn_mode = mode; /* ramfs_create() sets improper mode. */
    (*result)->vn_specinfo = specinfo;

    return 0;
}

int ramfs_lookup(vnode_t * dir, const char * name, vnode_t ** result)
{
    ramfs_inode_t * inode_dir;
    ino_t vnode_num;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    inode_dir = get_inode_of_vnode(dir);

    rwlock_rdlock(&inode_dir->in_lock);
    err = dh_lookup(inode_dir->in.dir, name, &vnode_num);
    rwlock_rdunlock(&inode_dir->in_lock);
    if (err)
        return -ENOENT; /* Link not found. */

    if (ramfs_get_vnode(dir->sb, &vnode_num, result)) {
        /* Translation from vnode_num to a vnode failed; Broken link? */
        return -ENOLINK;
    }

    if (*result == dir) {
        vrele(*result);
        return -EDOM;
    }

    return 0;
}

int ramfs_revlookup(vnode_t * dir, ino_t * ino, char * name, size_t name_len)
{
    ramfs_inode_t * inode_dir;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    inode_dir = get_inode_of_vnode(dir);

    rwlock_rdlock(&inode_dir->in_lock);
    err = dh_revlookup(inode_dir->in.dir, *ino, name, name_len);
    rwlock_rdunlock(&inode_dir->in_lock);

    return err;
}

int ramfs_link(vnode_t * dir, vnode_t * vnode, const char * name)
{
    ramfs_inode_t * inode_dir;
    ramfs_inode_t * inode;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

    inode_dir = get_inode_of_vnode(dir);
    inode = get_inode_of_vnode(vnode);

    rwlock_rdlock(&inode_dir->in_lock);
    err = dh_link(inode_dir->in.dir, vnode->vn_num,
                  IFTODT(vnode->vn_mode), name);
    rwlock_rdunlock(&inode_dir->in_lock);
    if (err)
        return err;

    ramfs_vnode_modified(dir);

    rwlock_wrlock(&inode_dir->in_lock);
    inode->in_nlink++; /* Increment the hard link count. */
    rwlock_wrunlock(&inode_dir->in_lock);

    return 0;
}

int ramfs_unlink(vnode_t * dir, const char * name)
{
    ramfs_inode_t * inode_dir;
    ramfs_inode_t * inode;
    vnode_t * vn;
    ino_t vnum;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    inode_dir = get_inode_of_vnode(dir);

    rwlock_rdlock(&inode_dir->in_lock);
    err = dh_lookup(inode_dir->in.dir, name, &vnum);
    rwlock_rdunlock(&inode_dir->in_lock);
    if (err)
        return err;

    err = ramfs_get_vnode(dir->sb, &vnum, &vn);
    if (err)
        return err;
    inode = get_inode_of_vnode(vn);

    /* Mandatory cleanup. */
    fs_vnode_cleanup(vn);

    rwlock_wrlock(&inode_dir->in_lock);
    err = dh_unlink(inode_dir->in.dir, name);
    rwlock_wrunlock(&inode_dir->in_lock);
    if (err)
        return err;

    ramfs_vnode_modified(dir);

    rwlock_wrlock(&inode_dir->in_lock);
    inode->in_nlink--; /* Decrement the hard link count. */
    rwlock_wrunlock(&inode_dir->in_lock);
    vrele_nunlink(vn);
    if (inode->in_nlink <= 0) {
        struct fs_superblock * sb = vn->sb;

        sb->delete_vnode(vn);
    }

    return 0;
}

int ramfs_mkdir(vnode_t * dir, const char * name, mode_t mode)
{
    ramfs_sb_t * ramfs_sb;
    vnode_t * vnode_new;
    ramfs_inode_t * inode_dir;
    ramfs_inode_t * inode_new;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    ramfs_sb = get_rfsb_of_sb(dir->sb);
    if (!RAMFS_SB_IS_HEALTHY(ramfs_sb))
        return -EROFS;

    vnode_new = inpool_get_next(&ramfs_sb->ramfs_ipool);
    if (!vnode_new)
        return -ENOSPC; /* Can't create a new dir. */
    inode_dir = get_inode_of_vnode(dir);
    inode_new = get_inode_of_vnode(vnode_new);

    init_inode_attr(inode_new, S_IFDIR | mode);

    /* Create a dh_table */
    inode_new->in.dir = kmalloc(sizeof(dh_table_t));
    if (!inode_new->in.dir) {
        destroy_inode(inode_new);
        return -ENOSPC; /* Cant allocate dh_table */
    }
    dh_init(inode_new->in.dir);

    /* Create links according to POSIX. */
    ramfs_link(&inode_new->in_vnode, &inode_new->in_vnode, RFS_DOT);
    ramfs_link(&inode_new->in_vnode, &inode_dir->in_vnode, RFS_DOTDOT);

    /* Insert the inode to the inode lookup table of its super block. */
    insert_inode(inode_new);
    err = ramfs_link(&inode_dir->in_vnode, vnode_new, name);
    if (err) {
        vrele_nunlink(vnode_new);
        ramfs_delete_vnode(vnode_new);
        return err;
    }

    ramfs_vnode_modified(dir);

    return 0;
}

int ramfs_rmdir(vnode_t * dir,  const char * name)
{
    ramfs_inode_t * inode_dir;
    ino_t vnum;
    vnode_t * vn;
    ramfs_inode_t * in;
    size_t nr_entries;
    int err;

#ifdef configRAMFS_DEBUG
    FS_KERROR_VNODE(KERROR_DEBUG, dir, "%s(dir %p, name \"%s\")\n",
                    __func__, dir, name);
#endif

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    inode_dir = get_inode_of_vnode(dir);

    rwlock_rdlock(&inode_dir->in_lock);
    err = dh_lookup(inode_dir->in.dir, name, &vnum);
    rwlock_rdunlock(&inode_dir->in_lock);
    if (err)
        return err;

    err = ramfs_get_vnode(dir->sb, &vnum, &vn);
    if (err)
        return err;
    in = get_inode_of_vnode(vn);
    vrele_nunlink(vn);

    if (vn->vn_next_mountpoint != vn)
        return -EBUSY; /* It's a mount point. */

    rwlock_rdlock(&inode_dir->in_lock);
    nr_entries = dh_nr_entries(in->in.dir);
    rwlock_rdunlock(&inode_dir->in_lock);
    if (nr_entries > 2) {
#ifdef configRAMFS_DEBUG
        FS_KERROR_VNODE(KERROR_DEBUG, &in->in_vnode,
                        "ENOTEMPTY (%u)\n", (unsigned)nr_entries);
#endif
        return -ENOTEMPTY;
    }

    rwlock_wrlock(&inode_dir->in_lock);
    dh_unlink(in->in.dir, RFS_DOT);
    dh_unlink(in->in.dir, RFS_DOTDOT);
    dh_unlink(inode_dir->in.dir, name);
    rwlock_wrunlock(&inode_dir->in_lock);

    ramfs_vnode_modified(dir);

    /* The following will call delete if the vnode should be deleted now. */
    vrele(vn);

    return 0;
}

int ramfs_readdir(vnode_t * dir, struct dirent * d, off_t * off)
{
    const off_t dea_ind_mask = 0x7FFFFFFF00000000;
    const off_t ch_ind_mask  = DIRENT_SEEK_START;
    dh_dir_iter_t it;
    dh_dirent_t * dh;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    /*
     * Dirent to iterator translation.
     * We assume here that off_t is a 64-bit signed integer, so we can store the
     * dea index to upper bits as it's definitely shorter than chain index which
     * will be the low 32-bits.
     * Note: For the first iteration ch_ind must be set to 0xFFFFFFFF.
     */
    it.dir = get_inode_of_vnode(dir)->in.dir;
    it.dea_ind = (*off & dea_ind_mask) >> 32;
    it.ch_ind  = (*off & ch_ind_mask);
    if (it.ch_ind == ch_ind_mask)
        it.ch_ind = SIZE_MAX; /* Just to make sure that the requirements of the
                               * iterator are met on systems with different
                               * architectures. (i.e. len of size_t) */

    dh = dh_iter_next(&it);
    if (!dh || dh->dh_size == 0)
        return -ESPIPE; /* End of dir. */

    /* Translate iterator back to dirent. */
    *off = ((((off_t)it.dea_ind) << 32) & dea_ind_mask) |
           (off_t)(it.ch_ind & ch_ind_mask);
    d->d_ino = dh->dh_ino;
    d->d_type = dh->dh_type;
    strlcpy(d->d_name, dh->dh_name, member_size(struct dirent, d_name));

    return 0;
}

int ramfs_stat(vnode_t * vnode, struct stat * buf)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);

    buf->st_dev     = vnode->sb->vdev_id;
    buf->st_ino     = vnode->vn_num;
    buf->st_mode    = vnode->vn_mode;
    buf->st_nlink   = inode->in_nlink;
    buf->st_uid     = inode->in_uid;
    buf->st_gid     = inode->in_gid;
    buf->st_rdev    = VNOVAL;
    buf->st_size    = vnode->vn_len;
    buf->st_atim    = inode->in_atime;
    buf->st_mtim    = inode->in_mtime;
    buf->st_ctim    = inode->in_ctime;
    buf->st_blksize = inode->in_blksize;
    buf->st_blocks  = inode->in_blocks;

    return 0;
}

int ramfs_chmod(vnode_t * vnode, mode_t mode)
{
    vnode->vn_mode = (vnode->vn_mode & S_IFMT) | (mode & ~S_IFMT);
    ramfs_vnode_changed(vnode);

    return 0;
}

int ramfs_chown(vnode_t * vnode, uid_t owner, gid_t group)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);

    inode->in_uid = owner;
    inode->in_gid = group;
    ramfs_vnode_changed(vnode);

    return 0;
}

/**
 * Intializes a ramfs superblock node.
 * @param fs        is a pointr to the file system initializing the sb.
 * @param ramfs_sb  is a pointer to a ramfs superblock.
 * @param mode      mount flags.
 */
static void ramfs_init_sb(fs_t * fs, ramfs_sb_t * ramfs_sb, uint32_t mode)
{
    struct fs_superblock * sb = &ramfs_sb->sb;

    fs_init_superblock(sb, fs);
    sb->mode_flags = mode;
    ramfs_sb->nr_inodes = ATOMIC_INIT(0);

    /* Function pointers to superblock methods: */
    sb->statfs = ramfs_statfs;
    sb->delete_vnode = ramfs_delete_vnode;
    sb->umount = ramfs_umount;
}

/**
 * Create a root node, set it as root and create dot and dotdot links for it.
 * @param ramfs_sb is the ramfs super block that needs a root.
 * @return Returns a pointer to the root vnode; Or null in case of error.
 */
static vnode_t * create_root(ramfs_sb_t * ramfs_sb)
{
    ramfs_inode_t * inode;
    vnode_t * vn;

    vn = inpool_get_next(&ramfs_sb->ramfs_ipool);
    if (!vn)
        return NULL; /* Can't create */
    inode = get_inode_of_vnode(vn);

    inode->in.dir = kmalloc(sizeof(dh_table_t)); /* Create a dh_table */
    if (!inode->in.dir) {
        destroy_inode(inode);
        return NULL;
    }
    dh_init(inode->in.dir);

    /* Root is a directory. */
    vn->vn_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    /* TODO Any other settings? */

    /* Insert inode to the inode lookup table of its superblock. */
    (void)insert_inode(inode); /* This can't fail on mount. */
    ramfs_sb->sb.root = vn;

    /* Create links according to POSIX. */
    ramfs_link(vn, vn, RFS_DOT);
    ramfs_link(vn, vn, RFS_DOTDOT);

    vrefset(vn, 2);

    return vn;
}

/**
 * Destroy a memory allocated for superblock and its inodes.
 * @note ramfs_sb is invalid after this call.
 * @param ramfs_sb Superblock to be freed.
 */
static void destroy_superblock(ramfs_sb_t * ramfs_sb)
{
    /*
     * NOTE: There shouldn't be any references to vnodes in this fs
     * anymore.
     */
    (void)vfs_hash_foreach(vfs_hash_ctx, &ramfs_sb->sb, destroy_vnode);

    /* Destroy inode pool */
    inpool_destroy(&ramfs_sb->ramfs_ipool);

    kfree(ramfs_sb);
}

/**
 * Create a new inode.
 * @param sb    is the superblock.
 * @param num   is the inode number.
 * @return Returns the newly created inode or null pointer if failed.
 */
static vnode_t * ramfs_raw_create_inode(const struct fs_superblock * sb)
{
    ramfs_inode_t * inode;
    ramfs_sb_t * ramfs_sb = get_rfsb_of_sb(sb);

    if (!RAMFS_SB_IS_HEALTHY(ramfs_sb))
        return NULL;

    inode = kzalloc(sizeof(ramfs_inode_t));
    if (!inode)
        return NULL;

    init_inode(inode, ramfs_sb, &ramfs_sb->next_inum);
    ramfs_sb->next_inum++;
    atomic_inc(&ramfs_sb->nr_inodes);

    return &inode->in_vnode;
}

/**
 * Intialize a ramfs_inode struct.
 * @param inode     is the struct that will be initialized.
 * @param ramfs_sb  is the current superblock.
 * @param num       is the inode number to be used.
 */
static void init_inode(ramfs_inode_t * inode, ramfs_sb_t * ramfs_sb,
                       ino_t * num)
{
    memset((void *)inode, 0, sizeof(ramfs_inode_t));
    rwlock_init(&inode->in_lock);
    fs_vnode_init(&inode->in_vnode, *num, &ramfs_sb->sb,
                  &ramfs_vnode_ops);
}

static void destroy_vnode(vnode_t * vnode)
{
    destroy_inode(get_inode_of_vnode(vnode));
}

/**
 * Destroy a ramfs_inode struct and its contents.
 * @note This should be normally called only if there is no more references and
 * links to the inode.
 * @param inode is the inode to be destroyed.
 */
static void destroy_inode(ramfs_inode_t * inode)
{
    ramfs_sb_t * ramfs_sb = get_rfsb_of_sb(inode->in_vnode.sb);

    atomic_dec(&ramfs_sb->nr_inodes);
    destroy_inode_data(inode);
    kfree(inode);
}

/**
 * Free all data associated with a inode.
 * Frees directory entries and data.
 * @param inode is a inode stored in ramfs.
 */
static void destroy_inode_data(ramfs_inode_t * inode)
{
    switch (inode->in_vnode.vn_mode & S_IFMT) {
    case S_IFREG:
        /* Free all data blocks. */
        ramfs_set_filesize(&inode->in_vnode, 0);
        break;
    case S_IFDIR:
        /* Free dhtable entries and dhtable. */
        if (inode->in.dir) {
            dh_destroy_all(inode->in.dir);
            kfree(inode->in.dir);
            inode->in.dir = NULL;
        }
        break;
    default: /* File type not supported or nothing to free. */
        break;
    }
}

/**
 * Insert inode to the lookup table.
 * @param inode is the inode to be inserted to the lookup table of its
 *              super block.
 * @return Returns 0 if inode was succesfully inserted to the lookup table;
 *         Otherwise value other than zero indicating an error occured.
 */
static int insert_inode(ramfs_inode_t * inode)
{
    size_t vn_hash;
    vnode_t * vp = NULL;
    int err;

    vn_hash = halfsiphash32(&inode->in_vnode.vn_num, sizeof(ino_t),
                            ramfs_siphash_key);
    err = vfs_hash_insert(vfs_hash_ctx, &inode->in_vnode, vn_hash, &vp, NULL);
    if (vp)
        return -EEXIST;
    return err;
}

/**
 * Transfers bytes from buf into a regular file.
 * Writing is begin from offset and ended at offset + count. buf must therefore
 * contain at least count bytes. If offset is past end of the current file the
 * file will be extended; If offset is smaller than file length, the existing
 * data will be overwriten.
 * @param file      is a regular file stored in ramfs.
 * @param offset    is the offset from SEEK_START.
 * @param buf       is a buffer where bytes are read from.
 * @param count     is the number of bytes buf contains.
 * @return Returns the number of bytes written.
 */
ssize_t ramfs_wr_regular(vnode_t * file, const off_t * restrict offset,
                         struct uio * uio, size_t count)
{
    ramfs_inode_t * inode = get_inode_of_vnode(file);
    const blksize_t blksize = inode->in_blksize;
    struct ramfs_dp dp;
    size_t bytes_wr = 0;

    /*
     * No file type check is needed as this function is called only for regular
     * files.
     */

    do {
        const size_t remain = count - bytes_wr;
        size_t curr_wr_len;
        int err;

        /* Get next block pointer. */
        dp = get_dp_by_offset(inode, *offset + bytes_wr);
        if (!dp.p) { /* Extend the file first. */
            /* Extend the file to the final size if possible. */
            if (ramfs_set_filesize(file, inode->in_blocks * blksize
                        + *offset + (off_t)count)) {
                break; /* Failed to extend the file. */
            } else { /* Try again to get a block. */
                continue;
            }
        }

        /*
         * Write bytes to the block.
         * Max per iteration is the size of the current block.
         */
        curr_wr_len = min(remain, dp.len);
        err = uio_copyin(uio, dp.p, bytes_wr, curr_wr_len);
        if (err)
            return err;
        bytes_wr += curr_wr_len;
    } while (bytes_wr < count);

    file->vn_len = max(file->vn_len, *offset + bytes_wr);
    return bytes_wr;
}

/**
 * Transfers bytes from a regular file into buf.
 * @param file      is a regular file.
 * @param offset    is the offset form SEEK_START.
 * @param count     is the requested number of bytes to be read.
 * @return Returns the number of bytes read from the file.
 */
ssize_t ramfs_rd_regular(vnode_t * file, const off_t * restrict offset,
                         struct uio * uio, size_t count)
{
    ramfs_inode_t * inode = get_inode_of_vnode(file);
    struct ramfs_dp dp;
    size_t bytes_rd = 0;

    /*
     * No file type check is needed as this function is called only for regular
     * files.
     */

    do {
        const size_t remain = count - bytes_rd;
        size_t curr_rd_len;
        int err;

        if ((*offset + bytes_rd) >= file->vn_len) {
            break; /* EOF */
        }

        /* Get next block pointer. */
        dp = get_dp_by_offset(inode, *offset + bytes_rd);
        if (!dp.p) {
            break; /* EOF */
        }

        /* Read bytes from the block. */
        curr_rd_len = min(remain, dp.len);
        err = uio_copyout(dp.p, uio, bytes_rd, curr_rd_len);
        if (err)
            return err;
        bytes_rd += curr_rd_len;
    } while (bytes_rd < count);

    return bytes_rd;
}

/**
 * Set file size.
 * Sets a new size for a regular file.
 * @param file      is the inode of a regular file.
 * @param new_size  is the new size of file.
 * @return Returns 0 if succeeded; Otherwise value other than zero.
 */
int ramfs_set_filesize(vnode_t * vnode, off_t new_size)
{
    ramfs_inode_t * file = get_inode_of_vnode(vnode);
    const blksize_t blksize = file->in_blksize;
    const off_t old_size = (off_t)file->in_blocks * (off_t)blksize;

    /* Calculate a block aligned new_size.
     * Some size_t casts to speed up the computation as ramfs doesn't support
     * larger files anyway. */
    new_size = new_size +
        (off_t)((size_t)blksize -
                (((size_t)new_size - 1) & ((size_t)blksize - 1)) - 1);

    if (new_size == old_size) { /* Same as old */
        return 0;
    } else if (new_size < old_size) { /* Truncate */
        if (new_size > 0) {
            /* TODO Free blocks to truncate the file */
        } else {  /* Free all blocks. */
            struct buf ** bpp = file->in.data;
            size_t i;

            for (i = 0; i < file->in_blocks; i++) {
                if (bpp[i])
                    vrfree(bpp[i]);
            }
            file->in.data = NULL;
            file->in_blocks = 0;
        }
    } else { /* Extend */
        struct buf ** new_data;
        const blkcnt_t new_blkcnt = new_size / blksize;
        size_t i;

        new_data = krealloc(file->in.data, sizeof(struct buf) * new_blkcnt);
        if (!new_data) {
            return -ENOMEM; /* Can't extend. */
        }
        file->in.data = new_data;

        /*
         * TODO It would be possibly more efficient for memory usage to allocate
         * new blocks just before writing but that'd require some refactoring.
         */

        /* Allocate new blocks. */
        for (i = file->in_blocks; i < new_blkcnt; i++) {
            new_data[i] = geteblk(blksize);
            if (!new_data[i]) { /* Can't alloc. */
                file->in_blocks += i;
                return -ENOMEM; /* Can't extend to the requested size. */
            }
        }
        file->in_blocks = new_blkcnt;
    }

    return 0;
}

/**
 * Get data pointer by given offset.
 * @note This function may return pointers that are pointing to a memory
 * location after the EOF.
 * @param inode     is a ramfs inode.
 * @param offset    is the offset of seek pointer.
 * @return Returns a struct that contains a pointer to the requested data and
 *         length of returned block. dp.p == NULL if request out of bounds.
 */
static struct ramfs_dp get_dp_by_offset(ramfs_inode_t * inode, off_t offset)
{
    const blksize_t blksize = inode->in_blksize;
    struct ramfs_dp dp = { .p = 0, .len = 0 }; /* Return value. */

    if (/* Out of bounds check. */
        !(offset >= ((off_t)inode->in_blocks * (off_t)blksize)) &&
        /* Data array is set. */
        inode->in.data) {
        size_t bi; /* Block index. */
        size_t di; /* Data index. */
        char * block; /* Pointer to the block. */

        bi = (size_t)((offset - (offset & ((off_t)(blksize - 1))))
                / (off_t)blksize);
        di = (size_t)(((size_t)offset) & (blksize - 1));
        block = (char *)(inode->in.data[bi]->b_data);

        /* Return value. */
        dp.p = &block[di];
        dp.len = blksize - di;
    }

    return dp;
}
