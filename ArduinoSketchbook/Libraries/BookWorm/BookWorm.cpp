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
	this->nvm = (bookworm_nvm_t*)malloc(sizeof(bookworm_nvm_t));
	memset((void*)this->nvm, 0, sizeof(bookworm_nvm_t));
	this->serialHasBegun = false;
	this->pinsHaveLoaded = false;
	#ifdef ENABLE_BATTERY_MONITOR
	this->vdivGcd = -1;
	#endif
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

	#ifdef BOOKWORM_DEBUG
	checkHardwareConfig(NULL, true);
	#endif

	EEPROM.begin(1024);
	if (this->loadNvm() == false)
	{
		// failed to load
		this->defaultValues();
		this->generateSsid(this->SSID);
		this->setSsid(this->SSID);
	}

	#ifdef ENABLE_SIGNALCROSS_RESET
	if (checkShouldReset() != false)
	{
		printf("Signal Pins Factory Reset\r\n");
		this->defaultValues();
		this->generateSsid(this->SSID);
		this->setSsid(this->SSID);
	}
	#endif
	debugf("BookWorm Library Begun\r\n");
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
	sprintf(this->nvm->password, BOOKWORM_DEFAULT_PASSWORD);
	strcpy(this->wifiPassword, this->nvm->password);
	return buff;
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
		this->nvm->ssid[i] = d;
		this->SSID[i] = d;
		if (d == 0) { // stop at null terminator
			break;
		}
	}

	if (strlen(this->SSID) <= 0) { // why is the length zero? autogenerate the default SSID and try again
		this->generateSsid(this->SSID);
		strcpy(this->nvm->ssid, this->SSID);
	}

	// safety null-terminate
	this->nvm->ssid[BOOKWORM_SSID_SIZE] = 0;
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
		if (d == 0) {
			break;
		}
		// input sanitation, non-alphanum chars are turned into hyphens
		if (d < 32 || d > 126) {
			failed = true;
		}
	}

	if (failed == false) {
		strcpy(this->nvm->password, str);
		strcpy(this->wifiPassword, str);
	}
	else {
		this->printf("password validation failed: %s\r\n", str);
		sprintf(this->nvm->password, BOOKWORM_DEFAULT_PASSWORD);
		strcpy(this->wifiPassword, this->nvm->password);
	}

	this->debugf("set password: %s\r\n", this->nvm->password);
}

/*
Sets if robot advanced features should be shown on the right side of the screen
(weapon control and inverted drive)

return: none
parameters: boolean, if advanced features should be shown on the right side of the screen
*/
void cBookWorm::setLeftHanded(bool x)
{
	this->nvm->leftHanded = x;
}

/*
Sets if robot advanced features should be enabled
(including weapon control and inverted drive)

return: none
parameters: boolean, if advanced features should be enabled
*/
void cBookWorm::setAdvanced(bool x)
{
	this->nvm->advanced = x;
}

/*
Sets desired WiFi channel to use (1 to 13)
Does a range limit

return: none
parameters: desired WiFi channel to use (1 to 13)
*/
void cBookWorm::setWifiChannel(uint8_t x)
{
	this->nvm->wifiChannel = x < 1 ? 1 : (x > 13 ? 13 : x);
}

/*
Sets all NVM items to default values
Does not generate SSID

return: none
parameters: none
*/
void cBookWorm::defaultValues()
{
	this->nvm->eeprom_version1 = BOOKWORM_EEPROM_VERSION;
	this->nvm->eeprom_version2 = BOOKWORM_EEPROM_VERSION;
	this->nvm->divider1 = 0;
	this->nvm->divider2 = 0;
	sprintf(this->nvm->password, BOOKWORM_DEFAULT_PASSWORD);
	memcpy(this->wifiPassword, this->nvm->password, BOOKWORM_PASSWORD_SIZE + 1);
	this->nvm->wifiChannel = 8;
	this->nvm->advanced = true;
	this->nvm->servoMax = 500;
	this->nvm->servoDeadzoneLeft = 0;
	this->nvm->servoDeadzoneRight = 0;
	this->nvm->servoBiasLeft = 0;
	this->nvm->servoBiasRight = 0;
	this->nvm->speedGain = 1000;
	this->nvm->steeringSensitivity = 500;
	this->nvm->steeringBalance = 0;
	this->nvm->servoFlip = 0;
	this->nvm->servoStoppedNoPulse = true;
	this->nvm->stickRadius = 100;
	#ifdef ENABLE_WEAPON
	this->nvm->weapPosSafe = 1000;
	this->nvm->weapPosA = 1500;
	this->nvm->weapPosB = 2000;
	this->nvm->enableWeapon = true;
	#endif
	this->nvm->leftHanded = false;
	#ifdef ENABLE_BATTERY_MONITOR
	this->nvm->vdiv_r1 = VDIV_R1_DEFAULT;
	this->nvm->vdiv_r2 = 0; // disable usage, 1000 in circuit
	this->nvm->vdiv_filter = 0;
	this->nvm->warning_voltage = 6000;
	#endif
	this->nvm->checksum = 0xABCD;
	loadPinAssignments();
	this->debugf("values set to defaults\r\n");
}

#ifdef ENABLE_SIGNALCROSS_RESET
bool cBookWorm::checkShouldReset(void)
{
	bool ret = true;
	#if defined(HWBOARD_ESP12_NANO) || defined(HWBOARD_ESP12)
	uint32_t t;
	//debugf("sL low\r\n");
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
	//debugf("sR low\r\n");
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
	//debugf("sL high\r\n");
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
		return false;
	}
	//debugf("sR high\r\n");
	pinMode(pinServoLeft, INPUT);
	pinMode(pinServoRight, OUTPUT);
	digitalWrite(pinServoRight, HIGH);
	t = micros();
	while ((micros() - t) < 500) {
		ESP.wdtFeed();
	}
	while ((micros() - t) < 1500) {
		ESP.wdtFeed();
		if (digitalRead(pinServoLeft) == LOW) {
			ret = false;
		}
	}
	digitalWrite(pinServoRight, LOW);
	if (ret == false) {
		pinMode(pinServoLeft, INPUT);
		pinMode(pinServoRight, INPUT);
		return false;
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
