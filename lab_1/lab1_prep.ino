#include <stdio.h>
// 1
byte tx_bit = (tx.frame << i) & 0x80000000; // 0x80000000 = 0b100..00 (32 bits)
(tx_bit == 0x80000000) ? tx_bit = 0x1 : tx_bit = 0x0;

// 2
const byte PREAMBLE = 0b10101010;
const int PIN_RX = 0;
const int PIN_TX = 13;
for (int i = 0; i < 8; i++)
{
    tx_bit = (PREAMBLE << i) & 0x80; // 0x80 = 0b1000..0 (8 bits)
    (tx_bit == 0x80) ? digitalWrite(PIN_TX, 0x1) : digitalWrite(PIN_TX, 0x0);
    delay(100);
}

// 3
rx.frame = (rx.frame << 1) | rx_bit;

// 4
Shield sh();
tx.frame_payload = sh.selectLed();