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
	#if (!defined(ENABLE_WEAPON)) || (pinServoWeapon != pinServoLeft && pinServoWeapon != pinServoRight)
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

void cBookWorm::setLedOn()
{
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
	pinMode(pinLed, OUTPUT);
	digitalWrite(pinLed, pinLedOnState);
	#endif
}

void cBookWorm::setLedOff()
{
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
	digitalWrite(pinLed, pinLedOffState);
	#endif
}

/*
Generate a string suitable for usage as a SSID
Uses the MAC address to be a bit unique

returns: the buffer that was given
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
		this->SSID[31] = 0;

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
	this->nvm.ssid[31] = 0;
	this->SSID[31] = 0;
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
	for (i = 0; i < 31; i++) {
		char d = str[i];
		if (((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z') || (d >= '0' && d <= '9') || d == 0) == false) {
			d = '-';
		}
		this->nvm.ssid[i] = d;
		this->SSID[i] = d;
		if (d == 0) {
			break;
		}
	}
	if (strlen(this->SSID) <= 0) {
		this->generateSsid(this->SSID);
		strcpy(this->nvm.ssid, this->SSID);
	}
	this->nvm.ssid[31] = 0;
	this->SSID[31] = 0;
	this->debugf("set SSID: %s\r\n", this->SSID);
}

void cBookWorm::setLeftHanded(bool x)
{
	this->nvm.leftHanded = x;
}

void cBookWorm::setAdvanced(bool x)
{
	this->nvm.advanced = x;
}

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
	this->nvm.checksum = 0xABCD;
	loadPinAssignments();
	this->debugf("values set to defaults\r\n");
}

void cBookWorm::factoryReset() {
	defaultValues();
	this->generateSsid(this->SSID);
	this->setSsid(this->SSID);
	this->printf("factory reset performed\r\n");
}

void cBookWorm::delayWhileFeeding(int x)
{
	int i;
	for (i = 0; i < x; i++)
	{
		delay(1);
		ESP.wdtFeed();
	}
}