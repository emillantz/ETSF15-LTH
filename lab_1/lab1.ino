#include <stdio.h>
// 1
byte tx_bit = (tx.frame >> i - 1) & 0x01;

// 2
const byte PREAMBLE = 0b10101010;
const int PIN_RX = 0;
const int PIN_TX = 13;
for (int i = 0; i < 8; i++)
{
    tx_bit = (PREAMBLE >> i - 1) & 0x01;
    digitalWrite(PIN_TX, tx_bit);
    delay(100);
}

// 3
rx.frame = (rx.frame << 1) | rx_bit;

// 4
tx.frame_payload = Shield::selectLed();