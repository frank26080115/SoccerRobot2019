#ifndef _BOOKWORM_H_
#define _BOOKWORM_H_

#include <Arduino.h>
#include "ContinuousServo.h"
#include "WString.h"

// pin definitions that can be utilized by other libraries
#define pinServoLeft         9
#define pinServoRight        7
#define pinServoWeap         1

typedef struct
{
	char ssid[32];
	uint16_t servoMax;
	uint16_t servoDeadzoneLeft;
	uint16_t servoDeadzoneRight;
	int16_t servoBiasLeft;
	int16_t servoBiasRight;
	uint16_t speedGain;
	int16_t steeringSensitivity;
	int16_t steeringBalance;
	uint8_t servoFlip;
	bool servoStoppedNoPulse;
	uint16_t stickRadius;
	uint16_t weapPosSafe;
	uint16_t weapPosA;
	uint16_t weapPosB;
	bool leftHanded;
	bool advanced;
	uint16_t checksum;
}
bookworm_nvm_t;

class cBookWorm
{
public:
	cBookWorm();
	void begin(void);

	void move(signed int left, signed int right);
	void moveLeftServo(signed int left);
	void moveRightServo(signed int right);
	void moveMixed(signed int throttle, signed int steer);

	// these functions below require more understanding of how servo signals work
	void calcMix(int32_t throttle, int32_t steer, int32_t * left, int32_t * right);
	void setSpeedGain(unsigned int x);
	void setServoMax(unsigned int x);
	void setServoDeadzoneLeft(unsigned int x);
	void setServoDeadzoneRight(unsigned int x);
	void setServoBiasLeft(signed int x);
	void setServoBiasRight(signed int x);
	void setSteeringSensitivity(signed int x);
	void setSteeringBalance(signed int x);
	void setServoStoppedNoPulse(bool);
	void setServoFlip(uint8_t);
	void setStickRadius(uint16_t);
	void setLeftHanded(bool);

	void setRobotFlip(bool);
	bool getRobotFlip();
	void togRobotFlip();

	void spinWeapon(uint16_t);
	void positionWeapon(uint8_t);
	void setWeapPosSafe(uint16_t);
	void setWeapPosA(uint16_t);
	void setWeapPosB(uint16_t);
	void setAdvanced(bool);

	// these are for making it easier to debug
	int printf(const char *format, ...);

	char SSID[32];

	void defaultValues();
	void factoryReset();
	bool loadNvm();
	void saveNvm();
	char* generateSsid(char*);
	void setSsid(char*);
	bookworm_nvm_t nvm;
	bool robotFlip;
};

extern cBookWorm BookWorm; // declare user accessible instance

#endif
