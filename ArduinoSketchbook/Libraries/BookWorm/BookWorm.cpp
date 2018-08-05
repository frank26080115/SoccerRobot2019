#include <Arduino.h>
#include "BookWorm.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "checksum.h"
#include <string.h>

#ifdef USE_CUSTOM_SERVO_LIB
#include "ContinuousServo.h"
#else
#include <Servo.h>
#endif

cBookWorm BookWorm; // user accessible instance declared here

ContinuousServo servoLeft;  // not directly user accessible but accessible through library
ContinuousServo servoRight; // not directly user accessible but accessible through library
#ifdef ENABLE_WEAPON
Servo servoWeap; // not directly user accessible but accessible through library
#endif

cBookWorm::cBookWorm(void)
{
	memset(&(this->nvm), 0, sizeof(bookworm_nvm_t));
	this->serialHasBegun = false;
	this->pinsHaveLoaded = false;
}

/*
Initializes all pins to default states, sets up the continuous rotation servos, enables serial port

return:	none
parameters:	none
*/
void cBookWorm::begin(void)
{
	#if (!defined(ENABLE_WEAPON)) || (pinServoWeapon != pinServoLeft && pinServoWeapon != pinServoRight) || defined(ALL_SAFE_DEBUG_MODE)
	Serial.begin(BOOKWORM_BAUD);
	this->serialHasBegun = true;
	#else
	system_set_os_print(0);
	#endif

	EEPROM.begin(512);
	if (this->loadNvm() == false)
	{
		// failed to load
		this->defaultValues();
		this->generateSsid(this->SSID);
		this->setSsid(this->SSID);
	}

	#ifdef ENABLE_SIGNALCROSS_RESET
	if (checkStartMode() != false)
	{
		printf("Signal Pins Factory Reset");
		this->defaultValues();
		this->generateSsid(this->SSID);
		this->setSsid(this->SSID);
	}
	#endif
}

/*
Turn on the LED

return:	none
parameters:	none
*/
void cBookWorm::setLedOn()
{
	#if defined(HWBOARD_ESP12) || defined(HWBOARD_ESP12_NANO)
	pinMode(pinLed1, OUTPUT);
	pinMode(pinLed2, OUTPUT);
	digitalWrite(pinLed1, pinLed1OnState);
	digitalWrite(pinLed2, pinLed2OnState);
	#else
	#ifdef pinLed
		#ifdef ENABLE_WEAPON
			#if pinServoWeapon == pinLed
				return;
			#endif
			#ifdef pinServoLeftAlt
				#if pinServoLeftAlt == pinLed
					return;
				#endif
			#endif
			#ifdef pinServoRightAlt
				#if pinServoRightAlt == pinLed
					return;
				#endif
			#endif
		#endif
		#if pinServoLeft == pinLed || pinServoRight == pinLed
			return;
		#endif
		#if pinLed == 1 && defined(ALL_SAFE_DEBUG_MODE)
			return;
		#endif
	pinMode(pinLed, OUTPUT);
	digitalWrite(pinLed, pinLedOnState);
	#endif
	#endif
}

/*
Turn off the LED

return:	none
parameters:	none
*/
void cBookWorm::setLedOff()
{
	#if defined(HWBOARD_ESP12) || defined(HWBOARD_ESP12_NANO)
	digitalWrite(pinLed1, pinLed1OffState);
	digitalWrite(pinLed2, pinLed2OffState);
	#else
	#ifdef pinLed
		#ifdef ENABLE_WEAPON
			#if pinServoWeapon == pinLed
				return;
			#endif
			#ifdef pinServoLeftAlt
				#if pinServoLeftAlt == pinLed
					return;
				#endif
			#endif
			#ifdef pinServoRightAlt
				#if pinServoRightAlt == pinLed
					return;
				#endif
			#endif
		#endif
		#if pinServoLeft == pinLed || pinServoRight == pinLed
			return;
		#endif
		#if pinLed == 1 && defined(ALL_SAFE_DEBUG_MODE)
			return;
		#endif
	digitalWrite(pinLed, pinLedOffState);
	#endif
	#endif
}

