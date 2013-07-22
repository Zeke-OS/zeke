/**
 *******************************************************************************
 * @file    queue.h
 * @author  Olli Vanhoja
 * @brief   Thread-safe queue
 *******************************************************************************
 */

#pragma once
#ifndef QUEUE_H
#define QUEUE_H

#include "string.h"

/**
 * Queue control block.
 */
typedef struct queue_cb {
    void * data;    /*!< Pointer to the data array used for queue. */
    ksize_t b_size;  /*!< Block size in bytes. */
    ksize_t a_len;   /*!< Array length. */
    int m_write;
    int m_read;
} queue_cb_t;

/**
 * Create a new queue control block.
 * Initializes a new queue control block and returns it as a value.
 * @param data_array an array where the actual queue data is stored.
 * @param block_size the size of single data block/struct/data type in data_array in bytes.
 * @param arra_size the size of the data_array in bytes.
 * @return a new queue_cb_t queue control block structure.
 */
queue_cb_t queue_create(void * data_array, ksize_t block_size, ksize_t array_size);

/**
 * Push element to the queue.
 * @param cb the queue control block.
 * @param element the new element to be copied.
 * @return 0 if queue is already full; otherwise operation was succeed.
 * @note element is always copied to the queue, so it is safe to remove the
 * original data after a push.
 */
int queue_push(queue_cb_t * cb, void * element);

/**
 * Pop element from the queue.
 * @param cb the queue control block.
 * @param element location where element is copied to from the queue.
 * @return 0 if queue is empty; otherwise operation was succeed.
 */
int queue_pop(queue_cb_t * cb, void * element);

/**
 * Clear the queue.
 * This operation is considered safe when committed from the push end thread.
 * @param cb the queue control block.
 */
void queue_clearFromPushEnd(queue_cb_t * cb);

/**
 * Clear the queue.
 * This operation is considered safe when committed from the pop end thread.
 * @param cb the queue control block.
 */
void queue_clearFromPopEnd(queue_cb_t * cb);

/**
 * Check if the queue is empty.
 * @param cb the queue control block.
 * @return 0 if the queue is not empty.
 */
int queue_isEmpty(queue_cb_t * cb);

/**
 * Check if the queue is full.
 * @param cb the queue control block.
 * @return 0 if the queue is not full.
 */
int queue_isFull(queue_cb_t * cb);

/**
 * Seek queue.
 * @param cb the queue control block.
 * @param i index.
 * @param[in] element returned element.
 * @return 0 if failed; otherwise succeed.
 */
int seek(queue_cb_t * cb, int i, void * element);

#endif /* QUEUE_H */
