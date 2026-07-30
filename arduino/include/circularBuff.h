#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <string.h>

#define HASHES 256
#define CHARACTERS 8

typedef struct
{
    uint8_t buff[HASHES][CHARACTERS];
    uint8_t next;
} circularBuffer;

int16_t containsBuff(circularBuffer *CBuff, uint8_t val[CHARACTERS]);
uint8_t addToBuff(circularBuffer *CBuff, uint8_t val[CHARACTERS]);

#endif