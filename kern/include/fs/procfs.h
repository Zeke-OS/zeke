/**
 *******************************************************************************
 * @file    procfs.h
 * @author  Olli Vanhoja
 * @brief   Process file system headers.
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

#pragma once
#ifndef PROCFS_H
#define PROCFS_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/param.h>
#include <fs/fs.h>

#define PROCFS_FSNAME           "procfs"    /*!< Name of the fs. */

#define PROCFS_PERMS            0400        /*!< Default file permissions of
                                             *   an procfs entry.
                                             */
/* Procfs file names */
#define PROCFS_FN_STATUS        "status"

/**
 * Procfs file types.
 */
enum procfs_filetype {
    PROCFS_STATUS       /*!< Process status file. */
};

/**
 * Procfs specinfo descriptor.
 */
struct procfs_info {
    enum procfs_filetype ftype;
    pid_t pid;
};

/**
 * Create an entry for a process into procfs.
 * @param proc is a PCB to be described in procfs.
 */
int procfs_mkentry(const proc_info_t * proc);

/**
 * Remove a process entry strored in procfs.
 * @param pid is the process ID of the process to be removed.
 */
int procfs_rmentry(pid_t pid);


#endif /* PROCFS_H */