/*
Turn on or off the LED

return:	none
parameters:	boolean, true for ON, false for OFF
*/
void cBookWorm::setLed(bool x)
{
	if (x) {
		setLedOn();
	}
	else {
		setLedOff();
	}
}

/*
Generate a string suitable for usage as a SSID
Uses the MAC address to be a bit unique

return: the buffer that was given
parameters: character buffer to be written into
*/
char* cBookWorm::generateSsid(char* buff)
{
	uint8_t macbuf[6];
	int i;
	WiFi.macAddress(macbuf);
	#ifdef ENABLE_SSID_MAC_MIX
	macbuf[3] += macbuf[0];
	macbuf[4] += macbuf[1];
	macbuf[5] += macbuf[2];
	#endif
	i  = sprintf(buff, "robo-");
	i += sprintf(&(buff[i]), "%02x", macbuf[3]);
	i += sprintf(&(buff[i]), "%02x", macbuf[4]);
	i += sprintf(&(buff[i]), "%02x", macbuf[5]);
	sprintf(this->nvm.password, BOOKWORM_DEFAULT_PASSWORD);
	return buff;
}

/*
Load saved information from NVM

return: successful or not
parameters: none
*/
bool cBookWorm::loadNvm()
{
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	int i;
	uint16_t chksum;

	this->debugf("NVM reading: ");
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		ptr[i] = EEPROM.read(i);
		this->debugf(" %02X", ptr[i]);
	}
	this->debugf(" done!\r\n");

	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	if (chksum == this->nvm.checksum && BOOKWORM_EEPROM_VERSION == this->nvm.eeprom_version1 && BOOKWORM_EEPROM_VERSION == this->nvm.eeprom_version2)
	{
		memcpy(this->SSID, this->nvm.ssid, 32);
		this->SSID[BOOKWORM_SSID_SIZE] = 0;

		setWifiChannel(this->nvm.wifiChannel); // this does a range check

		loadPinAssignments();

		#if defined(ENABLE_WEAPON) && (pinServoWeapon == pinServoLeft || pinServoWeapon == pinServoRight)
		if (this->nvm.enableWeapon == false) {
			Serial.begin(BOOKWORM_BAUD);
			this->serialHasBegun = true;
		}
		#endif

		this->debugf("NVM loaded successfully, checksum %04X\r\n", chksum);

		return true;
	}
	else
	{
		this->debugf("NVM load failed, \r\n\tchecksum %04X != %04X\r\n", chksum, this->nvm.checksum);
		if (BOOKWORM_EEPROM_VERSION != this->nvm.eeprom_version1) {
			this->debugf("\tversion %04X != %04X\r\n", BOOKWORM_EEPROM_VERSION, this->nvm.eeprom_version1);
		}
		if (BOOKWORM_EEPROM_VERSION != this->nvm.eeprom_version2) {
			this->debugf("\tversion %04X != %04X\r\n", BOOKWORM_EEPROM_VERSION, this->nvm.eeprom_version2);
		}
		return false;
	}
}

/*
Save information into NVM

return: none
parameters: none
*/
void cBookWorm::saveNvm()
{
	int i;
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	ensureNvmChecksums();
	this->debugf("EEPROM writing: ");
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		EEPROM.write(i, ptr[i]);
		this->debugf(" %02X", ptr[i]);
	}
	EEPROM.commit();
	this->debugf(" done!\r\n");
}

