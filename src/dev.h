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

#include <stdlib.h>
#include "devtypes.h"

extern dev_driver dev_alloc_table[];

#define DEV_FLAG_INIT       0x01 /*!< Device driver initialized. */
#define DEV_FLAG_LOCK       0x02 /*!< Device driver locked for thread_id_lock. */
#define DEV_FLAG_NONLOCK    0x04 /*!< Device driver non-lockable. */
#define DEV_FLAG_FAIL       0x08 /*!< Device driver has failed. */

/* Some macros for use with flags */
#define DEV_TFLAG_INIT(act_flags)       ((act_flags & DEV_FLAG_INIT) != 0)
#define DEV_TFLAG_LOCK(act_flags)       ((act_flags & DEV_FLAG_LOCK) != 0)
#define DEV_TFLAG_NONLOCK(act_flags)    ((act_flags & DEV_FLAG_NONLOCK) != 0)
#define DEV_TFLAG_FAIL(act_flags)       ((act_flags & DEV_FLAG_FAIL) != 0)
#define DEV_TFLAGS(act_flags, exp_flags) ((act_flags & exp_flags) != 0)

/**
 * Device driver initialization.
 * This must be called by every drvname_init(int major).
 * @param major number of the device driver.
 * @param cwrite a function pointer the character device write interface.
 * @param cread a function pointer the character device read interface.
 * @param bwrite a function pointer the block device write interface.
 * @param bread a function pointer the block device read interface.
 * @param add_flags additional flags (eg. DEV_FLAG_NONLOCK).
 */
#define DEV_INIT(major, cwrite, cread, bwrite, bread, add_flags) do {   \
    dev_alloc_table[major].flags = DEV_FLAG_INIT | add_flags            \
    dev_alloc_table[major].cwrite = cwrite;                             \
    dev_alloc_table[major].cread = cread;                               \
    dev_alloc_table[major].bwrite = bwrite;                             \
    dev_alloc_table[major].bread = bread;                               \
} while (0)

/**
 * Device driver struct.
 * @note Single device can be a character device and a block device
 * at the same time.
 */
struct dev_driver {
    unsigned int flags, /*!< Device driver flags */
    osThreadId thread_id_lock, /*!< Device locked for this thread if
                                * DEV_FLAG_LOCK is set. */
    int (*cwrite)(uint32_t ch, dev_t dev),
    int (*cread)(uint32_t * ch, dev_t dev),
    int (*bwrite)(void * buff, size_t size, size_t count, dev_t dev),
    int (*bread)(void * buff, size_t size, size_t count, dev_t dev)
};

void dev_init_all();
int dev_open(dev_t dev, osThreadId thread_id);
int dev_close(dev_t dev, osThreadId thread_id);
int dev_check_res(dev_t dev, osThreadId thread_id);
int dev_cwrite(uint32_t ch, dev_t dev, osThreadId thread_id);
int dev_cread(uint32_t * ch, dev_t dev, osThreadId thread_id);
int dev_bwrite(void * buff, size_t size, size_t count, dev_t dev, osThreadId thread_id);
int dev_bread(void * buff, size_t size, size_t count, dev_t dev, osThreadId thread_id);

#endif /* DEV_H */

/**
  * @}
  */
