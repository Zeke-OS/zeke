/**
 *******************************************************************************
 * @file    ramfs.c
 * @author  Olli Vanhoja
 * @brief   ramfs - a temporary file system stored in RAM.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdint.h>
#include <sys/types.h>
#include <machine/atomic.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <kinit.h>
#include <libkern.h>
#include <kstring.h>
#include <kmalloc.h>
#include <buf.h>
#include <proc.h>
#include <fs/dehtable.h>
#include <fs/inpool.h>
#include <fs/dev_major.h>
#include <fs/ramfs.h>

/**
 * inode pool size.
 * Defines maximum (and initial) size of inode pool
 * and initial size of inode array.
 */
#define RAMFS_INODE_POOL_SIZE   10

/**
 * Maximum number of files in a single ramfs mount.
 */
#define RAMFS_MAX_FILES         SIZE_MAX

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
} ramfs_inode_t;

/**
 * ramfs superblock struct.
 */
typedef struct ramfs_sb {
    superblock_lnode_t sbn;             /*!< Superblock node. */
    struct ramfs_inode ** ramfs_iarr;   /*!< inode lookup table. */
    size_t ramfs_iarr_size;             /*!< Size of the iarr array. */
    inpool_t ramfs_ipool;               /*!< inode pool. */
} ramfs_sb_t;

/**
 * Data pointer.
 * Data pointer to a block of data stored in vnode (regular file).
 */
struct ramfs_dp {
    char * p;   /*!< Pointer to a data in file. */
    size_t len; /*!< Length of block pointed by p. */
};


static atomic_t ramfs_vdev_minor;

/* Private */
static void init_sbn(ramfs_sb_t * ramfs_sb, uint32_t mode);
static vnode_t * create_root(ramfs_sb_t * ramfs_sb);
static void insert_superblock(ramfs_sb_t * ramfs_sb);
static void remove_superblock(ramfs_sb_t * ramfs_sb);
static void destroy_superblock(ramfs_sb_t * ramfs_sb);
vnode_t * ramfs_raw_create_inode(const fs_superblock_t * sb, ino_t * num);
static void init_inode(ramfs_inode_t * inode, ramfs_sb_t * ramfs_sb,
                       ino_t * num);
static void destroy_vnode(vnode_t * vnode);
static void destroy_inode(ramfs_inode_t * inode);
static void destroy_inode_data(ramfs_inode_t * inode);
static int insert_inode(ramfs_inode_t * inode);
static ssize_t ramfs_wr_regular(vnode_t * file, const off_t * restrict offset,
                                const void * buf, size_t count);
static ssize_t ramfs_rd_regular(vnode_t * file, const off_t * restrict offset,
                                void * buff, size_t count);
static int ramfs_set_filesize(ramfs_inode_t * inode, off_t size);
static struct ramfs_dp get_dp_by_offset(ramfs_inode_t * inode, off_t offset);


/**
 * Get ramfs_sb of a generic superblock that belongs to ramfs.
 * @param sb    is a pointer to a superblock pointing some ramfs mount.
 * @return Returns a pointer to the ramfs_sb ob of the sb.
 */
#define get_rfsb_of_sb(sb) \
    (container_of(container_of(sb, superblock_lnode_t, sbl_sb), \
                  ramfs_sb_t, sbn))

/**
 * Get corresponding inode of given vnode.
 * @param vn    is a pointer to a vnode.
 * @return Returns a pointer to the inode.
 */
#define get_inode_of_vnode(vn) \
    (container_of(vn, ramfs_inode_t, in_vnode))

/**
 * fs struct for ramfs.
 */
fs_t ramfs_fs = {
    .fsname = RAMFS_FSNAME,
    .mount = ramsfs_mount,
    .umount = ramfs_umount,
    .sbl_head = 0
};

/**
 * Vnode operations implemented for ramfs.
 * @note Virtual function pointers not set here will be null pointers.
 */
const vnode_ops_t ramfs_vnode_ops = {
    .write = ramfs_write,
    .read = ramfs_read,
    .create = ramfs_create,
    .mknod = ramfs_mknod,
    .lookup = ramfs_lookup,
    .link = ramfs_link,
    .unlink = ramfs_unlink,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .readdir = ramfs_readdir,
    .stat = ramfs_stat,
    .chmod = ramfs_chmod,
    .chown = ramfs_chown
};

