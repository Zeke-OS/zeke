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

/**
 * Queue control block.
 */
typedef struct queue_cb {
    void * data;    /*!< Pointer to the data array used for queue. */
    size_t b_size;  /*!< Block size in bytes. */
    size_t a_len;   /*!< Array length. */
    int m_write;
    int m_read;
} queue_cb_t;

queue_cb_t queue_create(void * data_array, size_t block_size, size_t array_size);
int queue_push(queue_cb_t * cb, void * element);
int queue_pop(queue_cb_t * cb, void * element);
void queue_clearFromPushEnd(queue_cb_t * cb);
void queue_clearFromPopEnd(queue_cb_t * cb);
int queue_isEmpty(queue_cb_t * cb);
int queue_isFull(queue_cb_t * cb);

#endif /* QUEUE_H */
