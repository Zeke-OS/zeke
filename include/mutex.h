/**
 *******************************************************************************
 * @file    mutex.h
 * @author  Olli Vanhoja
 * @brief   Mutex
 *******************************************************************************
 */

#pragma once
#ifndef MUTEX_H
#define MUTEX_H

enum os_mutex_strategy {
    os_mutex_str_reschedule,
    os_mutex_str_sleep
};

/**
 * Mutex control block
 */
typedef struct os_mutex_cb {
    volatile int thread_id;
    volatile int lock;
    enum os_mutex_strategy strategy;
} mutex_cb_t;

#endif /* MUTEX_H */
