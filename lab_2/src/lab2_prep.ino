#include <stdio.h>
#include <Arduino.h>

const byte DIVISOR = 0xA7;
// I think this works??
unsigned long generate_crc(int codeword)
{
    unsigned long crc = 0;
    for (int i = 0; i < 32; i++)
    {
        if ((codeword & 0x80) != 0)
        {
            codeword = codeword ^ (DIVISOR << (31 - i));
        }
        codeword <<= 1;
    }
    crc = codeword;
    return crc;
}