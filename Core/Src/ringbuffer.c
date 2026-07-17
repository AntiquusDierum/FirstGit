/*
 * ringbuffer.c
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */
#include "ringbuffer.h"

void RingBuffer_Init(RingBuffer_t *rb,
                     uint8_t *buffer,
                     uint16_t size)
{
    rb->buffer = buffer;

    rb->size = size;

    rb->head = 0;

    rb->tail = 0;
}

bool RingBuffer_Put(RingBuffer_t *rb,
                    uint8_t byte)
{
    uint16_t next = (rb->head + 1U) % rb->size;

    if (next == rb->tail)
    {
        return false;
    }

    rb->buffer[rb->head] = byte;

    rb->head = next;

    return true;
}

bool RingBuffer_Get(RingBuffer_t *rb,
                    uint8_t *byte)
{
    if (rb->head == rb->tail)
    {
        return false;
    }

    *byte = rb->buffer[rb->tail];

    rb->tail = (rb->tail + 1U) % rb->size;

    return true;
}

bool RingBuffer_IsEmpty(RingBuffer_t *rb)
{
    return (rb->head == rb->tail);
}

uint16_t RingBuffer_Count(RingBuffer_t *rb)
{
    if (rb->head >= rb->tail)
    {
        return rb->head - rb->tail;
    }

    return rb->size - rb->tail + rb->head;
}

