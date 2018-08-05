#ifndef _BOOKWORM_H_
#define _BOOKWORM_H_

#define BOOKWORM_VERSION 1
#define BOOKWORM_EEPROM_VERSION 1

#define ENABLE_WEAPON
#define BOOKWORM_DEBUG
#define BOOKWORM_BAUD 9600
#define BOOKWORM_SSID_SIZE 31
#define BOOKWORM_PASSWORD_SIZE 31
#define ALL_SAFE_DEBUG_MODE
#define ENABLE_BATTERY_MONITOR
#define ENABLE_SIGNALCROSS_RESET
#define BOOKWORM_DEFAULT_PASSWORD "12345678"

#include <Arduino.h>
#include "WString.h"

extern "C" {
  #include "user_interface.h"
}

#include "hwboards.h"

#define USE_CUSTOM_SERVO_LIB
#ifdef USE_CUSTOM_SERVO_LIB
#include "ContinuousServo.h"
#else
#define ContinuousServo Servo
#endif

#pragma pack(push, 1)
typedef struct
{
	uint32_t eeprom_version1;           // used in case the layout changes
	char ssid[BOOKWORM_SSID_SIZE + 1];
	char password[BOOKWORM_PASSWORD_SIZE + 1];
	uint8_t wifiChannel;
	uint8_t divider1;                   // used as a placeholder for pointer positioning
	uint32_t eeprom_version2;           // used in case the layout changes
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
	#ifdef ENABLE_BATTERY_MONITOR
	uint32_t vdiv_r1;
	uint32_t vdiv_r2;
	uint16_t vdiv_filter;
	uint16_t warning_voltage;
	#endif
	uint8_t  divider2;     // used as a placeholder for pointer positioning
	uint16_t checksumUser; // makes sure the contents after "divider1" isn't junk
	uint16_t checksum;     // makes sure the contents isn't junk
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

	#ifdef ENABLE_BATTERY_MONITOR
	double readBatteryVoltageRaw(void);
	uint16_t readBatteryVoltageFiltered(void);
	uint16_t readBatteryVoltageFilteredLast(void);
	void setVdivR1(uint32_t);
	void setVdivR2(uint32_t);
	void setVdivFilter(uint16_t);
	void setBatteryWarningVoltage(uint16_t);
	bool isBatteryLowWarning(void);
	uint16_t calcMaxBattVoltage(void);
	#endif

	void delayWhileFeeding(int);

	// these are for making it easier to debug
	int printf(const char *format, ...);
	int debugf(const char *format, ...);

	char SSID[32];

	void defaultValues();
	void factoryReset();
	bool loadNvm();
	void saveNvm();
	void ensureNvmChecksums();
	char* generateSsid(char*);
	void setSsid(char*);
	void setPassword(char*);
	void setWifiChannel(uint8_t);
	bool loadNvmHex(char* str, uint8_t* errCode, bool save);
	uint32_t calcUserNvmLength(bool withChecksum);
	bookworm_nvm_t nvm;
	bool robotFlip; // temporary flip, for inverted drive

	#ifdef ENABLE_SIGNALCROSS_RESET
	bool checkStartMode(void);
	#endif

private:
	void loadPinAssignments();
	int pinnumServoLeft;
	int pinnumServoRight;
	#ifdef ENABLE_WEAPON
	int pinnumServoWeapon;
	#endif
	bool serialHasBegun;
	bool pinsHaveLoaded;
	#ifdef ENABLE_BATTERY_MONITOR
	double batteryVoltageFiltered;
	double adcToVoltage(uint16_t);
	#endif
};

extern cBookWorm BookWorm; // declare user accessible instance

#endif
