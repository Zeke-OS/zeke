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
#include "ksignal.h"
#include "syscalldef.h"
#include "syscall.h"
#include "dev_config.h"
#include "dev.h"

/** TODO dev syscalls */
/** TODO How can we wait for dev to unlock? osSignalWait? */
/** TODO dev syscall should first try to access device and then go
 *       to wait if desired so */
/** TODO Especially non-lockable devs could have message queues for
 *  storing the data */

struct dev_driver dev_alloc_table[DEV_MAJORDEVS];

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
 *         otherwise DEV_CERR_x.
 */
int dev_close(osDev_t dev, osThreadId thread_id)
{
    if (!dev_check_res(dev, thread_id)) {
        return DEV_CERR_NLOCK;
    }
    dev_alloc_table[DEV_MAJOR(dev)].flags ^= DEV_FLAG_LOCK;

    /* TODO wait for dev to be not busy? */

    /* This is bit stupid but might be the easiest way to implement this :/ */
    dev_threadDevSignal(SCHED_DEV_WAIT_BIT, dev);

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
 * @return Error code.
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
 * @return Error code.
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
 * @return Error code.
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
 * @return Error code.
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
 * @return Error code.
 */
int dev_bseek(int offset, int origin, size_t size, osDev_t dev, osThreadId thread_id)
{
    return DEV_BSK_INTERNAL;
}

/**
 * Wait for device
 * @param dev Device that should be waited for; 0 = reset;
 * @return Event.
 */
osEvent * dev_threadDevWait(osDev_t dev, uint32_t millisec)
{
    current_thread->dev_wait = DEV_MAJOR(dev);

    if (dev == 0) {
        current_thread->event.status = osOK;
        return (osEvent *)(&(current_thread->event));
    }

    return ksignal_threadSignalWait(SCHED_DEV_WAIT_BIT, millisec);
}

/**
 * Send a signal that a dev resource is free now.
 * @param signal signal mask
 */
void dev_threadDevSignal(int32_t signal, osDev_t dev)
{
    int i;
    unsigned int temp_dev = DEV_MAJOR(dev);
    threadInfo_t * thread;

    /* This is unfortunately O(n) :'(
     * TODO Some prioritizing would be very nice at least.
     *      Possibly if we would just add waiting threads first to a
     *      priority queue? Is it a waste of cpu time? It isn't actually
     *      a quaranteed way to remove starvation so starvation would be
     *      still there :/
     */
    i = 0;
    do {
        thread = sched_get_pThreadInfo(i);
        if (   ((thread->sig_wait_mask & signal)    != 0)
            && ((thread->flags & SCHED_IN_USE_FLAG) != 0)
            && ((thread->flags & SCHED_NO_SIG_FLAG) == 0)
            && ((thread->dev_wait) == temp_dev)) {
            thread->dev_wait = 0u;
            /* I feel this is bit wrong but we won't save and return
             * prev_signals since no one cares... */
            ksignal_threadSignalSet(i, signal);
            /* We also assume that the signaling was succeed, if it wasn't we
             * are in deep trouble. But it will never happen!
             */

            return; /* Return now so other threads will keep waiting for their
                     * turn. */
        }
    } while (++i < configSCHED_MAX_THREADS);

    return; /* Thre is no threads waiting for this signal,
             * so we wasted a lot of cpu time.             */
}

/**
 * Dev syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uint32_t dev_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_DEV_OPEN:
        return (uint32_t)dev_open(
                    *((osDev_t *)p),
                    (osThreadId)(current_thread->id)
               );

    case SYSCALL_DEV_CLOSE:
        return (uint32_t)dev_close(
                    *((osDev_t *)p),
                    (osThreadId)(current_thread->id)
               );

    case SYSCALL_DEV_CHECK_RES:
        return (uint32_t)dev_check_res(
                    ((ds_osDevHndl_t *)p)->dev,
                    ((ds_osDevHndl_t *)p)->thread_id
               );

    case SYSCALL_DEV_CWRITE:
        return (uint32_t)dev_cwrite(
                    ((ds_osDevData_t *)p)->dev,
                    *(((ds_osDevData_t *)p)->data)
               );

    case SYSCALL_DEV_WAIT:
        return (uint32_t)dev_threadDevWait(
                    ((ds_osDevWait_t *)p)->dev,
                    ((ds_osDevWait_t *)p)->millisec
               );

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */
