////////////////////////////
//
// V21.1 Skeleton.ino
//
// Adapted to the physical and simulated environments
//
// 2022-12-17 Jens Andersson
//
#include <datacommlib.h>

void l1_send(unsigned long l2frame, int framelen);
boolean l1_receive(int timeout);
int read_ith_tx_bit(int i);
void write_ith_rx_bit(int i, int rx_bit);
void preamble_send();
void SFD_send();
void select_LED_and_address();
boolean header_receive(unsigned long start, int timeout);
byte crc_calculate(unsigned long l2frame);
void crc_generate(unsigned long l2frame);
boolean crc_check(unsigned long l2frame);

int state;
Shield sh; // note! no () since constructor takes no arguments
Transmit tx;
Receive rx;
int seq_nbr;
unsigned long start;
// bool crc_active;
const int DIP4 = 03;
int attempts;

//////////////////////////////////////////////////////////

//
// Code
//
void setup()
{
	sh.begin();
	seq_nbr = 0;
	attempts = 0;

	//////////////////////////////////////////////////////////
	//
	// Add init code here
	//
	pinMode(PIN_RX, INPUT);
	pinMode(PIN_TX, OUTPUT);
	pinMode(DEB_1, OUTPUT);
	pinMode(DEB_2, OUTPUT);
	sh.setMyAddress(5);
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

		sh.allDebsOff();
		// +++ add code here and to the predefined function void l1_send(unsigned long l2frame, int framelen) below
		l1_send(tx.frame, LEN_FRAME);
		start = millis();
		state = L1_RECEIVE;
		// ---
		break;

	case L1_RECEIVE:
		Serial.println("[State] L1_RECEIVE");
		// +++ add code here and to the predifend function boolean l1_receive(int timeout) below
		sh.allDebsOff();
		if (!l1_receive(20000))
		{
			Serial.println("The frame has not been found.");
			state = L2_RETRANSMIT;
			break;
		}

		Serial.println("The frame has been successfully determined.");
		state = L2_FRAME_REC;
		// ---
		break;

	case L2_DATA_SEND:
		Serial.println("[State] L2_DATA_SEND");
		// +++ add code here

		// Payload added in APP_PRODUCE state
		tx.frame_from = sh.getMyAddress();
		tx.frame_to = tx.message[MESSAGE_ADDRESS];
		tx.frame_type = FRAME_TYPE_DATA;
		tx.frame_seqnum = seq_nbr;
		tx.frame_payload = tx.message[MESSAGE_PAYLOAD];
		tx.frame_crc = 0x0;
		tx.frame_generation();
		tx.print_frame();
		crc_generate(tx.frame);

		state = L1_SEND;
		// ---
		break;

	case L2_RETRANSMIT:
		Serial.println("[State] L2_RETRANSMIT");
		// +++ add code here
		if (attempts < MAX_TX_ATTEMPTS)
		{
			attempts = attempts + 1;
			state = L1_SEND;
		}
		else
		{
			attempts = 0;
			state = APP_PRODUCE;
			Serial.println("Maximum number of attempts has been reached.");
		}
		// ---
		break;

	case L2_FRAME_REC:
		Serial.println("[State] L2_FRAME_REC");

		if (crc_check(rx.frame) == false)
		{
			Serial.println("crc active");
			// If not, silently drop the frame and start listening again
			state = L1_RECEIVE;
			break;
		}

		rx.frame_decompose();

		// check address
		if (rx.frame_from != sh.get_address())
		{
			Serial.println(tx.frame_to);
			Serial.println("Address wrong!");
			// If not, re-transmit
			state = L1_RECEIVE;
			break;
		}

		digitalWrite(DEB_3, HIGH); // Light third debug led if CRC OK

		switch (rx.frame_type)
		{
		case FRAME_TYPE_ACK:
			Serial.println("Frame type is ACK");
			state = L2_ACK_REC;
			break;
		case FRAME_TYPE_DATA:
			Serial.println("Frame type is DATA");
			state = L1_RECEIVE;
			break;
		default:
			Serial.print(rx.frame_type);
			Serial.print(" is the type. ");
			Serial.println("Type could not be interpreted.");
			state = APP_PRODUCE;
			break;
		}
		break;

	case L2_ACK_SEND:
		Serial.println("[State] L2_ACK_SEND");
		// FOR MASTER NODE
		break;

	case L2_ACK_REC:
		Serial.println("[State] L2_ACK_REC");
		// +++ add code here
		// check that the ACK’s source address is equal to the destination address of the just transmitted data frame
		if (rx.frame_from != sh.get_address())
		{
			Serial.println(tx.frame_to + "tx");
			Serial.println(rx.frame_from + "rx");
			Serial.println("Not from reciever");
			// If not, re-transmit
			state = L1_RECEIVE;
			break;
		}

		// Check if the sequence number is correct. An incorrect sequence number can be received by flipping the DIP 3 switch on the Master Node
		if (rx.frame_seqnum != tx.frame_seqnum)
		{
			Serial.println("Sequencenumber wrong!");
			// If not, re-transmit
			state = L1_RECEIVE;
			break;
		}

		// If everything is OK, go back to selecting a new LED
		Serial.println("ACK Receive OK!");
		state = APP_PRODUCE;
		// ---
		break;

	case APP_PRODUCE:
		Serial.println("[State] APP_PRODUCE");
		// +++ add code here
		select_LED_and_address(); // Select one of three LEDs and get address from DIP

		if (sh.get_address() == sh.getMyAddress())
		{
			Serial.println("ERROR: Can not use the same address for sender and receiver!");
			break;
		}
		seq_nbr += 1;
		state = L2_DATA_SEND;

		// ---
		break;

	case APP_ACT:
		Serial.println("[State] APP_ACT");
		// +++ add code here
		tx.message[MESSAGE_PAYLOAD] = rx.frame_payload; // Not sure if this is correct
		l1_send(tx.frame, LEN_FRAME);					// Send the full l1 frame

		state = APP_PRODUCE;
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
void select_LED_and_address()
{
	tx.message[MESSAGE_PAYLOAD] = sh.select_led();
	byte MASTER_address = sh.get_address();
	tx.message[MESSAGE_ADDRESS] = MASTER_address; // Get the destination address from the Development Node DIP switches (1 - 3)

	Serial.print("Selected LED: ");
	Serial.print(tx.message[MESSAGE_PAYLOAD]);
	Serial.print(" Selected address: ");
	Serial.print(tx.message[MESSAGE_ADDRESS]);
}

void l1_send(unsigned long l2frame, int framelen)
{
	Serial.print("\nSending L1 frame: ");

	preamble_send();

	Serial.print(" ");

	SFD_send();

	Serial.print(" ");

	byte send;
	Serial.print("Frame: ");
	for (int i = framelen - 1; i >= 0; i--)
	{
		send = ((l2frame >> i) & 0x1) == 0 ? 0 : 1;
		digitalWrite(PIN_TX, (send == 0) ? LOW : HIGH);
		Serial.print(send);
		if (i % 8 == 0)
		{
			Serial.print(" ");
		}
		delay(T_S);
	}

	Serial.println();
}

boolean l1_receive(int timeout)
{

	while (millis() - start < timeout)
	{
		if (sh.sampleRecCh(PIN_RX) == 1)
		{
			Serial.println();
			Serial.print("Receiving L1 frame: ");

			if (header_receive(timeout))
			{
				frame_receive();

				return true;
			}
			return false;
		}
	}

	return false;
}

void preamble_send()
{
	byte preamble = PREAMBLE_SEQ;
	byte msb;
	Serial.print("PRE: ");

	for (int i = 0; i < LEN_PREAMBLE; i++)
	{
		msb = ((preamble & 0x80) == 0) ? 0 : 1;
		digitalWrite(PIN_TX, (msb == 0) ? LOW : HIGH);
		Serial.print(msb);
		preamble = preamble << 1;
		delay(T_S);
	}
}

void SFD_send()
{
	byte SFD = SFD_SEQ; // 01111110
	byte msb;
	Serial.print("SFD: ");
	msb = 0;
	for (int i = 0; i < LEN_SFD; i++)
	{
		msb = ((SFD & 0x80) == 0) ? 0 : 1;
		digitalWrite(PIN_TX, (msb == 0) ? LOW : HIGH);
		Serial.print(msb);
		SFD = SFD << 1;
		delay(T_S);
	}
}

int read_ith_tx_bit(int i)
{
	int tx_bit;
	tx_bit = ((tx.frame << i) & 0x80000000) == 0 ? 0x0 : 0x1;
	return tx_bit;
}

void write_ith_rx_bit(int i, int rx_bit)
{
	unsigned long temp = (rx_bit == 0x0) ? 0 : 1;
	temp = temp >> (LEN_FRAME - i - 1);
	rx.frame = rx.frame | temp;
}

boolean header_receive(int timeout)
{
	delay(T_S / 2);
	byte bit = 0b0;
	bit = sh.sampleRecCh(PIN_RX);
	byte buffer = 0b0;
	buffer = buffer | bit;
	Serial.print(bit);

	while (millis() - start < timeout)
	{
		delay(T_S);
		buffer = buffer << 1;
		bit = sh.sampleRecCh(PIN_RX);
		buffer = buffer | bit;
		digitalWrite(DEB_1, (bit == 0) ? LOW : HIGH);
		Serial.print(bit);

		// Only checks SFD after first bit detection
		if (buffer == SFD_SEQ)
		{
			digitalWrite(DEB_1, HIGH);
			Serial.print(" ");
			return true;
		}
	}

	return false;
}

void frame_receive()
{
	Serial.print("Receiving frame (void frame_receive): ");
	unsigned long frame_buffer;
	int rx_bit;

	for (int i = 0; i < LEN_FRAME; i++)
	{
		delay(T_S);
		frame_buffer = frame_buffer << 1;
		rx_bit = sh.sampleRecCh(PIN_RX);
		digitalWrite(DEB_2, (rx_bit == 0) ? LOW : HIGH);
		frame_buffer = frame_buffer | rx_bit;
		Serial.print(rx_bit);

		if ((i + 1) % 8 == 0)
		{
			Serial.print(" ");
		}
	}

	rx.frame = frame_buffer;

	digitalWrite(DEB_2, HIGH);
	Serial.println("\n");
}

void crc_generate(unsigned long l2frame)
{
	tx.add_crc(crc_calculate(l2frame));
}

byte crc_calculate(unsigned long l2frame)
{
	unsigned long shift = l2frame;
	unsigned long generator = 0b10100111;
	generator = generator << (LEN_FRAME - LEN_FRAME_CRC);

	for (int i = 0; i < (LEN_FRAME - LEN_FRAME_CRC); i++)
	{
		if ((shift & 0x80000000) != 0)
		{
			shift = shift << 1;
			shift = shift ^ generator;
		}
		else
		{
			shift = shift << 1;
		}
	}

	byte remainder = (byte)(shift >> (LEN_FRAME - LEN_FRAME_CRC)); // Move the result to the first byte and do a byte conversion

	Serial.print("CRC REMAINDER: ");
	Serial.println(remainder);
	if (remainder == 0)
	{
		Serial.println("CRC OK");
	}
	return remainder;
}

boolean crc_check(unsigned long l2frame)
{
	return crc_calculate(l2frame) == 0;
}

//////////////////////////////////////////////////////////
//
// Add your functions here
//
