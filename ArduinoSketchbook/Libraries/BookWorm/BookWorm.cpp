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

/*
Initializes all pins to default states, sets up the continuous rotation servos, enables serial port

return:	none
parameters:	none
*/
void cBookWorm::begin(void)
{
	//Serial.begin(9600); // 9600 is slower but it's one less thing to screw up in the IDE

	servoLeft.attach(pinServoLeft);
	servoRight.attach(pinServoRight);
	move(0, 0);
	//enableAccelLimit();

	EEPROM.begin(64);
	if (this->loadNvm() == false)
	{
		// failed to load, so create default SSID
		uint8_t macbuf[6];
		int i;
		WiFi.macAddress(macbuf);
		macbuf[0] += macbuf[3];
		macbuf[1] += macbuf[4];
		macbuf[2] += macbuf[5];
		i  = sprintf(this->SSID, "robo-");
		i += sprintf(&(this->SSID[i]), "%02x", macbuf[0]);
		i += sprintf(&(this->SSID[i]), "%02x", macbuf[1]);
		i += sprintf(&(this->SSID[i]), "%02x", macbuf[2]);
	}
}

/*
Moves both left and right continuous rotation servos at a given speed
Direction for the servos are corrected for you

return: none
parameters:
				left: left side servo speed, value between -500 and +500, 0 meaning stopped, +500 means full speed forward, -500 means full speed reverse
				right: right side servo speed, value between -500 and +500, 0 meaning stopped, +500 means full speed forward, -500 means full speed reverse
*/
// void cBookWorm::move(signed int left, signed int right);
/* this function actually is written inside BookWormMove.cpp but I've put the documentation here for quicker access */

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

	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		ptr[i] = EEPROM.read(i);
	}

	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	if (chksum == this->nvm.checksum)
	{
		memcpy(this->SSID, this->nvm.ssid, 32);
		this->SSID[31] = 0;
		this->setServoDeadzoneLeft(this->nvm.servoDeadzoneLeft);
		this->setServoDeadzoneLeft(this->nvm.servoDeadzoneRight);
		this->setServoBiasLeft(this->nvm.servoBiasLeft);
		this->setServoBiasRight(this->nvm.servoBiasRight);
		return true;
	}
	else
	{
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
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		EEPROM.write(i, ptr[i]);
	}
	EEPROM.commit();
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
	}
	this->nvm.ssid[31] = 0;
	this->SSID[31] = 0;
	this->saveNvm();
}