int ramfs_init(void) __attribute__((constructor));
int ramfs_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("ramfs");

    ATOMIC_INIT(ramfs_vdev_minor);

    /* Register ramfs with vfs. */
    fs_register(&ramfs_fs);

    return 0;
}

/**
 * Mount a new ramfs.
 * @param mode      mount flags.
 * @param param     contains optional mount parameters.
 * @param parm_len  length of param string.
 * @param[out] sb   Returns the superblock of the new mount.
 * @return error code, -errno.
 */
int ramsfs_mount(const char * source, uint32_t mode,
                 const char * parm, int parm_len, struct fs_superblock ** sb)
{
    ramfs_sb_t * ramfs_sb;
    int err, retval = 0;

#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "ramfs_mount()\n");
#endif

    ramfs_sb = kmalloc(sizeof(ramfs_sb_t));
    if (ramfs_sb == 0) {
        retval = -ENOMEM;
        goto out;
    }
    init_sbn(ramfs_sb, mode);

    /*
     * Allocate memory for the inode lookup table.
     * kcalloc is used here to clear all inode pointers.
     */
    ramfs_sb->ramfs_iarr = kcalloc(RAMFS_INODE_POOL_SIZE,
                                   sizeof(ramfs_inode_t *));
    if (ramfs_sb->ramfs_iarr == 0) {
        retval = -ENOMEM;
        goto free_ramfs_sb;
    }
    ramfs_sb->ramfs_iarr_size = 0;

    /* Initialize the inode pool. */
#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "Initialize the inode pool.\n");
#endif
    err = inpool_init(&(ramfs_sb->ramfs_ipool), &(ramfs_sb->sbn.sbl_sb),
            ramfs_raw_create_inode, destroy_vnode, RAMFS_INODE_POOL_SIZE);
    if (err) {
        retval = -ENOMEM;
        goto free_ramfs_sb;
    }

    /* Set vdev number */
    ramfs_sb->sbn.sbl_sb.vdev_id =
        DEV_MMTODEV(VDEV_MJNR_RAMFS, atomic_inc(&ramfs_vdev_minor));

    /* Create the root inode */
#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "Create the root inode\n");
#endif
    ramfs_sb->sbn.sbl_sb.root = create_root(ramfs_sb);

    /* Add this sb to the list of mounted file systems. */
    insert_superblock(ramfs_sb);

    goto out;
free_ramfs_sb:
    destroy_superblock(ramfs_sb);
out:
    *sb = &(ramfs_sb->sbn.sbl_sb);
    return retval;
}

/**
 * Unmount a ramfs.
 * @param fs_sb is the superblock to be unmounted.
 * @return Returns zero if succeed; Otherwise value other than zero.
 */
int ramfs_umount(struct fs_superblock * fs_sb)
{
    ramfs_sb_t * rsb = get_rfsb_of_sb(fs_sb);
    int retval = 0;

    /*
     * TODO Check for any locks
     * TODO Check that there is no more references to any vnodes of
     * this super block before destroying everyting related to it.
     */
    remove_superblock(rsb); /* Remove it from the mount list. */
    destroy_superblock(rsb); /* Destroy all data. */

    return retval;
}

/**
 * Get the vnode struct linked to a vnode number.
 * @param[in] sb        is the superblock.
 * @param[in] vnode_num is the vnode number.
 * @param[out] vnode    is a pointer to the vnode, can be NULL.
 * @return Returns 0 if no error; Otherwise value other than zero.
 */
int ramfs_get_vnode(fs_superblock_t * sb, ino_t * vnode_num, vnode_t ** vnode)
{
    ramfs_sb_t * ramfs_sb;

    if (strcmp(sb->fs->fsname, RAMFS_FSNAME))
        return -EINVAL;

    /* Get pointer to the ramfs_sb from generic sb. */
    ramfs_sb = get_rfsb_of_sb(sb);

    if (*vnode_num >= (ino_t)(ramfs_sb->ramfs_iarr_size))
        return -ENOENT; /* inode can't exist. */

    if (vnode) {
        struct ramfs_inode * in;
        struct vnode * vn;
        const size_t vnnum = (size_t)(*vnode_num);

        in = ramfs_sb->ramfs_iarr[vnnum];
        if (!in)
            return -ENOENT;

        vn = &in->in_vnode;
        if (vref(vn))
            return -ENOENT; /* vnode was removed during the op. */
        *vnode = vn;
    }

    return 0;
}

