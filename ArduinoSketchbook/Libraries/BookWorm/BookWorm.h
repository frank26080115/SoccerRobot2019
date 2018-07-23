#ifndef _BOOKWORM_H_
#define _BOOKWORM_H_

#define BOOKWORM_VERSION 1
#define BOOKWORM_EEPROM_VERSION 1

#define ENABLE_WEAPON
#define BOOKWORM_DEBUG
#define BOOKWORM_BAUD 9600
#define BOOKWORM_SSID_SIZE 31

#include <Arduino.h>
#include "WString.h"

extern "C" {
  #include "user_interface.h"
}

// pin definitions that can be utilized by other libraries
#define pinServoLeft         2
#define pinServoRight        0

#ifdef ENABLE_WEAPON
#define pinServoWeapon       2
#define pinServoLeftAlt      1
#endif

#define pinLed LED_BUILTIN // WARNING: ESP-01, usage of LED will disable serial port
#define pinLedOnState LOW
#define pinLedOffState HIGH

#define USE_CUSTOM_SERVO_LIB
#ifdef USE_CUSTOM_SERVO_LIB
#include "ContinuousServo.h"
#else
#define ContinuousServo Servo
#endif

#pragma pack(push, 1)
typedef struct
{
	uint32_t eeprom_version;            // used in case the layout changes
	char ssid[BOOKWORM_SSID_SIZE + 1];
	uint16_t servoMax;                  // maximum servo range, 500 means 1000 to 2000 microseconds
	uint16_t servoDeadzoneLeft;         // overcomes deadzones in motor driver or continuous servos
	uint16_t servoDeadzoneRight;        // overcomes deadzones in motor driver or continuous servos
	int16_t servoBiasLeft;              // offset for calibration
	int16_t servoBiasRight;             // offset for calibration
	int16_t speedGain;                  // makes it go faster or slower, out of 1000
	int16_t steeringSensitivity;        // makes it turn slower or faster, out of 1000
	int16_t steeringBalance;            // correction for not driving straight, out of 1000, positive adjusts left side, nagative adjusts right side 
	uint8_t servoFlip;                  // bit flags, bit 0 right flip, bit 1 left flip, bit 2 swaps left and right
	bool servoStoppedNoPulse;           // used for continuous servos, no pulses to stop the servos instead of a 1500us pulse
	uint16_t stickRadius;               // size of the joystick shown on screen
	#ifdef ENABLE_WEAPON
	bool enableWeapon;
	uint16_t weapPosSafe;
	uint16_t weapPosA;
	uint16_t weapPosB;
	#endif
	bool leftHanded;   // makes the advanced controls show up on right side of the screen
	bool advanced;     // this option enables/disables weapon controls and inverted controls
	uint16_t checksum; // makes sure the contents isn't junk
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
	void setLed(bool);

	void move(signed int left, signed int right);
	void moveLeftServo(signed int left);
	void moveRightServo(signed int right);
	void moveMixed(signed int throttle, signed int steer);

	// these functions below require more understanding of how servo signals work
	void calcMix(int32_t throttle, int32_t steer, int32_t * left, int32_t * right);
	void setSpeedGain(signed int x);
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

	// temporary flip, for inverted drive
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

	void delayWhileFeeding(int);

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
	bool robotFlip; // temporary flip, for inverted drive
private:
	void loadPinAssignments();
	int pinnumServoLeft;
	int pinnumServoRight;
	#ifdef ENABLE_WEAPON
	int pinnumServoWeapon;
	#endif
	bool serialHasBegun;
	bool pinsHaveLoaded;
};

extern cBookWorm BookWorm; // declare user accessible instance

#endif
