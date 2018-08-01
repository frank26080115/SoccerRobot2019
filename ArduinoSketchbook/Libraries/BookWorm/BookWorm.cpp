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
	if (chksum == this->nvm.checksum && BOOKWORM_EEPROM_VERSION == this->nvm.eeprom_version)
	{
		memcpy(this->SSID, this->nvm.ssid, 32);
		this->SSID[BOOKWORM_SSID_SIZE] = 0;

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
		this->debugf("\tversion %04X != %04X\r\n", BOOKWORM_EEPROM_VERSION, this->nvm.eeprom_version);
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
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	int i;
	uint16_t chksum;
	this->nvm.eeprom_version = BOOKWORM_EEPROM_VERSION;
	this->nvm.ssid[BOOKWORM_SSID_SIZE] = 0;
	this->SSID[BOOKWORM_SSID_SIZE] = 0;
	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	this->nvm.checksum = chksum;
	this->debugf("EEPROM writing: ");
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		EEPROM.write(i, ptr[i]);
		this->debugf(" %02X", ptr[i]);
	}
	EEPROM.commit();
	this->debugf(" done!\r\n");
}

/*
Set the SSID, saves into NVM
Requires reboot to take effect
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
Sets all NVM items to default values
Does not generate SSID

return: none
parameters: none
*/
void cBookWorm::defaultValues()
{
	this->nvm.eeprom_version = BOOKWORM_EEPROM_VERSION;
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