/*
Set the SSID
Does not save into NVM immediately
Does very basic input sanitation

return: none
parameters: string containing the SSID
*/
void cBookWorm::setSsid(char* str)
{
	int i;
	for (i = 0; i < BOOKWORM_SSID_SIZE; i++) { // for all chars
		char d = str[i];
		// input sanitation, non-alphanum chars are turned into hyphens
		if (((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z') || (d >= '0' && d <= '9') || d == 0) == false) {
			d = '-';
		}
		this->nvm.ssid[i] = d;
		this->SSID[i] = d;
		if (d == 0) { // stop at null terminator
			break;
		}
	}

	if (strlen(this->SSID) <= 0) { // why is the length zero? autogenerate the default SSID and try again
		this->generateSsid(this->SSID);
		strcpy(this->nvm.ssid, this->SSID);
	}

	// safety null-terminate
	this->nvm.ssid[BOOKWORM_SSID_SIZE] = 0;
	this->SSID[BOOKWORM_SSID_SIZE] = 0;

	this->debugf("set SSID: %s\r\n", this->SSID);
}

/*
Set the WiFi password
Does not save into NVM immediately
Does very basic input check, resets to 12345678 if errors are found

return: none
parameters: string containing the SSID
*/
void cBookWorm::setPassword(char* str)
{
	int i;
	bool failed = false;
	i = strlen(str);
	if (i < 8 || i > 31) {
		failed = true;
	}
	for (i = 0; i < BOOKWORM_PASSWORD_SIZE && failed == false; i++) { // for all chars
		char d = str[i];
		// input sanitation, non-alphanum chars are turned into hyphens
		if (d < 32 || d > 126) {
			failed = true;
		}
	}

	if (failed == false) {
		strcpy(this->nvm.password, str);
	}
	else {
		this->printf("password validation failed: %s\r\n", str);
		sprintf(this->nvm.password, BOOKWORM_DEFAULT_PASSWORD);
	}

	this->debugf("set password: %s\r\n", this->nvm.password);
}

/*
Sets if robot advanced features should be shown on the right side of the screen
(weapon control and inverted drive)

return: none
parameters: boolean, if advanced features should be shown on the right side of the screen
*/
void cBookWorm::setLeftHanded(bool x)
{
	this->nvm.leftHanded = x;
}

/*
Sets if robot advanced features should be enabled
(including weapon control and inverted drive)

return: none
parameters: boolean, if advanced features should be enabled
*/
void cBookWorm::setAdvanced(bool x)
{
	this->nvm.advanced = x;
}

/*
Sets desired WiFi channel to use (1 to 13)
Does a range limit

return: none
parameters: desired WiFi channel to use (1 to 13)
*/
void cBookWorm::setWifiChannel(uint8_t x)
{
	this->nvm.wifiChannel = x < 1 ? 1 : (x > 13 ? 13 : x);
}

/*
Sets all NVM items to default values
Does not generate SSID

return: none
parameters: none
*/
void cBookWorm::defaultValues()
{
	this->nvm.eeprom_version1 = BOOKWORM_EEPROM_VERSION;
	this->nvm.eeprom_version2 = BOOKWORM_EEPROM_VERSION;
	this->nvm.divider1 = 0;
	this->nvm.divider2 = 0;
	sprintf(this->nvm.password, BOOKWORM_DEFAULT_PASSWORD);
	this->nvm.wifiChannel = 1;
	this->nvm.advanced = true;
	this->nvm.servoMax = 500;
	this->nvm.servoDeadzoneLeft = 0;
	this->nvm.servoDeadzoneRight = 0;
	this->nvm.servoBiasLeft = 0;
	this->nvm.servoBiasRight = 0;
	this->nvm.speedGain = 1000;
	this->nvm.steeringSensitivity = 500;
	this->nvm.steeringBalance = 0;
	this->nvm.servoFlip = 0;
	this->nvm.servoStoppedNoPulse = true;
	this->nvm.stickRadius = 100;
	#ifdef ENABLE_WEAPON
	this->nvm.weapPosSafe = 1000;
	this->nvm.weapPosA = 1500;
	this->nvm.weapPosB = 2000;
	this->nvm.enableWeapon = true;
	#endif
	this->nvm.leftHanded = false;
	#ifdef ENABLE_BATTERY_MONITOR
	this->nvm.vdiv_r1 = 7500;
	this->nvm.vdiv_r2 = 0; // disable usage, 1000 in circuit
	this->nvm.vdiv_filter = 50;
	this->nvm.warning_voltage = 6000;
	#endif
	this->nvm.checksum = 0xABCD;
	loadPinAssignments();
	this->debugf("values set to defaults\r\n");
}

/*
Sets all NVM items to default values, and generates a SSID

return: none
parameters: none
*/
void cBookWorm::factoryReset() {
	defaultValues();
	this->generateSsid(this->SSID);
	this->setSsid(this->SSID);
	this->printf("factory reset performed\r\n");
}

/*
Delays a number of milliseconds, while feeding the watchdog timer
Insanely inaccurate

return: none
parameters: number of milliseconds to delay
*/
void cBookWorm::delayWhileFeeding(int x)
{
	int i;
	for (i = 0; i < (x - 10); i += 10)
	{
		delay(10);
		if ((i % 200) == 0) {
			ESP.wdtFeed();
		}
	}
	for (; i < x; i += 1)
	{
		delay(1);
		ESP.wdtFeed();
	}
}

#ifdef ENABLE_SIGNALCROSS_RESET
bool cBookWorm::checkStartMode(void)
{
	bool ret = true;
	#if defined(HWBOARD_ESP12_NANO) || defined(HWBOARD_ESP12)
	uint32_t t;
	pinMode(pinServoLeft, OUTPUT);
	pinMode(pinServoRight, INPUT_PULLUP);
	digitalWrite(pinServoLeft, LOW);
	t = micros();
	while ((micros() - t) < 500) {
		ESP.wdtFeed();
	}
	while ((micros() - t) < 1500) {
		ESP.wdtFeed();
		if (digitalRead(pinServoRight) != LOW) {
			pinMode(pinServoLeft, INPUT);
			pinMode(pinServoRight, INPUT);
			return false;
		}
	}
	pinMode(pinServoLeft, INPUT_PULLUP);
	pinMode(pinServoRight, OUTPUT);
	digitalWrite(pinServoRight, LOW);
	t = micros();
	while ((micros() - t) < 500) {
		ESP.wdtFeed();
	}
	while ((micros() - t) < 1500) {
		ESP.wdtFeed();
		if (digitalRead(pinServoLeft) != LOW) {
			pinMode(pinServoLeft, INPUT);
			pinMode(pinServoRight, INPUT);
			return false;
		}
	}
	pinMode(pinServoLeft, OUTPUT);
	pinMode(pinServoRight, INPUT);
	digitalWrite(pinServoLeft, HIGH);
	t = micros();
	while ((micros() - t) < 500) {
		ESP.wdtFeed();
	}
	while ((micros() - t) < 1500) {
		ESP.wdtFeed();
		if (digitalRead(pinServoRight) == LOW) {
			ret = false;
		}
	}
	digitalWrite(pinServoLeft, LOW);
	if (ret == false) {
		pinMode(pinServoLeft, INPUT);
		pinMode(pinServoRight, INPUT);
	}
	pinMode(pinServoLeft, INPUT);
	pinMode(pinServoRight, OUTPUT);
	digitalWrite(pinServoRight, HIGH);
	t = micros();
	while ((micros() - t) < 500) {
		ESP.wdtFeed();
	}
	while ((micros() - t) < 1500) {
		ESP.wdtFeed();
		if (digitalRead(pinServoLeft) != LOW) {
			ret = false;
		}
	}
	digitalWrite(pinServoRight, LOW);
	if (ret == false) {
		pinMode(pinServoLeft, INPUT);
		pinMode(pinServoRight, INPUT);
	}
	else {
		return true;
	}
	#else
	return false;
	#endif
	return ret;
}
#endif

/*
Loads a string of hexadecimal characters into NVM structure
Validates checksum
Automatically detects length to see if it contains WiFi info or not
Optional commit into NVM

returns:
	boolean, true on success
	errCode is a pointer to error Code
			1 means wrong length
			2 means illegal char detected
			3 means checksum failed

parameters:
	str, a string of hexadecimal to process
	save, automatically save (commit) to NVM if successful
*/
bool cBookWorm::loadNvmHex(char* str, uint8_t* errCode, bool save)
{
	uint8_t* tmpbuff = (uint8_t*)malloc(sizeof(bookworm_nvm_t));
	bookworm_nvm_t* castPtr = (bookworm_nvm_t*)tmpbuff;

	int slen = strlen(str);
	uint16_t chksum;
	uint32_t userLength = calcUserNvmLength(false);
	uint32_t userLength2 = calcUserNvmLength(true);
	int i, j, k, accum;
	for (i = 0, j = 0, k = 0; i < slen, j < sizeof(bookworm_nvm_t); i++)
	{
		if ((k % 2) == 0) {
			accum = 0;
		}
		else {
			accum <<= 8;
		}

		char c = str[i];
		if (c >= '0' && c <= '9') {
			accum += c - '0';
			k++;
		}
		else if (c >= 'a' && c <= 'f') {
			accum += c - 'a' + 0xA;
			k++;
		}
		else if (c >= 'A' && c <= 'F') {
			accum += c - 'A' + 0xA;
			k++;
		}
		else if ((c == ' ' || c == '\t' || c == '\n' || c == '\r') && (k % 2) == 0) {
			accum = 0;
			k = k;
		}
		else {
			if (errCode != NULL) { *errCode = 2; /* valid chars failed */ }
			free(tmpbuff); return false;
		}

		if ((k % 2) == 1) {
			tmpbuff[j] = accum;
			j++;
		}
	}
	if (j == sizeof(bookworm_nvm_t))
	{
		// wifi + user blocks
		chksum = bookworm_fletcher16(tmpbuff, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
		if (chksum != castPtr->checksum) {
			if (errCode != NULL) { *errCode = 3; /* checksum failed */ }
			free(tmpbuff); return false;
		}
		memcpy((void*)(&(this->nvm)), (const void*)tmpbuff, sizeof(bookworm_nvm_t));
		free(tmpbuff);
		if (save) {
			saveNvm();
		}
		return true;
	}
	else if (j == userLength2)
	{
		// user block only
		uint16_t* chksum2;
		chksum = bookworm_fletcher16(tmpbuff, userLength);
		chksum2 = (uint16_t*)(&tmpbuff[userLength + sizeof(uint8_t)]);
		if (chksum != (*chksum2)) {
			if (errCode != NULL) { *errCode = 3; /* checksum failed */ }
			free(tmpbuff); return false;
		}
		memcpy(&(this->nvm.divider1), tmpbuff, userLength);
		free(tmpbuff);
		if (save) {
			saveNvm();
		}
		return true;
	}
	else
	{
		if (errCode != NULL) { *errCode = 1; /* length check failed */ }
		free(tmpbuff); return false;
	}
}

/*
Calculates the size of the NVM block that does not contain WiFi data

return: size in bytes
parameter: boolean, whether or not to include the checksum in the size calculation
*/
uint32_t cBookWorm::calcUserNvmLength(bool withChecksum)
{
	uint8_t* ptrUser = (uint8_t*)&(this->nvm.divider1);
	uint8_t* ptrUserEnd = (uint8_t*)&(this->nvm.divider2);
	uint32_t userLength = (uint32_t)ptrUserEnd;
	userLength -= (uint32_t)ptrUser;
	if (withChecksum) {
		userLength += sizeof(uint8_t) + sizeof(uint16_t);
	}
	return userLength;
}

/*
Ensures that the checksum fields inside NVM cache is correct
Used before printing out HexBlob

return: none
parameter: none
*/
void cBookWorm::ensureNvmChecksums(void)
{
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	uint8_t* ptrUser = (uint8_t*)&(this->nvm.divider1);
	int i;
	uint16_t chksum, chksumUser;
	uint32_t userLength = calcUserNvmLength(false);
	this->nvm.eeprom_version1 = BOOKWORM_EEPROM_VERSION;
	this->nvm.eeprom_version2 = BOOKWORM_EEPROM_VERSION;
	this->nvm.ssid[BOOKWORM_SSID_SIZE] = 0;
	this->SSID[BOOKWORM_SSID_SIZE] = 0;
	this->nvm.password[BOOKWORM_PASSWORD_SIZE] = 0;
	chksumUser = bookworm_fletcher16(ptrUser, userLength);
	this->nvm.checksumUser = chksumUser;
	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	this->nvm.checksum = chksum;
}