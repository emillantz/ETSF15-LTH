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
#include "UnoArduSimV2.9.2/Include_3rdParty/datacommsimlib.h"
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
const int TIMEOUT = 20000;								  // 20000 ms
const int LEN_TRAIN = LEN_PREAMBLE + LEN_SFD + LEN_FRAME; // For task 4
byte inputFrame;
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
		Serial.println("[State] L1_SEND");
		// +++ add code here and to the predefined function void l1_send(unsigned long l2frame, int framelen) below
		l1_send(tx.frame, LEN_FRAME);
		state = L1_RECEIVE;
		// ---
		break;

	case L1_RECEIVE:
		Serial.println("[State] L1_RECEIVE");
		// +++ add code here and to the predifend function boolean l1_receive(int timeout) below
		l1_receive(TIMEOUT) ? state = L2_FRAME_REC : state = APP_PRODUCE;
		// ---
		break;

	case L2_DATA_SEND:
		Serial.println("[State] L2_DATA_SEND");
		// +++ add code here
		tx.frame_from = SOURCE;
		tx.frame_to = DEST;
		tx.frame_type = 0;	 // Ask
		tx.frame_seqnum = 0; // Ask
		tx.frame_payload = led_pin;
		tx.frame_crc = 0; // Ask
		tx.frame_generation();
		// ---
		state = L1_SEND;
		break;

	case L2_RETRANSMIT:
		Serial.println("[State] L2_RETRANSMIT");
		// +++ add code here

		// ---
		break;

	case L2_FRAME_REC:
		Serial.println("[State] L2_FRAME_REC");
		// +++ add code here

		// ---
		break;

	case L2_ACK_SEND:
		Serial.println("[State] L2_ACK_SEND");
		// +++ add code here

		// ---
		break;

	case L2_ACK_REC:
		Serial.println("[State] L2_ACK_REC");
		// +++ add code here

		// ---
		break;

	case APP_PRODUCE:
		Serial.println("[State] APP_PRODUCE");

		led_pin = sh.select_led();
		Serial.println(led_pin);

		state = HALT;
		break;

	case APP_ACT:
		Serial.println("[State] APP_ACT");
		// +++ add code here

		// ---
		break;

	case HALT:
		Serial.println("[State] HALT");
		sh.halt();
		break;

	default:
		Serial.println("UNDEFINED STATE");
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
	for (int i = framelen - 1; i >= 0; i--)
	{
		tx_bit = PREAMBLE_SEQ << i & 0x80;
		(tx_bit == 0x80) ? digitalWrite(PIN_TX, 1) : digitalWrite(PIN_TX, 0);
		delay(T_S);
	}
	// Send SFD
	for (int i = framelen - 1; i >= 0; i--)
	{
		tx_bit = SFD_SEQ << i & 0x80;
		(tx_bit == 0x80) ? digitalWrite(PIN_TX, 1) : digitalWrite(PIN_TX, 0);
		delay(T_S);
	}
	// Send frame
	for (int i = framelen - 1; i >= 0; i--)
	{
		tx_bit = frame << i & 0x80000000; // 32 bits? not sure if this should be constant when framelen is a variable?
		(tx_bit == 0x80000000) ? digitalWrite(PIN_TX, 1) : digitalWrite(PIN_TX, 0);
		delay(T_S);
	}
}

boolean l1_receive(int timeout)
{
	int start = millis();
	char buf = 0x00;
	// Wait for preamble
	while (digitalRead(PIN_RX) == 0)
	{
		if (check_timeout(start, TIMEOUT))
		{
			return false;
		}
	}
	// Recieve preamble
	delay(T_S >> 1); // wait for middle of pulse (X >> 1 divides by 2 but faster)
	while (buf != SFD_SEQ)
	{
		if (check_timeout(start, TIMEOUT))
		{
			return false;
		}
		buf = buf << 1 | digitalRead(PIN_RX);
		delay(T_S);
	}
	digitalWrite(DEB_1, buf);
	// Zero out buffer
	free(&buf);
	byte buf = 0x00;

	for (int i = 0; i < LEN_FRAME; i++)
	{
		int rx_val = digitalRead(PIN_RX);
		buf = buf << 1 | rx_val;
		digitalWrite(DEB_2, rx_val);
		Serial.println(rx_val);
		delay(T_S);
	}
	inputFrame = buf;
	return true;
}

boolean check_timeout(int start, int timeout)
{
	return (millis() - start > timeout);
}
//////////////////////////////////////////////////////////
//
// Add your functions here
//