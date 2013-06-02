/**
 *******************************************************************************
 * @file    queue.c
 * @author  Olli Vanhoja
 * @brief   Thread-safe queue
 *******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "queue.h"

/**
 * Create a new queue control block.
 * Initializes a new queue control block and returns it as a value.
 * @param data_array an array where the actual queue data is stored.
 * @param block_size the size of single data block/struct/data type in data_array in bytes.
 * @param arra_size the size of the data_array in bytes.
 * @return a new queue_cb_t queue control block structure.
 */
queue_cb_t queue_create(void * data_array, size_t block_size, size_t array_size)
{
    queue_cb_t cb;

    cb.data = data_array;
    cb.b_size = block_size;
    cb.a_len = array_size / block_size;
    cb.m_read = 0;
    cb.m_write = 0;

    return cb;
}

/**
 * Push element to the queue.
 * @param cb the queue control block.
 * @param element the new element to be copied.
 * @return 0 if queue is already full; otherwise operation was succeed.
 * @note element is always copied to the queue, so it is safe to remove the
 * original data after a push.
 */
int queue_push(queue_cb_t * cb, void * element)
{
    int nextElement = (cb->m_write + 1) % cb->a_len;
    int b_size = cb->b_size;

    /* Check that queue is not full */
    if(nextElement == cb->m_read)
        return 0;

    /* Copy element to the data array */
    memcpy(&((uint8_t *)(cb->data))[cb->m_write * b_size], element, b_size);

    cb->m_write = nextElement;
    return 1;
}

/**
 * Pop element from the queue.
 * @param cb the queue control block.
 * @param element location where element is copied to from the queue.
 * @return 0 if queue is empty; otherwise operation was succeed.
 */
int queue_pop(queue_cb_t * cb, void * element)
{
    int read = cb->m_read;
    int write = cb->m_write;
    int b_size = cb->b_size;
    int nextElement;

    /* Check that queue is not empty */
    if(read == write)
        return 0;

    nextElement = (read + 1) % cb->a_len;

    /* Copy from data array to the element */
    memcpy(element, &((uint8_t *)(cb->data))[read * b_size], b_size);

    cb->m_read = nextElement;
    return 1;
}

/**
 * Clear the queue.
 * This operation is considered safe when committed from the push end thread.
 * @param cb the queue control block.
 */
void queue_clearFromPushEnd(queue_cb_t * cb)
{
    cb->m_write = cb->m_read;
}

/**
 * Clear the queue.
 * This operation is considered safe when committed from the pop end thread.
 * @param cb the queue control block.
 */
void queue_clearFromPopEnd(queue_cb_t * cb)
{
    cb->m_read = cb->m_write;
}

int queue_isEmpty(queue_cb_t * cb)
{
    return cb->m_write == cb->m_read;
}

int queue_isFull(queue_cb_t * cb)
{
    return ((cb->m_write + 1) % cb->a_len) == cb->m_read;
}

/**
 * Seek queue.
 */
int seek(queue_cb_t * cb, int i, void * element)
{
    int read = cb->m_read;
    int write = cb->m_write;

    /* Check that queue is not empty */
    if((read % cb->a_len) == write)
        return 0;

    int element_i = (read + i) % cb->a_len;

    /* Check that we don't hit write position */
    if(element_i == write)
        return 0;

    /* Copy from data array to the element */
    memcpy(element, &((uint8_t *)(cb->data))[element_i * cb->b_size], cb->b_size);

    return 1;
}
