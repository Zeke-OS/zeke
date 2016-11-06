/**
 *******************************************************************************
 * @file    dirent.h
 * @author  Olli Vanhoja
 * @brief   Format of directory entries.
 * @section LICENSE
 * Copyright (c) 2013, 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup LIBC
 * @{
 */

#ifndef DIRENT_H
#define DIRENT_H

#include <stdint.h>
#include <sys/types.h>

/**
 * @addtogroup getdents
 * @{
 */

/**
 * The dirent structure.
 */
struct dirent {
    ino_t d_ino;        /*!< File serial number. */
    uint8_t d_type;     /*!< File type. */
    char d_name[256];   /*!< Name of entry. */
};

/*
 * File types
 */
#define DT_UNKNOWN   0 /*!< The file type is unknown. */
#define DT_FIFO      1 /*!< The file type is FIFO. */
#define DT_CHR       2 /*!< The file type is character device. */
#define DT_DIR       4 /*!< The file type is directory. */
#define DT_BLK       6 /*!< The file type is block device. */
#define DT_REG       8 /*!< The file type is regular file. */
#define DT_LNK      10 /*!< The file type is symbolic link. */
#define DT_SOCK     12 /*!< The file type is socket. */
#define DT_WHT      14 /*!< The file type is whiteout. */

/*
 * Convert between stat structure types and directory types.
 */

/**
 * Convert from stat type to dirent type.
 */
#define IFTODT(mode)    (((mode) & 0170000) >> 12)

/**
 * Convert from dirent type to stat type.
 */
#define DTTOIF(dirtype) ((dirtype) << 12)

/**
 * @}
 */

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/**
 * Arguments for SYSCALL_FS_GETDENTS
 */
struct _fs_getdents_args {
    int fd;
    char * buf;
    size_t nbytes;
};
#endif

/*
 * Definitions for library routines operating on directories.
 */
typedef struct _dirdesc {
    int dd_fd;
    size_t dd_loc;
    size_t dd_count;
    struct dirent dd_buf[10];
} DIR;

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

/**
 * @addtogroup getdents
 * Get directory etries.
 * @{
 */
/**
 * Get directory entries.
 */
int getdents(int fd, char * buf, int nbytes);
/**
 * @}
 */

/*
int alphasort(const struct dirent **, const struct dirent **);
*/

/**
 * @addtogroup closedir
 * Close a directory.
 * @{
 */
/**
 * Close a directory.
 */
int closedir(DIR * dirp);
/**
 * @}
 */

/**
 * @addtogroup dirfd
 * Get directory stream fd.
 * @{
 */
/**
 * Get directory stream fd.
 */
int dirfd(DIR * dirp);
/**
 * @}
 */

/**
 * @addtogroup opendir
 * Open a directory.
 * @{
 */
/**
 * Open fd as a directory.
 */
DIR * fdopendir(int);
/**
 * Open a directory.
 */
DIR * opendir(const char * name);
/**
 * @}
 */

/**
 * @addtogroup readdir
 * @{
 */
/**
 * Read from a directory.
 */
struct dirent * readdir(DIR * dirp);
/**
 * @}
 */

/*
int readdir_r(DIR * restrict, struct dirent * restrict,
        struct dirent ** restrict);
*/

/**
 * @addtogroup rewinddir
 * @{
 */
/**
 * Rewind a direcory.
 */
void rewinddir(DIR * dirp);
/**
 * @}
 */

/*
int scandir(const char *, struct dirent ***,
        int (*)(const struct dirent *),
        int (*)(const struct dirent **,
        const struct dirent **));
*/

/**
 * @addtogroup seekdir
 * @{
 */
/**
 * Set a the position of the nex readdir() call.
 */
void seekdir(DIR * dirp, off_t loc);
/**
 * @}
 */

/**
 * @addtogroup telldir
 * @{
 */
/**
 * Return the current location of a directory stream.
 */
off_t telldir(DIR * dirp);
/**
 * @}
 */

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#if (KERNEL_INTERNAL || _DIRENT_INTERNAL_)
#define DIRENT_SEEK_START 0x00000000FFFFFFFF
#endif

#endif /* DIRENT_H */

/**
 * @}
 */
