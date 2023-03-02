////////////////////////////////
//
// datacommsimlib.cpp
// date 2022-11-14
// version 4.1.0 for UnoArduSim
//
////////////////////////////////

//#include <datacommlib.h>
#include <datacommsimlib.h>

////////////////////////////////
//
// Shield
//
///////////////////////////////

Shield::Shield() {
	// constructor, do nothing
}

void Shield::begin() {
	int i;  
	Serial.begin(9600);
	Serial.println("DataCommSimLib version 4.1.0");
	Serial.println("[BEGIN]");
	adThreshold = AD_TH;
	Serial.print("Curent AD Threshol = ");
	Serial.println(adThreshold);

	// Set Pins
	pinMode(LED_B, OUTPUT);
	pinMode(LED_R, OUTPUT);
	pinMode(LED_G, OUTPUT);
	pinMode(DEB_1, OUTPUT);
	pinMode(DEB_2, OUTPUT);
	pinMode(DEB_3, OUTPUT);
	pinMode(PIN_TX, OUTPUT);
	pinMode(PIN_BUTTON, INPUT);

	// Address pins
	for (i=0; i<LEN_FRAME_ADDR; i++) {
		pinMode(PIN_ADDR[i], INPUT);
	}

	// Turn off LEDs
	allLedsOff();
	allDebsOff();
	digitalWrite(PIN_TX,LOW);
}

// Select LED
int Shield::select_led() {
	int currentLed;
	// turn all LEDs on
	allLedsOn();
	delay(1000);
	//Wait until button is pushed
	while (!readButtonState()) {
	}
	//the turn off all LEDs
	allLedsOff();
	currentLed = LED_B;
	do {
		digitalWrite(currentLed, HIGH);
		delay(1000);
		if (!readButtonState()) {
			return currentLed;
		}
		digitalWrite(currentLed, LOW);
		currentLed++;
		if (currentLed > LED_R) {
			currentLed = LED_B;
		}
	} while (true);
	return 0; //dummy return
}

// Retreive address from DIP switch
// In the simulated environment only switches 
// connected to pin 5 and 6 are used for addresses

int Shield::get_address(){
	int i, address_dec;
	//byte address_bin[LEN_FRAME_ADDR];
	address_dec = 0;
	for (i=2; i<LEN_FRAME_ADDR; i++) {
		//address_bin[i] = digitalRead(PIN_ADDR[i]);
		address_dec <<= 1;
		address_dec |= digitalRead(PIN_ADDR[i]);
	}
	//address_dec = Shield::binarray_to_int(address_bin, 0, LEN_FRAME_ADDR);

	//Serial.print("[Shield] Address is: ");
	//Serial.println(address_dec);

	return address_dec;
}

// All application LEDs on
void Shield::allLedsOn() {
	//Serial.println("[Shield] All LEDs on");
	digitalWrite(LED_B,HIGH);
	digitalWrite(LED_G,HIGH);
	digitalWrite(LED_R,HIGH);
}

// All application LEDs off
void Shield::allLedsOff() {
	//Serial.println("[Shield] All LEDs off");
	digitalWrite(LED_B,LOW);
	digitalWrite(LED_G,LOW);
	digitalWrite(LED_R,LOW);
}

// All debug LEDs off
void Shield::allDebsOff() {
	digitalWrite(DEB_1,LOW);
	digitalWrite(DEB_2,LOW);
	digitalWrite(DEB_3,LOW);
}

// All debug LEDs on
void Shield::allDebsOn() {
	digitalWrite(DEB_1,HIGH);
	digitalWrite(DEB_2,HIGH);
	digitalWrite(DEB_3,HIGH);
}

void Shield::debsShowNum(int value) {
	if (value & 0x4) {	digitalWrite(DEB_3,HIGH);} else {	digitalWrite(DEB_3,LOW);}
	if (value & 0x2) {	digitalWrite(DEB_2,HIGH);} else {	digitalWrite(DEB_2,LOW);}  
	if (value & 0x1) {	digitalWrite(DEB_1,HIGH);} else {	digitalWrite(DEB_1,LOW);}  
}

// Get current button state
int Shield::readButtonState() {
	const int debounceTime = 50;
	unsigned long lastButtonChangeTime = millis();
	int lastButtonState = LOW;
	int currButtonState;
	do {
		currButtonState = digitalRead(PIN_BUTTON);
		//digitalWrite(tstLedPin,currState); // debug
		if (currButtonState != lastButtonState) {
			// push-button has changed state
			lastButtonState = currButtonState;
			lastButtonChangeTime = millis();
		}
	} while (millis() - lastButtonChangeTime < debounceTime);
	return lastButtonState;
}

