////////////////////////////
//
// V21.1 Skeleton.ino
//
// Adapted to the physical and simulated environments
//
// 2022-12-17 Jens Andersson
//
////////////////////////////

//
// Select library
//
// uncomment for the physical environment
// #include "UnoArduSimV2.9.2/Include_3rdParty/datacommlib.h"
//
// uncomment for the simulated environment
#include "datacommlib.h"
#include <stdio.h>
#include <Arduino.h>

//
// Prototypes
//

// predefined
void l1_send(unsigned long l2frame, int framelen);
boolean l1_receive(int timeout);
// your own

//
// Runtime
//

//////////////////////////////////////////////////////////
//
// Add your global constant and variabel declaretions here
//

int state = NONE;
Shield sh; // note! no () since constructor takes no arguments
Transmit tx;
Receive rx;
int led_pin;
const int SOURCE = 0;
const int DEST = 0;
const unsigned int TIMEOUT = 20000;						  // 20000 ms
const int LEN_TRAIN = LEN_PREAMBLE + LEN_SFD + LEN_FRAME; // For task 4
unsigned long inputFrame;
int mit; // Drift mitigation
//////////////////////////////////////////////////////////
//
// Code
//
void setup()
{
	sh.begin();

	//////////////////////////////////////////////////////////
	//
	// Add init code here
	//

	state = APP_PRODUCE;

	// Set your development node's address here

	//////////////////////////////////////////////////////////
}

void loop()
{

	//////////////////////////////////////////////////////////
	//
	// State machine
	// Add code for the different states here
	//

	switch (state)
	{

	case L1_SEND:
		Serial.println("\n[State] L1_SEND");
		// +++ add code here and to the predefined function void l1_send(unsigned long l2frame, int framelen) below
		l1_send(tx.frame, LEN_FRAME);
		state = L1_RECEIVE;
		// ---
		break;

	case L1_RECEIVE:
		Serial.println("\n[State] L1_RECEIVE");
		// +++ add code here and to the predifend function boolean l1_receive(int timeout) below
		l1_receive(TIMEOUT) ? state = L2_FRAME_REC : state = APP_PRODUCE;
		// ---
		break;

	case L2_DATA_SEND:
		Serial.println("\n[State] L2_DATA_SEND");
		// +++ add code here
		tx.frame_from = SOURCE;
		tx.frame_to = DEST;
		tx.frame_type = FRAME_TYPE_DATA; // Ask
		tx.frame_seqnum = 0;			 // Ask
		tx.frame_payload = tx.message[MESSAGE_PAYLOAD];
		tx.frame_crc = 0; // Ask
		tx.frame_generation();
		// ---
		state = L1_SEND;
		break;

	case L2_RETRANSMIT:
		Serial.println("\n[State] L2_RETRANSMIT");
		// +++ add code here
		// ---
		break;

	case L2_FRAME_REC:
		Serial.println("\n[State] L2_FRAME_REC");
		// +++ add code here
		rx.frame = inputFrame;
		rx.frame_decompose();
		digitalWrite(DEB_1, LOW);
		digitalWrite(DEB_2, LOW);
		state = APP_PRODUCE;
		// ---
		break;

	case L2_ACK_SEND:
		Serial.println("\n[State] L2_ACK_SEND");
		// +++ add code here

		// ---
		break;

	case L2_ACK_REC:
		Serial.println("\n[State] L2_ACK_REC");
		// +++ add code here

		// ---
		break;

	case APP_PRODUCE:
		Serial.println("\n[State] APP_PRODUCE");

		led_pin = sh.select_led();
		tx.message[MESSAGE_PAYLOAD] = led_pin;
		Serial.println(led_pin);

		state = L2_DATA_SEND;
		break;

	case APP_ACT:
		Serial.println("\n[State] APP_ACT");
		// +++ add code here

		// ---
		break;

	case HALT:
		Serial.println("\n[State] HALT");
		sh.halt();
		break;

	default:
		Serial.println("\nUNDEFINED STATE");
		break;
	}

	//////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////
//
// Add code to the predfined functions
//
void l1_send(unsigned long frame, int framelen)
{
	byte tx_bit;
	// Send preamble
	for (int i = LEN_PREAMBLE - 1; i >= 0; i--)
	{
		mit = millis();
		tx_bit = (PREAMBLE_SEQ << i) & 0x80;
		(tx_bit == 0x80) ? digitalWrite(PIN_TX, 1) : digitalWrite(PIN_TX, 0);
		delay(T_S - (millis() - mit));
	}
	// Send SFD
	for (int i = LEN_SFD - 1; i >= 0; i--)
	{
		mit = millis();
		tx_bit = (SFD_SEQ << i) & 0x80;
		(tx_bit == 0x80) ? digitalWrite(PIN_TX, 1) : digitalWrite(PIN_TX, 0);
		delay(T_S - (millis() - mit));
	}
	// Send frame
	Serial.println("Frame:");
	for (int i = framelen - 1; i >= 0; i--)
	{
		mit = millis();
		tx_bit = (((frame >> i) & 0x1) == 1) ? 1 : 0; // 32 bits? not sure if this should be constant when framelen is a variable?
		digitalWrite(PIN_TX, tx_bit);
		Serial.print(tx_bit);
		delay(T_S - (millis() - mit));
	}
}

boolean l1_receive(int timeout)
{
	unsigned int start = millis();
	byte buf1 = 0x00;
	// Wait for preamble
	while (sh.sampleRecCh(PIN_RX) == 0)
	{
		if (check_timeout(start, TIMEOUT))
		{
			return false;
		}
	}
	// Recieve SFD
	delay(T_S / 2); // wait for middle of pulse (X >> 1 divides by 2 but faster)
	Serial.println("SFD Buffer:");
	while (buf1 != SFD_SEQ)
	{
		mit = millis();
		if (check_timeout(start, TIMEOUT))
		{
			return false;
		}
		buf1 = buf1 << 1 | sh.sampleRecCh(PIN_RX);
		Serial.print(buf1);
		delay(T_S - (millis() - mit));
	}
	Serial.println("\nFrame buffer:");
	digitalWrite(DEB_1, HIGH);
	unsigned long buf2 = 0x00000000;

	for (int i = 0; i < LEN_FRAME; i++)
	{
		mit = millis();
		byte rx_val = sh.sampleRecCh(PIN_RX);
		buf2 = (buf2 << 1) | rx_val;
		digitalWrite(DEB_2, rx_val);
		Serial.print(rx_val);
		delay(T_S - (millis() - mit));
	}
	Serial.println(" ");
	Serial.println(buf2, BIN);
	inputFrame = buf2;
	digitalWrite(DEB_2, HIGH);
	return true;
}

boolean check_timeout(int start, int timeout)
{
	return (millis() - start > TIMEOUT);
}
//////////////////////////////////////////////////////////
//
// Add your functions here
//