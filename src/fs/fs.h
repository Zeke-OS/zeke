/**
 *******************************************************************************
 * @file    fs.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system headers.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/** @addtogroup fs
  * @{
  */

#pragma once
#ifndef FS_H
#define FS_H

#include "kernel.h"
#include "syscalldef.h"
#include "devtypes.h"

#define FS_FLAG_INIT       0x01 /*!< Filse system initialized. */
#define FS_FLAG_FAIL       0x08 /*!< File system has failed. */

/* Some macros for use with flags *********************************************/
/**
 * Test act_flags for FS_FLAG_INIT.
 * @param act_flags actual flag values.
 */
#define FS_TFLAG_INIT(act_flags)       ((act_flags & FS_FLAG_INIT) != 0)

/**
 * Test act_flags for FS_FLAG_FAIL.
 * @param act_flags actual flag values.
 */
#define FS_TFLAG_FAIL(act_flags)       ((act_flags & FS_FLAG_FAIL) != 0)

/**
 * Test act_flags for any of exp_flags.
 * @param act_flags actual flag values.
 * @param exp_flags expected flag values.
 */
#define FS_TFLAGS_ANYOF(act_flags, exp_flags) ((act_flags & exp_flags) != 0)

/**
 * Test act_flags for all of exp_flags.
 * @param act_flags actual flag values.
 * @param exp_flags expected flag values.
 */
#define FS_TFLAGS_ALLOF(act_flags, exp_flags) ((act_flags & exp_flags) == exp_flags)
/* End of macros **************************************************************/

typedef struct {
    size_t inode;   /*!< inode number. (Can be ignored if irrelevant) */
    dev_t dev;      /*!< Device identifier. */
    struct file_ops * file_ops;
} vfile_t;

/**
 * File system
 */
typedef struct {
    unsigned int flags; /*!< Flags */
    int (*mount)(vfile_t * file, char * mpoint, uint32_t mode);
    int (*lookup)(vfile_t * file, char * str);
} fs_t;

/**
 * File operations struct.
 */
typedef struct file_ops {
    int (*lock)(vfile_t * file);
    int (*release)(vfile_t * file);
    int (*cwrite)(vfile_t * file, uint32_t ch); /*!< Character write function */
    int (*cread)(vfile_t * file, uint32_t * ch); /*!< Character read function */
    int (*bwrite)(vfile_t * file, void * buff, size_t size, size_t count); /*!< Block write function */
    int (*bread)(vfile_t * file, void * buff, size_t size, size_t count); /*!< Block read function */
    int (*bseek)(vfile_t * file, int offset, int origin, size_t size, pthread_t thread_id); /*!< Block seek function */
} file_ops_t;

extern fs_t fs_alloc_table[];

/* Thread specific functions used mainly by Syscalls **************************/
uint32_t fs_syscall(uint32_t type, void * p);

#endif /* FS_H */

/**
  * @}
  */