/**
 * Delete a vnode reference.
 * Deletes a reference to a vnode and destroys the inode corresponding to the
 * inode if there is no more links and references to it.
 * @param[in] vnode is the vnode.
 * @return Returns 0 if no error; Otherwise value other than zero.
 */
int ramfs_delete_vnode(vnode_t * vnode)
{
    ramfs_inode_t * inode;
    vnode_t * vn_tmp;

    inode = get_inode_of_vnode(vnode);

    if (inode->in_nlink > 0)
        return 0;

    if (vrefcnt(&inode->in_vnode) > 0)
        return 0;

#if configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "%s, ramfs_delete_vnode(%u)\n", vnode->sb->fs->fsname,
           (unsigned)vnode->vn_num);
#endif

    /* TODO Clear mutexes, queues etc. */
    destroy_inode_data(inode);
    vn_tmp = &(inode->in_vnode);

    /* Recycle this inode */
    inpool_insert(&(get_rfsb_of_sb(vn_tmp->sb)->ramfs_ipool), vn_tmp);

    return 0;
}

ssize_t ramfs_write(file_t * file, const void * buf, size_t count)
{
    size_t bytes_wr = 0;

    switch (file->vnode->vn_mode & S_IFMT) {
    case S_IFREG: /* File is a regular file. */
        bytes_wr = ramfs_wr_regular(file->vnode, &(file->seek_pos), buf, count);
        break;
    default: /* File type not supported. */
        return -EOPNOTSUPP;
    }

    file->seek_pos += bytes_wr;
    return bytes_wr;
}

ssize_t ramfs_read(file_t * file, void * buf, size_t count)
{
    size_t bytes_rd = 0;

    switch (file->vnode->vn_mode & S_IFMT) {
    case S_IFREG: /* File is a regular file. */
        bytes_rd = ramfs_rd_regular(file->vnode, &(file->seek_pos), buf, count);
        break;
    case S_IFDIR:
        return -EISDIR;
    default: /* File type not supported. */
        return -EOPNOTSUPP;
    }

    file->seek_pos += bytes_rd;
    return bytes_rd;
}

int ramfs_create(vnode_t * dir, const char * name, mode_t mode,
                 vnode_t ** result)
{
    ramfs_sb_t * ramfs_sb;
    vnode_t * vnode;
    ramfs_inode_t * inode;
    int retval = 0;

    if (!S_ISDIR(dir->vn_mode)) {
        retval = -ENOTDIR; /* No a directory entry. */
        goto out;
    }

    ramfs_sb = get_rfsb_of_sb(dir->sb);
    vnode = inpool_get_next(&(ramfs_sb->ramfs_ipool));
    if (!vnode) {
        retval = -ENOSPC; /* Can't create */
        goto out;
    }

    inode = get_inode_of_vnode(vnode);

    /* Insert inode to the inode lookup table of its superblock. */
    insert_inode(inode);

    /* Init file data section. */
    inode->in_blocks = 0; /* Will be set by ramfs_set_filesize() */
    inode->in_blksize = MMU_PGSIZE_COARSE;
    retval = ramfs_set_filesize(inode, 1 * MMU_PGSIZE_COARSE);
    if (retval != 0) {
#ifdef configRAMFS_DEBUG
        KERROR(KERROR_DEBUG, "ramfs_set_filesize() failed on inode creation\n");
#endif
        destroy_inode(inode);
        goto out;
    }

    vnode->vn_len = 0;
    vnode->vn_mode = S_IFREG | mode;
    inode->in_uid = curproc->euid;
    inode->in_gid = curproc->egid;
    /* TODO update times */
    //vnode->mutex = /* TODO other flags etc. */

    /* Create a directory entry. */
    retval = ramfs_link(dir, vnode, name);
    if (retval != 0) { /* Hard link creation failed. */
#ifdef configRAMFS_DEBUG
        KERROR(KERROR_DEBUG, "ramfs_link() failed on inode creation\n");
#endif
        destroy_inode(inode);
        goto out;
    }

    *result = vnode;
    vrefset(*result, 2); /* One ref for ramfs, one ref for caller. */
out:
    return retval;
}

