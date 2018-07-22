#include <Arduino.h>
#include "BookWorm.h"
#include "ContinuousServo.h"

#ifdef ENABLE_WEAPON

extern Servo servoWeap;

void cBookWorm::spinWeapon(uint16_t x)
{
	if (x > 0)
	{
		if (x < 500) {
			x = 500;
		}
		if (x > 2500) {
			x = 2500;
		}
		if (servoWeap.attached() == false) {
			servoWeap.attach(pinnumServoWeapon);
		}
		servoWeap.writeTicks(x);
	}
	else // x == 0
	{
		servoWeap.detach();
	}
}

void cBookWorm::positionWeapon(uint8_t pos)
{
	if (pos == 1)
	{
		this->spinWeapon(this->nvm.weapPosA);
	}
	else if (pos == 2)
	{
		this->spinWeapon(this->nvm.weapPosB);
	}
	else
	{
		this->spinWeapon(this->nvm.weapPosSafe);
	}
}

void cBookWorm::setWeapPosSafe(uint16_t x)
{
	this->nvm.weapPosSafe = x;
}

void cBookWorm::setWeapPosA(uint16_t x)
{
	this->nvm.weapPosA = x;
}

void cBookWorm::setWeapPosB(uint16_t x)
{
	this->nvm.weapPosB = x;
}

void cBookWorm::setEnableWeapon(bool x)
{
	this->nvm.enableWeapon = x;
}

#endif