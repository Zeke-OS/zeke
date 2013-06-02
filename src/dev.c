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

static void dev_threadDevSignalSet(osDev_t dev);

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
    dev_threadDevSignalSet(dev);

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
 * Read/Write to a character device.
 * @param ch buffer.
 * @param dev device.
 * @param thread_id thread that is reading/writing to the device.
 * @return Error code.
 */
int dev_crw(ds_osDevCData_t * args, int write, osThreadId thread_id)
{
    osDev_t dev = args->dev;
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && !(DEV_TFLAG_NONLOCK(dev_al->flags))) {
        return DEV_COME_NLOCK;
    }

    if (DEV_TFLAG_FAIL(dev_al->flags)) {
        return DEV_COME_INTERNAL;
    }

    if (write) {
        if (dev_al->cwrite == NULL) {
            return DEV_COME_NDEV;
        }

        return dev_al->cwrite(*(uint32_t *)(args->data), dev);
    } else {
        if (dev_al->cread == NULL) {
            return DEV_COME_NDEV;
        }

        return dev_al->cread(args->data, dev);
    }
}

/**
 * Write to a block device.
 * @param args function parameters in syscall data struct.
 * @param thread_id id of the thread that is writing to the block device.
 * @return Error code.
 */
int dev_brw(ds_osDevBData_t * args, int write, osThreadId thread_id)
{
    osDev_t dev = args->dev;
    struct dev_driver * dev_al = &dev_alloc_table[DEV_MAJOR(dev)];

    if (!dev_check_res(dev, thread_id)
        && !(DEV_TFLAG_NONLOCK(dev_al->flags))) {
        return DEV_COME_NLOCK;
    }

    if (DEV_TFLAG_FAIL(dev_al->flags)) {
        return DEV_COME_INTERNAL;
    }

    if (write) {
        if (dev_al->bwrite == NULL) {
            return DEV_COME_NDEV;
        }

        return dev_al->bwrite(args->buff, args->size, args->count, dev);
    } else {
        if (dev_al->bread == NULL) {
            return DEV_BRD_NOT;
        }

        return dev_al->bread(args->buff, args->size, args->count, dev);
    }
}

/**
 * Seek block device.
 * @TODO Implementation
 * @param args function parameters in syscall data struct.
 * @param thread_id id of the thread that is seeking the block device.
 * @return Error code.
 */
int dev_bseek(ds_osDevBSeekData_t * args, osThreadId thread_id)
{
    /* NOTE dev driver system expects:
     * int (*bseek)(int offset, int origin, size_t size, osDev_t dev, osThreadId thread_id);
     * for the driver.
     *
     * dev_bseek(
     *               ((ds_osDevBseekData_t *)p)->offset,
     *               ((ds_osDevBseekData_t *)p)->origin,
     *               ((ds_osDevBseekData_t *)p)->size,
     *               ((ds_osDevBseekData_t *)p)->dev,
     *               (osThreadId)(current_thread->id)
     */

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
 * @param dev dev
 */
static void dev_threadDevSignalSet(osDev_t dev)
{
    int i;
    unsigned int temp_dev = DEV_MAJOR(dev);
    threadInfo_t * thread;

    /* This is unfortunately O(n) :'(
     */
    i = 0;
    do {
        thread = sched_get_pThreadInfo(i);
        if (   ((thread->sig_wait_mask & SCHED_DEV_WAIT_BIT) != 0)
            && ((thread->flags & SCHED_IN_USE_FLAG) != 0)
            && ((thread->flags & SCHED_NO_SIG_FLAG) == 0)
            && (thread->dev_wait == temp_dev)) {
            /* Update event struct */
            thread->event.value.signals = signal; /* Only this signal */
            thread->event.status = osEventSignal;

            thread->dev_wait = 0u;
            ksignal_threadSignalWaitMaskClear(thread);

            /* Set the signaled thread back into execution */
            sched_thread_set_exec(thread->id);

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
        return (uint32_t)dev_crw(
                    (((ds_osDevCData_t *)p)),
                    1,
                    (osThreadId)(current_thread->id)
               );

    case SYSCALL_DEV_CREAD:
        return (uint32_t)dev_crw(
                    ((ds_osDevCData_t *)p),
                    0,
                    (osThreadId)(current_thread->id)
               );

    case SYSCALL_DEV_BWRITE:
        return (uint32_t)dev_brw(
                    ((ds_osDevBData_t *)p),
                    1,
                    (osThreadId)(current_thread->id)
               );

    case SYSCALL_DEV_BREAD:
        return (uint32_t)dev_brw(
                    ((ds_osDevBData_t *)p),
                    0,
                    (osThreadId)(current_thread->id)
               );

    case SYSCALL_DEV_BSEEK:
        return (uint32_t)dev_bseek(
                    ((ds_osDevBSeekData_t *)p),
                    (osThreadId)(current_thread->id)
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
