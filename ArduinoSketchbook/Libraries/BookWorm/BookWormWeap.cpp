#include <Arduino.h>
#include "BookWorm.h"

#ifdef USE_CUSTOM_SERVO_LIB
#include "ContinuousServo.h"
#else
#include <Servo.h>
#endif

#ifdef ENABLE_WEAPON

extern Servo servoWeap;

void cBookWorm::spinWeapon(uint16_t x)
{
	if (this->nvm->enableWeapon == false) {
		servoWeap.detach();
		return;
	}
	if (x > 0)
	{
		if (x < 500) {
			x = 500;
		}
		if (x > 2500) {
			x = 2500;
		}
		if (servoWeap.attached() == false) {
			#ifndef ALL_SAFE_DEBUG_MODE
			servoWeap.attach(pinnumServoWeapon);
			#endif
		}
		#ifdef ENABLE_SERVO_DEBUG
		dbgWeaponTicks = x;
		#endif
		#ifndef ALL_SAFE_DEBUG_MODE
		servoWeap.write(x);
		#endif
	}
	else // x == 0
	{
		#ifndef ALL_SAFE_DEBUG_MODE
		servoWeap.detach();
		#endif
		#ifdef ENABLE_SERVO_DEBUG
		dbgWeaponTicks = 0;
		#endif
	}
}

void cBookWorm::positionWeapon(uint8_t pos)
{
	if (pos == 1)
	{
		this->spinWeapon(this->nvm->weapPosA);
	}
	else if (pos == 2)
	{
		this->spinWeapon(this->nvm->weapPosB);
	}
	else
	{
		this->spinWeapon(this->nvm->weapPosSafe);
	}
}

void cBookWorm::setWeapPosSafe(uint16_t x)
{
	this->nvm->weapPosSafe = x;
}

void cBookWorm::setWeapPosA(uint16_t x)
{
	this->nvm->weapPosA = x;
}

void cBookWorm::setWeapPosB(uint16_t x)
{
	this->nvm->weapPosB = x;
}

void cBookWorm::setEnableWeapon(bool x)
{
	this->nvm->enableWeapon = x;
}

#endif