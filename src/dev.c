/**
 *******************************************************************************
 * @file    dev.c
 * @author  Olli Vanhoja
 * @brief   Device driver subsystem.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

#include <string.h>
#include <stdlib.h>
#include "sched.h"
#include "dev_config.h"
#include "dev.h"

/** TODO dev syscalls */
/** TODO How can we wait for dev to unlock? osSignalWait? */
/** TODO dev syscall should first try to access device and then go
 *       to wait if desired so */

struct dev_driver dev_alloc_table[31];

/**
 * Init device drivers.
 */
void dev_init_all(void)
{
    memset(dev_alloc_table, 0, sizeof(dev_alloc_table));

    /* Call initializers */
    #define DEV_DECLARE2(major, dname) dname##_init(major);
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
int dev_open(osDev_t dev, osThreadId thread_id)
{
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];
    unsigned int flags = dev_al->flags;
    unsigned int tmp;

    if ((flags & DEV_FLAG_INIT) == 0) {
        return DEV_OERR_UNKNOWN;
    }

    tmp = flags & (DEV_FLAG_LOCK | DEV_FLAG_NONLOCK | DEV_FLAG_FAIL);
    if (tmp) {
        if (tmp & DEV_FLAG_FAIL)
            return DEV_OERR_INTERNAL;
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
   otherwise DEV_CERR_x.
 */
int dev_close(osDev_t dev, osThreadId thread_id)
{
    if (!dev_check_res(dev, thread_id)) {
        return DEV_CERR_NLOCK;
    }
    dev_alloc_table[DEV_MAJOR(dev)].flags ^= DEV_FLAG_LOCK;

    /* TODO wait for dev to be not busy? */

    /* This is bit stupid but might be the easiest way to implement this :/ */
    sched_threadDevSignal(SCHED_DEV_WAIT_BIT, dev);

    return DEV_CERR_OK;
}

/**
 * Check if thread_id has locked the device given in dev.
 * @param dev device that is checked for lock.
 * @param thread_id thread that may have the lock for the dev.
 * @return 0 if the device is not locked by thread_id.
 */
int dev_check_res(osDev_t dev, osThreadId thread_id)
{
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    return (DEV_TFLAG_LOCK(dev_al->flags)
            && (dev_al->thread_id_lock == thread_id));
}

/**
 * Write to a character device.
 * @param ch is written to the device.
 * @param dev device.
 * @param thread_id thread that is writing to the device.
 */
int dev_cwrite(uint32_t ch, osDev_t dev, osThreadId thread_id)
{
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && !(DEV_TFLAG_NONLOCK(dev_al->flags))) {
        return DEV_CWR_NLOCK;
    }

    if (DEV_TFLAG_FAIL(dev_al->flags)) {
        return DEV_CWR_INTERNAL;
    }

    if (dev_al->cwrite == NULL) {
        return DEV_CWR_NOT;
    }

    return dev_al->cwrite(ch, dev);
}

/**
 * Read from a character device.
 * @param ch output is written here.
 * @param dev device.
 * @param thread_id thread that is reading the device.
 */
int dev_cread(uint32_t * ch, osDev_t dev, osThreadId thread_id)
{
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && !(DEV_TFLAG_NONLOCK(dev_al->flags))) {
        return DEV_CRD_NLOCK;
    }

    if (DEV_TFLAG_FAIL(dev_al->flags)) {
        return DEV_CRD_INTERNAL;
    }

    if (dev_al->cread == NULL) {
        return DEV_CRD_NOT;
    }

    return dev_al->cread(ch, dev);
}

/**
 * Write to a block device.
 * @param buff Pointer to the array of elements to be written.
 * @param size in bytes of each element to be written.
 * @param count number of elements, each one with a size of size bytes.
 * @param dev device to be written to.
 * @param thread_id id of the thread that is writing to the block device.
 */
int dev_bwrite(const void * buff, size_t size, size_t count, osDev_t dev, osThreadId thread_id)
{
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && !(DEV_TFLAG_NONLOCK(dev_al->flags))) {
        return DEV_BWR_NLOCK;
    }

    if (DEV_TFLAG_FAIL(dev_al->flags)) {
        return DEV_BWR_INTERNAL;
    }

    if (dev_al->bwrite == NULL) {
        return DEV_BWR_NOT;
    }

    return dev_al->bwrite((void *)buff, size, count, dev);
}

/**
 * Read from a block device.
 * @param buff pointer to a block of memory with a size of at least (size*count) bytes, converted to a void *.
 * @param size in bytes, of each element to be read.
 * @param count number of elements, each one with a size of size bytes.
 * @param dev device to be read from.
 * @param thread_id id of the thread that is reading the block device.
 */
int dev_bread(void * buff, size_t size, size_t count, osDev_t dev, osThreadId thread_id)
{
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && !(DEV_TFLAG_NONLOCK(dev_al->flags))) {
        return DEV_BRD_NLOCK;
    }

    if (DEV_TFLAG_FAIL(dev_al->flags)) {
        return DEV_BRD_INTERNAL;
    }

    if (dev_al->bread == NULL) {
        return DEV_BRD_NOT;
    }

    return dev_al->bread((void *)buff, size, count, dev);
}

/**
 * Seek block device.
 * TODO Implementation
 * @param Number of size units to offset from origin.
 * @param origin Position used as reference for the offset.
 * @param size in bytes, of each element.
 * @param thread_id id of the thread that is seeking the block device.
 */
int dev_bseek(int offset, int origin, size_t size, osDev_t dev, osThreadId thread_id)
{
    return DEV_BSK_INTERNAL;
}

/**
  * @}
  */
