/**
 *******************************************************************************
 * @file    devfs.c
 * @author  Olli Vanhoja
 * @brief   Device driver subsystem/devfs.
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

#include <kstring.h>
#include <sched.h>
#include <ksignal.h>
#include <syscalldef.h>
#include <syscall.h>
#include <fs/fs.h>
#include "devfs.h"

/** TODO dev syscalls */
/** TODO How can we wait for dev to unlock? osSignalWait? */
/** TODO dev syscall should first try to access device and then go
 *       to wait if desired so */
/** TODO Especially non-lockable devs could have message queues for
 *  storing the data */

struct dev_driver dev_alloc_table[DEV_MAJORDEVS];

fs_t devfs {
    .fsname = "devfs";
    .mount = 0;
    .umount = 0;
}

void devfs_init(void) __attribute__((constructor));

void devfs_init(void)
{
    fs_register(&devfs);
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
  * @}
  */

/**
  * @}
  */
