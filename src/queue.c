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

queue_cb_t queue_create(void * data_array, size_t block_size, size_t array_size)
{
    queue_cb_t cb = {
        .data = data_array,
        .b_size = block_size,
        .a_len = array_size / block_size,
        .m_read = 0,
        .m_write = 0
    };

    return cb;
}

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

void queue_clearFromPushEnd(queue_cb_t * cb)
{
    cb->m_write = cb->m_read;
}

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
