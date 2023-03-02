#include "datacommlib.h"
#include <stdio.h>
#include <Arduino.h>

#define REG_SIZE 8				  // size of register
#define ALLOWED_RETRANSMISSIONS 3 // number of retransmissions allowed

void l1_send(unsigned long frame, int framelen);
boolean l1_receive(unsigned long timeout);
int generate_crc(int frame);
boolean check_crc(int frame);

unsigned long rx_start = 0; // time when we start receiving
int temp = 0;
int seq_num = 0; // unique sequence number for each transmission
int state = NONE;
const int DIVISOR = 0xA7;

Shield sh;
Transmit tx;
Receive rx;

void setup()
{
	sh.begin();
	sh.setMyAddress(0b101); // decimal: 5
	state = APP_PRODUCE;
}

void loop()
{
	switch (state)
	{
	case L1_SEND:
		Serial.println("[State] L1_SEND");

		// send the preamble, SFD and frame
		l1_send(PREAMBLE_SEQ, LEN_PREAMBLE);
		l1_send(SFD_SEQ, LEN_SFD);
		l1_send(tx.frame, LEN_FRAME);

		state = L1_RECEIVE;
		break;

	case L1_RECEIVE:
		Serial.println("[State] L1_RECEIVE");
		rx_start = millis();
		// if we don't receive a message within 20s
		if (l1_receive(20000))
		{
			Serial.println("Recieve timeout, retransmitting");
			state = L2_RETRANSMIT;
			break;
		}

		Serial.println("Message recieved");
		state = L2_FRAME_REC;

		break;

	case L2_DATA_SEND:
		Serial.println("[State] L2_DATA_SEND");
		// get the address from the shield
		tx.frame_from = sh.getMyAddress();

		// set the to-address and type to be data
		tx.frame_to = tx.message[MESSAGE_ADDRESS];
		tx.frame_type = FRAME_TYPE_DATA;

		tx.frame_payload = tx.message[MESSAGE_PAYLOAD];

		// set the current sequence number
		tx.frame_seqnum = seq_num;

		tx.frame_crc = generate_crc(tx.frame);
		tx.frame_generation();

		state = L1_SEND;

		seq_num++;

		break;

	case L2_RETRANSMIT:
		Serial.println("[State] L2_RETRANSMIT");

		// if we have tried too many times
		if (tx.tx_attempts >= ALLOWED_RETRANSMISSIONS)
		{
			Serial.println("Too many attempts");
			// give up and go to app_produce
			state = APP_PRODUCE;
			break;
		}

		// increment attempts
		tx.tx_attempts++;
		// try again
		state = L1_SEND;
		break;

	case L2_FRAME_REC:
		Serial.println("[State] L2_FRAME_REC");
		rx.frame_decompose();

		if (!(rx.frame_to != sh.getMyAddress()))
		{
			Serial.println("Incorrect address");
			state = L1_RECEIVE;
			break;
		}

		if (rx.frame_type != FRAME_TYPE_ACK)
		{
			Serial.println("Incorrect frame type");
			state = L1_SEND;
			break;
		}

		if (!check_crc(rx.frame))
		{
			Serial.println("Incorrect CRC");
			state = L1_SEND;
			break;
		}

		state = L2_ACK_REC;
		break;

	case L2_ACK_SEND:
		Serial.println("[State] L2_ACK_SEND");
		break;

	case L2_ACK_REC:
		Serial.println("[State] L2_ACK_REC");

		if (rx.frame_seqnum != tx.frame_seqnum)
		{
			Serial.println("Incorrect sequence number");
			state = APP_PRODUCE;
			break;
		}

		state = L2_RETRANSMIT;
		break;

	case APP_PRODUCE:
		Serial.println("[State] APP_PRODUCE");

		digitalWrite(DEB_1, 0);
		digitalWrite(DEB_2, 0);

		tx.message[MESSAGE_ADDRESS] = sh.get_address();
		tx.message[MESSAGE_PAYLOAD] = sh.select_led();

		state = L2_DATA_SEND;
		break;

	case APP_ACT:
		Serial.println("[State] APP_ACT");
		break;

	case HALT:
		Serial.println("[State] HALT");
		sh.halt();
		break;

	default:
		Serial.println("UNDEFINED STATE");
		break;
	}
}
//
// Add code to the predfined functions
//
void l1_send(unsigned long frame, int framelen)
{
	for (int i = framelen; i > 0; i--)
	{
		int shifted = ((frame >> (i - 1)) & 0x1);

		digitalWrite(PIN_TX, shifted);
		delay(T_S);
		Serial.print(shifted);
	}

	Serial.println("");
}

boolean l1_receive(unsigned long timeout)
{
	digitalWrite(DEB_1, 0);
	digitalWrite(DEB_2, 0);

	unsigned long l1_header = 0;
	unsigned long l2_data = 0;
	int l1_header_len = 0;
	int l2_data_len = 0;

	while (millis() - rx_start < timeout)
	{
		// if we get a 0 we wait for a 1
		if (!sh.sampleRecCh(PIN_RX))
		{
			continue;
		}

		delay(T_S / 2); // delay half a period

		l1_header = sh.sampleRecCh(PIN_RX);

		// Flash led
		digitalWrite(DEB_1, sh.sampleRecCh(PIN_RX));
		l1_header_len++;

		for (int i = 0; i < 47; i++)
		{
			if (i == 15)
			{
				digitalWrite(DEB_1, 1);
			}

			if (i < 15)
			{
				delay(T_S);
				// Flash led
				digitalWrite(DEB_1, sh.sampleRecCh(PIN_RX));

				l1_header <<= 1;
				l1_header += sh.sampleRecCh(PIN_RX);
				l1_header_len++;
			}

			boolean isSFD = (l1_header & 0b11111111) == SFD_SEQ;

			if (i >= 15 && isSFD)
			{
				delay(T_S);
				// Flash led
				digitalWrite(DEB_2, sh.sampleRecCh(PIN_RX));
				l2_data <<= 1; // We really dont care about the payload so just look at
							   // the CRC, last 8 bits.
				l2_data += sh.sampleRecCh(PIN_RX);
				l2_data_len++;
			}
			if (i >= 15 && isSFD)
			{
				Serial.println("SFD didnt check out, try again");
				return false;
			}
		}

		Serial.println(l1_header, BIN);
		Serial.println(l1_header_len);
		Serial.println(l2_data, BIN);
		Serial.println(l2_data_len);

		digitalWrite(DEB_2, 1);

		rx.frame = l2_data;
		return true;
	}

	Serial.println("Timeout reached");
	return false;
}

//////////////////////////////////////////////////////////
//
// Add your functions here
//

int generate_crc(int frame)
{
	byte crc = frame;

	for (int i = 0; i < LEN_FRAME - 1; i++)
	{
		((crc & 0x80) != 0) ? (crc = (byte)((crc << 1) ^ DIVISOR)) : (crc <<= 1);
	}
	return crc;
}

boolean check_crc(int frame)
{
	return generate_crc(frame) == 0;
}
