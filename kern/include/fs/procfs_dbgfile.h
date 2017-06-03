/**
 *******************************************************************************
 * @file    procfs_dbgfile.c
 * @author  Olli Vanhoja
 * @brief   Generic debug file handler.
 * @section LICENSE
 * Copyright (c) 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup fs
 * @{
 */

/**
 * @addtogroup procfs
 * @{
 */

#pragma once
#ifndef FS_PROCFS_DBGFILE_H
#define FS_PROCFS_DBGFILE_H

#include <stddef.h>

struct procfs_dbgfile_opt {
    void * sect_start;
    void * sect_stop;
    size_t bsize;
    int (*read)(void * buf, size_t max, void * elem);
    int (*write)(const void * buf, size_t bufsize);
};

struct procfs_stream * procfs_dbgfile_read(const struct procfs_file * spec);
ssize_t procfs_dbgfile_write(const struct procfs_file * spec,
                             struct procfs_stream * stream,
                             const uint8_t * buf, size_t bufsize);
void procfs_dbgfile_rele(struct procfs_stream * stream);

/**
 * Create a static debug file under procfs.
 * @param _filename_ is the name of the file in the procfs root.
 * @param _sect_start_ is the start address of the file data.
 * @param _sect_stop_ is the stop address of the file data.
 * @param _read_ is a pointer to the read function for the data.
 * @param _write_ is a pointer to the write function for the data.
 */
#define PROCFS_DBGFILE(_filename_, _sect_start_, _sect_stop_, _read_, _write_) \
    static struct procfs_file procfs_dbgfile_##_filename_ = {                  \
        .filename = __XSTRING(_filename_),                                     \
        .readfn = procfs_dbgfile_read,                                         \
        .writefn = procfs_dbgfile_write,                                       \
        .relefn = procfs_dbgfile_rele,                                         \
        .opt = (&(struct procfs_dbgfile_opt){                                  \
            .sect_start = (void *)(_sect_start_),                              \
            .sect_stop = (void *)(_sect_stop_),                                \
            .bsize = sizeof(*(_sect_start_)),                                  \
            .read = (_read_),                                                  \
            .write = (_write_),                                                \
        }),                                                                    \
    };                                                                         \
    DATA_SET(procfs_files, procfs_dbgfile_##_filename_)

#endif /* FS_PROCFS_DBGFILE_H */

/**
 * @}
 */

/**
 * @}
 */