int ramfs_mknod(vnode_t * dir, const char * name, int mode, void * specinfo,
                vnode_t ** result)
{
    int retval;

    retval = ramfs_create(dir, name, mode, result);
    if (retval)
        return retval;

    (*result)->vn_mode = mode; /* ramfs_create() sets improper mode. */
    (*result)->vn_specinfo = specinfo;

    return 0;
}

int ramfs_lookup(vnode_t * dir, const char * name, vnode_t ** result)
{
    dh_table_t * dh_dir;
    ino_t vnode_num;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR; /* No a directory entry. */

    dh_dir = get_inode_of_vnode(dir)->in.dir;
    if (dh_lookup(dh_dir, name, &vnode_num))
        return -ENOENT; /* Link not found. */

    if (ramfs_get_vnode(dir->sb, &vnode_num, result)) {
        return -ENOLINK; /* Could not translate vnode_num to a vnode;
                          * Broken link? */
    }

    if (*result == dir) {
        vrele(*result);
        return -EDOM;
    }

    return 0;
}

int ramfs_link(vnode_t * dir, vnode_t * vnode, const char * name)
{
    ramfs_inode_t * inode;
    ramfs_inode_t * inode_dir;
    int retval = 0;

    if (!S_ISDIR(dir->vn_mode)) {
        retval = -ENOTDIR; /* No a directory entry. */
        goto out;
    }

    inode = get_inode_of_vnode(vnode);
    inode_dir = get_inode_of_vnode(dir);
    retval = dh_link(inode_dir->in.dir, vnode->vn_num, name);
    if (retval)
        goto out;

    inode->in_nlink++; /* Increment hard link count. */

out:
    return retval;
}

int ramfs_unlink(vnode_t * dir, const char * name)
{
    ramfs_inode_t * inode_dir;
    ino_t vnum;
    vnode_t * vn;
    ramfs_inode_t * inode;
    int retval = 0;

    if (!S_ISDIR(dir->vn_mode)) {
        retval = -ENOTDIR; /* No a directory entry. */
        goto out;
    }

    inode_dir = get_inode_of_vnode(dir);
    retval = dh_lookup(inode_dir->in.dir, name, &vnum);
    if (retval)
        goto out;

    retval = ramfs_get_vnode(dir->sb, &vnum, &vn);
    if (retval)
        goto out;
    inode = get_inode_of_vnode(vn);

    /* Mandatory cleanup. */
    fs_vnode_cleanup(vn);

    retval = dh_unlink(inode_dir->in.dir, name);
    if (retval)
        goto out;

    inode->in_nlink--; /* Decrement hard link count. */
    vrele(vn);
    if (inode->in_nlink <= 0)
        ramfs_delete_vnode(vn);

out:
    return retval;
}

int ramfs_mkdir(vnode_t * dir, const char * name, mode_t mode)
{
    ramfs_sb_t * ramfs_sb;
    vnode_t * vnode_new;
    ramfs_inode_t * inode_dir;
    ramfs_inode_t * inode_new;
    int retval = 0;

    if (!S_ISDIR(dir->vn_mode)) {
        retval = -ENOTDIR; /* No a directory entry. */
        goto out;
    }

    ramfs_sb = get_rfsb_of_sb(dir->sb);
    vnode_new = inpool_get_next(&(ramfs_sb->ramfs_ipool));
    if (vnode_new == 0) {
        retval = -ENOSPC;
        goto out; /* Can't create a new dir. */
    }
    inode_dir = get_inode_of_vnode(dir);
    inode_new = get_inode_of_vnode(vnode_new);

    inode_new->in.dir = kcalloc(1, sizeof(dh_table_t)); /* Create a dh_table */
    if (inode_new->in.dir == 0) {
        retval = -ENOSPC;
        goto delete_inode; /* Cant allocate dh_table */
    }
    vnode_new->vn_mode = S_IFDIR | mode; /* This is a directory. */
    inode_new->in_uid = curproc->euid;
    inode_new->in_gid = curproc->egid; /* TODO or to egid of the parent dir */
    /* TODO set times */

    /* Create links according to POSIX. */
    ramfs_link(&(inode_new->in_vnode), &(inode_new->in_vnode), RFS_DOT);
    ramfs_link(&(inode_new->in_vnode), &(inode_dir->in_vnode), RFS_DOTDOT);

    /* Insert inode to the inode lookup table of its superblock. */
    insert_inode(inode_new); /* This can't fail on mount. */
    ramfs_link(&(inode_dir->in_vnode), vnode_new, name);

    goto out;
delete_inode:
    ramfs_delete_vnode(&(inode_new->in_vnode));
out:
    return retval;
}

