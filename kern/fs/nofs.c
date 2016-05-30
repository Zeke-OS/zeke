/**
 *******************************************************************************
 * @file    fs.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system, nofs.
 * @section LICENSE
 * Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fs/fs.h>
#include <kstring.h>
#include <proc.h>

const vnode_ops_t nofs_vnode_ops = {
    .lock = fs_enotsup_lock,
    .release = fs_enotsup_release,
    .read = fs_enotsup_read,
    .write = fs_enotsup_write,
    .lseek = fs_enotsup_lseek,
    .ioctl = fs_enotsup_ioctl,
    .event_vnode_opened = fs_enotsup_event_vnode_opened,
    .event_fd_created = fs_enotsup_event_fd_created,
    .event_fd_closed = fs_enotsup_event_fd_closed,
    .event_vnode_unlink = fs_enotsup_event_vnode_unlink,
    .create = fs_enotsup_create,
    .mknod = fs_enotsup_mknod,
    .lookup = fs_enotsup_lookup,
    .revlookup = nofs_revlookup,
    .link = fs_enotsup_link,
    .unlink = fs_enotsup_unlink,
    .mkdir = fs_enotsup_mkdir,
    .rmdir = fs_enotsup_rmdir,
    .readdir = fs_enotsup_readdir,
    .stat = fs_enotsup_stat,
    .utimes = fs_enotsup_utimes,
    .chmod = fs_enotsup_chmod,
    .chflags = fs_enotsup_chflags,
    .chown = fs_enotsup_chown,
};

/* Not sup vnops */
int fs_enotsup_lock(file_t * file)
{
    return -ENOTSUP;
}

int fs_enotsup_release(file_t * file)
{
    return -ENOTSUP;
}

ssize_t fs_enotsup_read(file_t * file, struct uio * uio, size_t count)
{
    return -ENOTSUP;
}

ssize_t fs_enotsup_write(file_t * file, struct uio * uio, size_t count)
{
    return -ENOTSUP;
}

off_t fs_enotsup_lseek(file_t * file, off_t offset, int whence)
{
    vnode_t * vn = file->vnode;
    struct stat stat_buf;
    off_t new_offset;

    switch (whence) {
    case SEEK_SET:
        file->seek_pos = offset;
        break;
    case SEEK_CUR:
        file->seek_pos += offset;
        break;
    case SEEK_END:
        if (vn->vnode_ops->stat(vn, &stat_buf))
            return -EBADF;

        new_offset = stat_buf.st_size + offset;

        if (new_offset >= stat_buf.st_size)
            file->seek_pos = new_offset;
        else
            return -EOVERFLOW;
        break;
    default:
        return -EINVAL;
    }

    return file->seek_pos;
}

int fs_enotsup_ioctl(file_t * file, unsigned request, void * arg,
                     size_t arg_len)
{
    return -ENOTTY;
}

int fs_enotsup_event_vnode_opened(struct proc_info * p, vnode_t * vnode)
{
    return 0;
}

void fs_enotsup_event_fd_created(struct proc_info * p, file_t * file)
{
}

void fs_enotsup_event_fd_closed(struct proc_info * p, file_t * file)
{
}

void fs_enotsup_event_vnode_unlink(vnode_t * vnode)
{
}

int fs_enotsup_create(vnode_t * dir, const char * name, mode_t mode,
                      vnode_t ** result)
{
    return -ENOTSUP;
}

int fs_enotsup_mknod(vnode_t * dir, const char * name, int mode,
                     void * specinfo, vnode_t ** result)
{
    return -ENOTSUP;
}

int fs_enotsup_lookup(vnode_t * dir, const char * name, vnode_t ** result)
{
    return -ENOTSUP;
}

int nofs_revlookup(vnode_t * dir, ino_t * ino, char * name, size_t name_len)
{
    struct dirent d;
    off_t doff = DIRENT_SEEK_START;
    int err;

    /*
     * Note: This doesn't handle symbolic links and we dont't even check
     * st_dev here.
     * TODO Support symbolic links.
     */

    do {
        err = dir->vnode_ops->readdir(dir, &d, &doff);
        if (!err) {
            if (d.d_ino == *ino) {
                size_t len;

                len = strlcpy(name, d.d_name, name_len);
                if (len >= name_len)
                    return -ENAMETOOLONG;

                return 0;
            }
        }
    } while (!err);

    if (err == -ESPIPE)
        return -ENOENT;
    return err;
}

int fs_enotsup_link(vnode_t * dir, vnode_t * vnode, const char * name)
{
    return -EACCES;
}

int fs_enotsup_unlink(vnode_t * dir, const char * name)
{
    return -EACCES;
}

int fs_enotsup_mkdir(vnode_t * dir,  const char * name, mode_t mode)
{
    return -ENOTSUP;
}

int fs_enotsup_rmdir(vnode_t * dir,  const char * name)
{
    return -ENOTSUP;
}

int fs_enotsup_readdir(vnode_t * dir, struct dirent * d, off_t * off)
{
    return -ENOTSUP;
}

int fs_enotsup_stat(vnode_t * vnode, struct stat * buf)
{
    return -ENOTSUP;
}

int fs_enotsup_utimes(vnode_t * vnode, const struct timespec times[2])
{
    return -EPERM;
}

int fs_enotsup_chmod(vnode_t * vnode, mode_t mode)
{
    return -ENOTSUP;
}

int fs_enotsup_chflags(vnode_t * vnode, fflags_t flags)
{
    return -ENOTSUP;
}

int fs_enotsup_chown(vnode_t * vnode, uid_t owner, gid_t group)
{
    return -ENOTSUP;
}
