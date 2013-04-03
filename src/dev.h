/**
 *******************************************************************************
 * @file    dev.h
 * @author  Olli Vanhoja
 * @brief   Device driver system header file for dev.c
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

#pragma once
#ifndef DEV_H
#define DEV_H

#include "devtypes.h"

extern dev_driver dev_alloc_table[];

#define DEV_FLAG_INIT       0x01 /*!< Device driver initialized. */
#define DEV_FLAG_LOCK       0x02 /*!< Device driver locked for thread_id_lock. */
#define DEV_FLAG_NONLOCK    0x04 /*!< Device driver non-lockable. */
#define DEV_FLAG_FAIL       0x08 /*!< Device driver has failed. */

/**
 * Device driver initialization.
 * This must be called by every drvname_init(int major).
 * @add_flags additional flags (eg. DEV_FLAG_NONLOCK).
 */
#define DEV_INIT(major, cwrite, cread, bwrite, bread, add_flags) do {   \
    dev_alloc_table[major].flags = DEV_FLAG_INIT | flags                \
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
    unsigned int flags;
    osThreadId thread_id_lock, /*!< Device locked for this thread if
                                * DEV_FLAG_LOCK is set. */
    int (*cwrite)(uint32_t ch, dev_t dev),
    int (*cread)(uint32_t * ch, dev_t dev),
    int (*bwrite)(void * buff, int size, int count, dev_t dev),
    int (*bread)(void * buff, int size, int count, dev_t dev)
};

void dev_init_all();
int dev_open(dev_t dev, osThreadId thread_id);
int dev_close(dev_t dev, osThreadId thread_id);
int dev_check_res(dev_t dev, osThreadId thread_id);
int dev_cwrite(uint32_t ch, dev_t dev, osThreadId thread_id);
int dev_cread(uint32_t * ch, dev_t dev, osThreadId thread_id);
int dev_bwrite(void * buff, int size, int count, dev_t dev, osThreadId thread_id);
int dev_bread(void * buff, int size, int count, dev_t dev, osThreadId thread_id);

#endif /* DEV_H */

/**
  * @}
  */
