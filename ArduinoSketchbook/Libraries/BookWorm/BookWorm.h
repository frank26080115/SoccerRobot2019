#ifndef _BOOKWORM_H_
#define _BOOKWORM_H_

#define BOOKWORM_VERSION 1
#define BOOKWORM_EEPROM_VERSION 1

#include <Arduino.h>
#include "WString.h"

extern "C" {
  #include "user_interface.h"
}

// pin definitions that can be utilized by other libraries
#define pinServoLeft         2
#define pinServoRight        0

// There's not enough pins on ESP-01 for a weapon
// Only enable this if you can find a pin for it
//#define ENABLE_WEAPON

#ifdef ENABLE_WEAPON
#define pinServoWeapon       2
#define pinServoLeftAlt      1
#endif

#define BOOKWORM_DEBUG
#define BOOKWORM_BAUD 9600

#define USE_CUSTOM_SERVO_LIB
#ifdef USE_CUSTOM_SERVO_LIB
#include "ContinuousServo.h"
#else
#define ContinuousServo Servo
#endif

#pragma pack(push, 1)
typedef struct
{
	uint32_t eeprom_version;
	char ssid[32];
	uint16_t servoMax;
	uint16_t servoDeadzoneLeft;
	uint16_t servoDeadzoneRight;
	int16_t servoBiasLeft;
	int16_t servoBiasRight;
	uint16_t speedGain;
	int16_t steeringSensitivity;
	int16_t steeringBalance;
	uint8_t servoFlip; // bit flags, bit 0 right flip, bit 1 left flip, bit 2 swaps left and right
	bool servoStoppedNoPulse;
	uint16_t stickRadius; // size of the joystick shown on screen
	#ifdef ENABLE_WEAPON
	bool enableWeapon;
	uint16_t weapPosSafe;
	uint16_t weapPosA;
	uint16_t weapPosB;
	#endif
	bool leftHanded;
	bool advanced; // this option enables/disables weapon controls and inverted controls
	uint16_t checksum;
}
bookworm_nvm_t;
#pragma pack(pop)

class cBookWorm
{
public:
	cBookWorm();
	void begin(void);

	void setLedOn(void);
	void setLedOff(void);

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

	#ifdef ENABLE_WEAPON
	void spinWeapon(uint16_t);
	void positionWeapon(uint8_t);
	void setWeapPosSafe(uint16_t);
	void setWeapPosA(uint16_t);
	void setWeapPosB(uint16_t);
	void setEnableWeapon(bool);
	#endif
	void setAdvanced(bool);

	// these are for making it easier to debug
	int printf(const char *format, ...);
	int debugf(const char *format, ...);

	char SSID[32];

	void defaultValues();
	void factoryReset();
	bool loadNvm();
	void saveNvm();
	char* generateSsid(char*);
	void setSsid(char*);
	bookworm_nvm_t nvm;
	bool robotFlip;
private:
	void loadPinAssignments();
	int pinnumServoLeft;
	int pinnumServoRight;
	#ifdef ENABLE_WEAPON
	int pinnumServoWeapon;
	#endif
	bool serialHasBegun;
};

extern cBookWorm BookWorm; // declare user accessible instance

#endif
