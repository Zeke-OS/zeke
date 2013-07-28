/**
 *******************************************************************************
 * @file    dev.h
 * @author  Olli Vanhoja
 * @brief   Device driver subsystem header file for dev.c
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

#pragma once
#ifndef DEV_H
#define DEV_H

//#include <stdlib.h>
#include "kernel.h"
#include "syscalldef.h"
#include "devtypes.h"

#define DEV_FLAG_INIT       0x01 /*!< Device driver initialized. */
#define DEV_FLAG_LOCK       0x02 /*!< Device driver locked for thread_id_lock. */
#define DEV_FLAG_NONLOCK    0x04 /*!< Device driver non-lockable. */
#define DEV_FLAG_FAIL       0x08 /*!< Device driver has failed. */

/* Some macros for use with flags *********************************************/
/**
 * Test act_flags for DEV_FLAG_INIT.
 * @param act_flags actual flag values.
 */
#define DEV_TFLAG_INIT(act_flags)       ((act_flags & DEV_FLAG_INIT) != 0)

/**
 * Test act_flags for DEV_FLAG_LOCK.
 * @param act_flags actual flag values.
 */
#define DEV_TFLAG_LOCK(act_flags)       ((act_flags & DEV_FLAG_LOCK) != 0)

/**
 * Test act_flags for DEV_FLAG_NONLOCK.
 * @param act_flags actual flag values.
 */
#define DEV_TFLAG_NONLOCK(act_flags)    ((act_flags & DEV_FLAG_NONLOCK) != 0)

/**
 * Test act_flags for DEV_FLAG_FAIL.
 * @param act_flags actual flag values.
 */
#define DEV_TFLAG_FAIL(act_flags)       ((act_flags & DEV_FLAG_FAIL) != 0)

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

/* Thread specific functions used mainly by Syscalls **************************/
osStatus dev_threadDevWait(osDev_t dev, uint32_t millisec);
uint32_t dev_syscall(uint32_t type, void * p);

#endif /* DEV_H */

/**
  * @}
  */