// My Address
void Shield::setMyAddress(int value) {
	my_address = value;
}
int Shield::getMyAddress() {
	return my_address;
}

// Sample Receiving Channel (alternative method with same name also in datacomlib)
int Shield::sampleRecCh(int pin){
	return digitalRead(pin);
}

// A/D Converter
int Shield::adConv(int value){
	return value > adThreshold ? 0 : 1; // bug fix (Jens)
}
void Shield::setAdThreshold(int value){
	adThreshold = value;
}
int Shield::getAdThreshold(){
	return adThreshold;
}

// Halt
void Shield::halt(int dly) {
	allDebsOff();
	//allLedsOff();
	while (true) {
		// stay here forever
		digitalWrite(DEB_1,HIGH);
		delay(dly);
		digitalWrite(DEB_2,HIGH);
		delay(dly);
		digitalWrite(DEB_3,HIGH);
		delay(dly);
		digitalWrite(DEB_3,LOW);
		delay(dly);
		digitalWrite(DEB_2,LOW);
		delay(dly);
		digitalWrite(DEB_1,LOW);
		delay(dly);
	}
}

////////////////////////////////
//
// Frame
//
///////////////////////////////

Frame::Frame() {
	// constructor, do nothing

}

////////////////////////////////
//
// Transmit
//
///////////////////////////////

Transmit::Transmit() : Frame() {
	// init variables
	tx_attempts = 0;
}

// print the frame content
void Frame::print_frame() {
	Serial.print("     frame = ");
	Serial.println(frame, HEX);
}  

// Generate data frame
void Transmit::frame_generation(){	// Fails if not all field variables are present.
	//  Serial.println(frame_payload,HEX);
	frame = frame_from;
	//  Serial.println(frame,HEX);
	frame <<= LEN_FRAME_ADDR;
	//  Serial.println(frame,HEX);
	frame |= frame_to;
	//  Serial.println(frame,HEX);
	frame <<= LEN_FRAME_TYPE;
	//  Serial.println(frame,HEX);
	frame |= frame_type;
	//  Serial.println(frame,HEX);
	frame <<= LEN_FRAME_SEQNUM;
	//  Serial.println(frame,HEX);
	frame |= frame_seqnum;
	//  Serial.println(frame,HEX);
	frame <<= LEN_FRAME_PAYLOAD;
	//  Serial.println(frame,HEX);
	frame |= frame_payload;
	//  Serial.println(frame,HEX);
	frame <<= LEN_FRAME_CRC;
	frame |= frame_crc;
	//  Serial.println(frame,HEX);
}

void Transmit::add_crc(int crc) {
	frame &= 0xffffff00;
	frame |= crc & 0xff;
}

////////////////////////////////
//
// Receive
//
///////////////////////////////

Receive::Receive() : Frame() {
	// constructor, do nothing
}

// Decompose frame - Frame format: [from | 4][to | 4][type | 4][seqnum | 4][payload | 8][CRC | 8]
void Receive::frame_decompose(){
	unsigned long frbuff; 
	frbuff = frame;
	frame_crc = frbuff & 0x000000ff;
	frbuff >>= LEN_FRAME_CRC;
	frame_payload = frbuff & 0x000000ff;
	frbuff >>= LEN_FRAME_PAYLOAD;
	frame_seqnum = frbuff & 0x0000000f;
	frbuff >>= LEN_FRAME_SEQNUM;
	frame_type = frbuff & 0x0000000f;
	frbuff >>= LEN_FRAME_TYPE;
	frame_to = frbuff & 0x0000000f;
	frbuff >>= LEN_FRAME_ADDR;
	frame_from = frbuff & 0x0000000f;
	Serial.println("[Decomp] Received message:");
	Serial.print("\t From: ");
	Serial.println(frame_from);
	Serial.print("\t To: ");
	Serial.println(frame_to);
	Serial.print("\t Type: ");
	switch(frame_type){
		case FRAME_TYPE_ACK:
			Serial.print(frame_type);
			Serial.println(" = ACK");
			break;
		case FRAME_TYPE_DATA:
			Serial.print(frame_type);
			Serial.println(" = DATA");
			break;
		default:
			Serial.print(frame_type);
			Serial.println("  UNKNOWN");
			break;
	}
	Serial.print("\t Seq no: ");
	Serial.println(frame_seqnum);
	Serial.print("\t Payload: ");
	Serial.println(frame_payload);
	Serial.print("\t CRC: ");
	Serial.println(frame_crc,BIN);
}

