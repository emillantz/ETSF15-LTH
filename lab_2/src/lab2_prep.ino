#include <stdio.h>
#include <Arduino.h>

const byte DIVISOR = 0xA7;
// I think this works??
byte Compute_CRC8(byte *bytes, int len)
{
    byte crc = 0;

    while (len--)
    {
        crc ^= *bytes++;

        for (int i = 0; i < 8; i++)
        {
            ((crc & 0x80) != 0) ? (crc = (byte)((crc << 1) ^ DIVISOR)) : (crc <<= 1);
        }
    }
    return crc;
}
