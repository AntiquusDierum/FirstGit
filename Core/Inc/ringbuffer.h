/*
 * ringbuffer.h
 *
 *  Created on: 9 Jul 2026
 *      Author: alan
 */

#ifndef INC_RINGBUFFER_H_
#define INC_RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t *buffer;

    uint16_t size;

    volatile uint16_t head;

    volatile uint16_t tail;

} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t *rb,
                     uint8_t *buffer,
                     uint16_t size);

bool RingBuffer_Put(RingBuffer_t *rb,
                    uint8_t byte);

bool RingBuffer_Get(RingBuffer_t *rb,
                    uint8_t *byte);

bool RingBuffer_IsEmpty(RingBuffer_t *rb);

bool RingBuffer_IsFull(RingBuffer_t *rb);

uint16_t RingBuffer_Count(RingBuffer_t *rb);

#endif /* INC_RINGBUFFER_H_ */