int ramfs_rmdir(vnode_t * dir,  const char * name)
{
    ramfs_inode_t * inode_dir;
    ino_t vnum;
    vnode_t * vn;
    ramfs_inode_t * inode;
    int retval = 0;

    if (!S_ISDIR(dir->vn_mode)) {
        retval = -ENOTDIR; /* No a directory entry. */
        goto out;
    }

    inode_dir = get_inode_of_vnode(dir);
    retval = dh_lookup(inode_dir->in.dir, name, &vnum);
    if (retval)
        goto out;

    retval = ramfs_get_vnode(dir->sb, &vnum, &vn);
    if (retval)
         goto out;
    inode = get_inode_of_vnode(vn);

    if (inode->in_nlink > 2) {
        retval = -ENOTEMPTY;
        goto out;
    }

    dh_unlink(inode->in.dir, RFS_DOT);
    dh_unlink(inode->in.dir, RFS_DOTDOT);
    dh_unlink(inode_dir->in.dir, name);

    vrele(vn);

out:
    return retval;
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
    if (dh == 0 || dh->dh_size == 0)
        return -ESPIPE; /* End of dir. */

    /* Translate iterator back to dirent. */
    *off = ((((off_t)it.dea_ind) << 32) & dea_ind_mask) |
           (off_t)(it.ch_ind & ch_ind_mask);
    d->d_ino = dh->dh_ino;
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
    buf->st_atime   = inode->in_atime;
    buf->st_mtime   = inode->in_mtime;
    buf->st_ctime   = inode->in_ctime;
    buf->st_blksize = inode->in_blksize;
    buf->st_blocks  = inode->in_blocks;

    return 0;
}

int ramfs_chmod(vnode_t * vnode, mode_t mode)
{
    vnode->vn_mode = mode;
    return 0;
}

int ramfs_chown(vnode_t * vnode, uid_t owner, gid_t group)
{
    ramfs_inode_t * inode = get_inode_of_vnode(vnode);

    inode->in_uid = owner;
    inode->in_gid = group;

    return 0;
}

/**
 * Intializes a ramfs superblock node.
 * @param ramfs_sb  is a pointer to a ramfs superblock.
 * @param mode      mount flags.
 */
static void init_sbn(ramfs_sb_t * ramfs_sb, uint32_t mode)
{
    fs_superblock_t * sb = &(ramfs_sb->sbn.sbl_sb);

    sb->fs = &ramfs_fs;
    sb->mode_flags = mode;
    sb->root = NULL; /* Cleared temporarily */

    /* Function pointers to superblock methods: */
    sb->get_vnode = ramfs_get_vnode;
    sb->delete_vnode = ramfs_delete_vnode;

    ramfs_sb->sbn.next = NULL;
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

    vn = inpool_get_next(&(ramfs_sb->ramfs_ipool));
    if (!vn) {
        goto out; /* Can't create */
    }
    inode = get_inode_of_vnode(vn);

    inode->in.dir = kcalloc(1, sizeof(dh_table_t)); /* Create a dh_table */
    /* Root is a directory. */
    vn->vn_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    /* TODO Any other settings? */

    /* Insert inode to the inode lookup table of its superblock. */
    insert_inode(inode); /* This can't fail on mount. */
    ramfs_sb->sbn.sbl_sb.root = vn;

    /* Create links according to POSIX. */
    ramfs_link(vn, vn, RFS_DOT);
    ramfs_link(vn, vn, RFS_DOTDOT);

    vrefset(vn, 2);
out:
    return vn;
}

