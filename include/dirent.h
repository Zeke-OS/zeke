/**
 *******************************************************************************
 * @file    dirent.h
 * @author  Olli Vanhoja
 * @brief   Format of directory entries.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup libc
 * @{
 */

#pragma once
#ifndef DIRENT_H
#define DIRENT_H

#include <sys/cdefs.h>
#include <stdint.h>
#include <sys/types.h>

struct dirent {
    ino_t d_ino; /*!< File serial number. */
    uint8_t d_type;
    char d_name[256]; /*!< Name of entry. */
};

/*
 * File types
 */
#define DT_UNKNOWN   0
#define DT_FIFO      1
#define DT_CHR       2
#define DT_DIR       4
#define DT_BLK       6
#define DT_REG       8
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14

/*
 * Convert between stat structure types and directory types.
 */
#define IFTODT(mode)    (((mode) & 0170000) >> 12)
#define DTTOIF(dirtype) ((dirtype) << 12)

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

/**
 * Get directory entries.
 */
int getdents(int fd, char * buf, int nbytes);

/*
int alphasort(const struct dirent **, const struct dirent **);
int closedir(DIR *);
int dirfd(DIR *);
DIR * fdopendir(int);
DIR * opendir(const char *);
struct dirent * readdir(DIR *);
int readdir_r(DIR * restrict, struct dirent * restrict,
        struct dirent ** restrict);
void rewinddir(DIR *);
int scandir(const char *, struct dirent ***,
        int (*)(const struct dirent *),
        int (*)(const struct dirent **,
        const struct dirent **));
void seekdir(DIR *, long int);
long int telldir(DIR *);*/

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* DIRENT_H */

/**
 * @}
 */
