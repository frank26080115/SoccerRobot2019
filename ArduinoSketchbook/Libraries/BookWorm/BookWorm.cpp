#include <Arduino.h>
#include "BookWorm.h"
#include "ContinuousServo.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "checksum.h"
#include <string.h>

cBookWorm BookWorm; // user accessible instance declared here

ContinuousServo servoLeft;  // not directly user accessible but accessible through library
ContinuousServo servoRight; // not directly user accessible but accessible through library
Servo servoWeap; // not directly user accessible but accessible through library

cBookWorm::cBookWorm(void)
{
	// resets all nvm to default
	memset(&(this->nvm), 0, sizeof(bookworm_nvm_t));

	this->defaultValues();
}

/*
Initializes all pins to default states, sets up the continuous rotation servos, enables serial port

return:	none
parameters:	none
*/
void cBookWorm::begin(void)
{
	Serial.begin(9600); // 9600 is slower but it's one less thing to screw up in the IDE

	EEPROM.begin(512);
	if (this->loadNvm() == false)
	{
		// failed to load, so create default SSID
		this->generateSsid(this->SSID);
		strcpy(this->nvm.ssid, this->SSID);
	}

	if ((this->nvm.servoFlip & 0x04) == 0) {
		servoLeft.attach(pinServoLeft);
		servoRight.attach(pinServoRight);
	}
	else {
		servoLeft.attach(pinServoLeft);
		servoRight.attach(pinServoRight);
	}
	move(0, 0);
	positionWeapon(0);
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

	#ifdef BOOKWORM_DEBUG
	this->printf("NVM reading: ");
	#endif
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		ptr[i] = EEPROM.read(i);
		#ifdef BOOKWORM_DEBUG
		this->printf(" %02X", ptr[i]);
		#endif
	}
	#ifdef BOOKWORM_DEBUG
	this->printf(" done!\r\n");
	#endif

	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	if (chksum == this->nvm.checksum)
	{
		#ifdef BOOKWORM_DEBUG
		this->printf("NVM loaded successfully, checksum %04X\r\n", chksum);
		#endif
		memcpy(this->SSID, this->nvm.ssid, 32);
		this->SSID[31] = 0;
		return true;
	}
	else
	{
		#ifdef BOOKWORM_DEBUG
		this->printf("NVM load failed due to mismatched checksum %04X != %04X\r\n", chksum, this->nvm.checksum);
		#endif
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
	#ifdef BOOKWORM_DEBUG
	this->printf("EEPROM writing: ");
	#endif
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		EEPROM.write(i, ptr[i]);
		#ifdef BOOKWORM_DEBUG
		this->printf(" %02X", ptr[i]);
		#endif
	}
	EEPROM.commit();
	#ifdef BOOKWORM_DEBUG
	this->printf(" done!\r\n");
	#endif
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
		if (((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z') || (d >= '0' && d <= '9')) == false) {
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
	#ifdef BOOKWORM_DEBUG
	this->printf("set SSID: %s\r\n", this->SSID);
	#endif
	this->nvm.ssid[31] = 0;
	this->SSID[31] = 0;
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
	this->nvm.advanced = false;
	this->nvm.servoMax = 500;
	this->nvm.servoDeadzoneLeft = 0;
	this->nvm.servoDeadzoneRight = 0;
	this->nvm.servoBiasLeft = 0;
	this->nvm.servoBiasRight = 0;
	this->nvm.speedGain = 1000;
	this->nvm.steeringSensitivity = 500;
	this->nvm.steeringBalance = 1000;
	this->nvm.servoFlip = 0;
	this->nvm.servoStoppedNoPulse = true;
	this->nvm.stickRadius = 100;
	this->nvm.weapPosSafe = 1000;
	this->nvm.weapPosA = 1500;
	this->nvm.weapPosB = 2000;
	this->nvm.leftHanded = false;
	this->printf("values set to defaults\r\n");
}

void cBookWorm::factoryReset() {
	defaultValues();
	this->generateSsid(this->SSID);
	this->setSsid(this->SSID);
	this->printf("factory reset performed\r\n");
}