/**
 * Insert a ramfs_sb_t at the end of the sb mount linked list.
 * @param ramfs_sb is a pointer to a ramfs superblock.
 */
static void insert_superblock(ramfs_sb_t * ramfs_sb)
{
    superblock_lnode_t * curr = ramfs_fs.sbl_head;

#ifdef configRAMFS_DEBUG
    KERROR(KERROR_DEBUG, "insert_superblock()\n");
#endif

    /* Add as a first sb if no other mounts yet */
    if (!curr) {
        ramfs_fs.sbl_head = &(ramfs_sb->sbn);
    } else {
        /* else find the last sb on the linked list. */
        while (curr->next) {
            curr = curr->next;
        }

        curr->next = &(ramfs_sb->sbn);
    }
}

/**
 * Remove a given ramfs_sb_t from the sb mount list.
 * @param ramfs_sb is a pointer to a ramfs superblock.
 */
static void remove_superblock(ramfs_sb_t * ramfs_sb)
{
    const char err_msg[] = "Unable to remove ramfs sb from the mount list.\n";
    superblock_lnode_t * prev;
    superblock_lnode_t * curr = ramfs_fs.sbl_head;

    if (!curr) {
        KERROR(KERROR_ERR, err_msg);
        return;
    }

    while (curr != &(ramfs_sb->sbn)) {
        prev = curr;
        curr = curr->next;
        if (!curr) {
            KERROR(KERROR_ERR, err_msg);
            return;
        }
    }

    prev->next = curr->next;
}

/**
 * Destroy a memory allocated for superblock and its inodes.
 * @note ramfs_sb is invalid after this call.
 * @param ramfs_sb Superblock to be freed.
 */
static void destroy_superblock(ramfs_sb_t * ramfs_sb)
{
    if (ramfs_sb->ramfs_iarr) {
        size_t i;

        /* Destroy inodes in iarr */
        for (i = 0; i < ramfs_sb->ramfs_iarr_size; i++) {
            /* NOTE: There should be no more references to vnodes anymore. */
            if (ramfs_sb->ramfs_iarr[i])
                destroy_inode(ramfs_sb->ramfs_iarr[i]);
        }

        kfree(ramfs_sb->ramfs_iarr);
        ramfs_sb->ramfs_iarr = NULL;
    }

    /* Destroy inode pool */
    inpool_destroy(&(ramfs_sb->ramfs_ipool));

    kfree(ramfs_sb);
}

/**
 * Create a new inode.
 * @param sb    is the superblock.
 * @param num   is the inode number.
 * @return Returns the newly created inode or null pointer if failed.
 */
vnode_t * ramfs_raw_create_inode(const fs_superblock_t * sb, ino_t * num)
{
    ramfs_inode_t * inode;
    ramfs_sb_t * ramfs_sb;

    inode = kmalloc(sizeof(ramfs_inode_t));
    if (!inode)
        return NULL;

    ramfs_sb = get_rfsb_of_sb(sb);
    init_inode(inode, ramfs_sb, num);

    return &(inode->in_vnode);
}

/**
 * Intialize a ramfs_inode struct.
 * @param inode     is the struct that will be initialized.
 * @param ramfs_sb  is the current superblock.
 * @param num       is the inode number to be used.
 */
