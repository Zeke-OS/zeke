/**
 *******************************************************************************
 * @file     devfs.h
 * @author   Olli Vanhoja
 * @brief    Device driver subsystem/devfs header file for devfs.c
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

/** @addtogroup dev
  * @{
  */

#pragma once
#ifndef DEV_H
#define DEV_H

#include "kernel.h"
#include "syscalldef.h"
#include "devtypes.h"
#include "deverr.h"

#define DEV_FLAG_INIT       0x01 /*!< Device driver initialized. */
#define DEV_FLAG_LOCK       0x02 /*!< Device driver locked for thread_id_lock. */
#define DEV_FLAG_NONLOCK    0x04 /*!< Device driver non-lockable. */
#define DEV_FLAG_FAIL       0x08 /*!< Device driver has failed. */

/* Some macros for use with flags *********************************************/
/**
* Test act_flags for DEV_FLAG_INIT.
* @param act_flags actual flag values.
*/
#define DEV_TFLAG_INIT(act_flags) ((act_flags & DEV_FLAG_INIT) != 0)

/**
* Test act_flags for DEV_FLAG_LOCK.
* @param act_flags actual flag values.
*/
#define DEV_TFLAG_LOCK(act_flags) ((act_flags & DEV_FLAG_LOCK) != 0)

/**
* Test act_flags for DEV_FLAG_NONLOCK.
* @param act_flags actual flag values.
*/
#define DEV_TFLAG_NONLOCK(act_flags) ((act_flags & DEV_FLAG_NONLOCK) != 0)

/**
* Test act_flags for DEV_FLAG_FAIL.
* @param act_flags actual flag values.
*/
#define DEV_TFLAG_FAIL(act_flags) ((act_flags & DEV_FLAG_FAIL) != 0)

/**
* Test act_flags for any of exp_flags.
* @param act_flags actual flag values.
* @param exp_flags expected flag values.
*/
#define DEV_TFLAGS_ANYOF(act_flags, exp_flags) ((act_flags & exp_flags) != 0)

/**
* Test act_flags for all of exp_flags.
* @param act_flags actual flag values.
* @param exp_flags expected flag values.
*/
#define DEV_TFLAGS_ALLOF(act_flags, exp_flags) ((act_flags & exp_flags) == exp_flags)
/* End of macros **************************************************************/

/**
* Device driver initialization.
* This must be called by every drvname_init(int major).
* @param major number of the device driver.
* @param pcwrite a function pointer the character device write interface.
* @param pcread a function pointer the character device read interface.
* @param pbwrite a function pointer the block device write interface.
* @param pbread a function pointer the block device read interface.
* @param add_flags additional flags (eg. DEV_FLAG_NONLOCK).
*/
#define DEV_INIT(major, pcwrite, pcread, pbwrite, pbread, pbseek, add_flags) do\
{                                                                              \
    dev_alloc_table[major].flags = DEV_FLAG_INIT | add_flags;                  \
    dev_alloc_table[major].cwrite = pcwrite;                                   \
    dev_alloc_table[major].cread = pcread;                                     \
    dev_alloc_table[major].bwrite = pbwrite;                                   \
    dev_alloc_table[major].bread = pbread;                                     \
    dev_alloc_table[major].bseek = pbseek;                                     \
} while (0)

/**
* Device driver struct.
* @note Single device can be a character device and a block device
* at the same time.
*/
struct dev_driver {
    unsigned int flags; /*!< Device driver flags */
    osThreadId thread_id_lock; /*!< Device locked for this thread if
                                * DEV_FLAG_LOCK is set. */
    int (*cwrite)(uint32_t ch, osDev_t dev); /*!< Character device write function */
    int (*cread)(uint32_t * ch, osDev_t dev); /*!< Character device read function */
    int (*bwrite)(void * buff, size_t size, size_t count, osDev_t dev); /*!< Block device write function */
    int (*bread)(void * buff, size_t size, size_t count, osDev_t dev); /*!< Block device read function */
    int (*bseek)(int offset, int origin, size_t size, osDev_t dev, osThreadId thread_id); /*!< Block device seek function */
};

extern struct dev_driver dev_alloc_table[];

int dev_open(osDev_t dev, osThreadId thread_id);
int dev_close(osDev_t dev, osThreadId thread_id);
int dev_check_res(osDev_t dev, osThreadId thread_id);
int dev_crw(ds_osDevCData_t * args, int write, osThreadId thread_id);
int dev_brw(ds_osDevBData_t * args, int write, osThreadId thread_id);
int dev_bseek(ds_osDevBSeekData_t * args, osThreadId thread_id);

#endif /* DEVFS_H */

/**
  * @}
  */

/**
  * @}
  *
