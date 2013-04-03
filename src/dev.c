/**
 *******************************************************************************
 * @file    dev.c
 * @author  Olli Vanhoja
 * @brief   Device driver system.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

/**
 * TODO Add internal error & busy detection for read & write functions
 */

#include "dev_config.h"
#include "dev.h"

struct dev_driver dev_alloc_table[31];

/**
 * Init device drivers.
 */
void dev_init_all()
{
    memset(dev_alloc_table, 0, sizeof(dev_alloc_table));

    /* Call initializers */
    #define DEV_DECLARE2(major, dname) dname##_init(major)
    #define DEV_DECLARE(major, dname) DEV_DECLARE2(major, dname)
    #include "dev_config.h"
    #undef DEV_DECLARE
    #undef DEV_DECLARE2
}

/**
 * Open and lock device access.
 * @param dev the device accessed.
 * @param thread_id that should get lock for the dev.
 * @return DEV_OERR_OK (0) if the lock acquired and device access was opened;
 * otherwise DEV_OERR_x
 */
int dev_open(dev_t dev, osThreadId thread_id)
{
    struct * dev_al = dev_alloc_table[DEV_MAJOR(dev)];
    unsigned int flags = dev_al->flags;
    unsigned int tmp;

    if ((flags & DEV_FLAG_INIT) == 0) {
        return DEV_OERR_UNKNOWN;
    }

    tmp = flags & (DEV_FLAG_LOCK | DEV_FLAG_NONLOCK | DEV_FLAG_FAIL);
    if (tmp) {
        if (tmp & DEV_FLAG_FAIL)
            return DEV_ORERR_INTERNAL;
        if (tmp & DEV_FLAG_NONLOCK)
            return DEV_OERR_NONLOCK;
        if (tmp & DEV_FLAG_LOCK)
            return DEV_OERR_LOCKED;
    }

    dev_al->flags |= DEV_FLAG_LOCK;
    dev_al->thread_id_lock = thread_id;

    return DEV_OERR_OK;
}

/**
 * Close and unlock device access.
 * @param dev the device.
 * @param thread_id thread that tries to close this device.
 * @return DEV_CERR_OK (0) if the device access was closed succesfully;
   otherwise DEV_CERR_x
 */
int dev_close(dev_t dev, osThreadId thread_id)
{
    if (!dev_check_res(dev, thread_id)) {
        return DEV_CERR_NOT;
    }
    dev_al->flags ^= DEV_FLAG_LOCK;

    return DEV_CERR_OK;
}

int dev_check_res(dev_t dev, osThreadId thread_id)
{
    struct * dev_al = dev_alloc_table[DEV_MAJOR(dev)];

    return ((dev_al->flags & DEV_FLAG_LOCK)
        && (dev_al->thread_id_lock == thread_id));
}

/**
 * Write to a character device.
 */
int dev_cwrite(uint32_t ch, dev_t dev, osThreadId thread_id)
{
    struct * dev_al = dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && ((dev_al->flags & DEV_FLAG_NONLOCK) == 0)) {
        return DEV_CWR_NLOCK;
    }

    if (dev_al->cwrite == NULL) {
        return DEV_CWR_NOT;
    }

    dev_al->cwrite(ch, dev);

    return DEV_CWR_OK;
}

/**
 * Read from a character device.
 */
int dev_cread(uint32_t * ch, dev_t dev, osThreadId thread_id)
{
    struct * dev_al = dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && ((dev_al->flags & DEV_FLAG_NONLOCK) == 0)) {
        return DEV_CRD_NLOCK;
    }

    if (dev_al->cread == NULL) {
        return DEV_CRD_NOT;
    }

    dev_al->cread(ch, dev);

    return DEV_CRD_OK;
}

/**
 * Write to a block device.
 */
int dev_bwrite(void * buff, int size, int count, dev_t dev, osThreadId thread_id)
{
    struct * dev_al = dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && ((dev_al->flags & DEV_FLAG_NONLOCK) == 0)) {
        return DEV_BWR_NLOCK;
    }

    if (dev_al->bwrite == NULL) {
        return DEV_BWR_NOT;
    }

    dev_al->bwrite(buff, size, count, dev);

    return DEV_BWR_OK;
}

/**
 * Read from a block device.
 */
int dev_bread(void * buff, int size, int count, dev_t dev, osThreadId thread_id)
{
    struct * dev_al = dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && ((dev_al->flags & DEV_FLAG_NONLOCK) == 0)) {
        return DEV_BRD_NLOCK;
    }

    if (dev_al->bread == NULL) {
        return DEV_BRD_NOT;
    }

    dev_al->bread(buff, size, count, dev);

    return DEV_BRD_OK;
}

/**
  * @}
  */