static void init_inode(ramfs_inode_t * inode, ramfs_sb_t * ramfs_sb, ino_t * num)
{
    memset((void *)inode, 0, sizeof(ramfs_inode_t));
    fs_vnode_init(&inode->in_vnode, *num, &(ramfs_sb->sbn.sbl_sb),
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
        ramfs_set_filesize(inode, 0);
        break;
    case S_IFDIR:
        /* Free dhtable entries and dhtable. */
        if (inode->in.dir) {
            dh_destroy_all(inode->in.dir);
            kfree(inode->in.dir);
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
 *         Otherwise value other than zero indicating that isert failed.
 */
static int insert_inode(ramfs_inode_t * inode)
{
    ramfs_sb_t * ramfs_sb;
    ramfs_inode_t ** tmp_iarr;
    const ino_t vnode_num = inode->in_vnode.vn_num;
    int retval = 0;

    ramfs_sb = get_rfsb_of_sb(inode->in_vnode.sb);

    if (vnode_num >= (ino_t)(ramfs_sb->ramfs_iarr_size)) {
        /* Allocate more space for iarr. */
        ramfs_sb->ramfs_iarr_size++;
        tmp_iarr = (ramfs_inode_t **)krealloc(ramfs_sb->ramfs_iarr,
                (ino_t)(ramfs_sb->ramfs_iarr_size) * sizeof(ramfs_inode_t *));
        if (!tmp_iarr) {
            /* Can't allocate more memory for inode lookup table */
            retval = -ENOSPC;
            goto out;
        }
        ramfs_sb->ramfs_iarr = tmp_iarr; /* reallocate ok. */
    }

    /* Assign inode to the lookup table. */
    ramfs_sb->ramfs_iarr[vnode_num] = inode;

out:
    return retval;
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
static ssize_t ramfs_wr_regular(vnode_t * file, const off_t * restrict offset,
                                const void * buf, size_t count)
{
    ramfs_inode_t * inode = get_inode_of_vnode(file);
    const blksize_t blksize = inode->in_blksize;
    struct ramfs_dp dp;
    size_t bytes_wr = 0;

    /* No file type check is needed as this function is called only for regular
     * files. */

    do {
        const size_t remain = count - bytes_wr;
        size_t curr_wr_len;

        /* Get next block pointer. */
        dp = get_dp_by_offset(inode, *offset + bytes_wr);
        if (!dp.p) { /* Extend the file first. */
            /* Extend the file to the final size if possible. */
            if (ramfs_set_filesize(inode, inode->in_blocks * blksize
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
        memcpy(dp.p, ((char *)buf + bytes_wr), curr_wr_len);
        bytes_wr += curr_wr_len;
    } while (bytes_wr < count);

    file->vn_len = max(file->vn_len, *offset + bytes_wr);
    return bytes_wr;
}

/**
 * Transfers bytes from a regular file into buf.
 * @param file      is a regular file.
 * @param offset    is the offset form SEEK_START.
 * @param buf       is a buffer where bytes are written to.
 * @param count     is the requested number of bytes to be read.
 * @return Returns the number of bytes read from the file.
 */
static ssize_t ramfs_rd_regular(vnode_t * file, const off_t * restrict offset,
                                void * buf, size_t count)
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

        if ((*offset + bytes_rd) >= file->vn_len) {
            break; /* EOF */
        }

        /* Get next block pointer. */
        dp = get_dp_by_offset(inode, *offset + bytes_rd);
        if (dp.p == 0) {
            break; /* EOF */
        }

        /* Read bytes from the block. */
        curr_rd_len = min(remain, dp.len);
        memcpy(((char *)buf + bytes_rd), dp.p, curr_rd_len);
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
static int ramfs_set_filesize(ramfs_inode_t * file, off_t new_size)
{
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
                vrfree(bpp[i]);
            }
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
 *         length of returned block. dp.p == 0 if request out of bounds.
 */
static struct ramfs_dp get_dp_by_offset(ramfs_inode_t * inode, off_t offset)
{
    const blksize_t blksize = inode->in_blksize;
    size_t bi; /* Block index. */
    size_t di; /* Data index. */
    char * block; /* Pointer to the block. */
    struct ramfs_dp dp = { .p = 0, .len = 0 }; /* Return value. */

    if (offset > ((off_t)inode->in_blocks * (off_t)blksize))
        goto out; /* Out of bounds. */
    if (!inode->in.data)
        goto out; /* No data array. */

    bi = (size_t)((offset - (offset & ((off_t)(blksize - 1))))
            / (off_t)blksize);
    di = (size_t)(((size_t)offset) & (blksize - 1));
    block = (char *)(inode->in.data[bi]->b_data);

    /* Return value. */
    dp.p = &(block[di]);
    dp.len = blksize - di;

out:
    return dp;
}
