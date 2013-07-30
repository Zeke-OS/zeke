/**
 *******************************************************************************
 * @file    semaphore.h
 * @author  Olli Vanhoja
 * @brief   Semaphore
 *******************************************************************************
 */

#pragma once
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

/** Thread must still wait for a semaphore token. */
#define OS_SEMAPHORE_THREAD_SPINWAIT_WAITING    -1
/** Can't get a timeout timer for the thread. */
#define OS_SEMAPHORE_THREAD_SPINWAIT_RES_ERROR  -2

/**
 * Semaphore control block
 */
typedef struct os_semaphore_cb {
    uint32_t s;
    uint32_t count;
} os_semaphore_cb_t;

#endif /* SEMAPHORE_H */
