#include "circularBuff.h"

int16_t containsBuff(circularBuffer *CBuff, uint8_t val[CHARACTERS])
{
    for (uint16_t row = 0; row < HASHES; row++)
        if (memcmp(val, CBuff->buff[row], CHARACTERS) == 0)
            return row;
    return -1;
}

uint8_t addToBuff(circularBuffer *CBuff, uint8_t val[CHARACTERS])
{
    int16_t location = containsBuff(CBuff, val);
    if (location == -1)
    {
        memcpy(CBuff->buff[CBuff->next], val, CHARACTERS);
        if (CBuff->next == 255)
        {
            CBuff->next = 0;
            return 255;
        }
        else
        {
            CBuff->next += 1;
            return CBuff->next - 1;
        }
    }
    else
        return location;
}