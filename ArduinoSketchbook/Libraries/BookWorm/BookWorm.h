#ifndef _BOOKWORM_H_
#define _BOOKWORM_H_

#include <Arduino.h>
#include "ContinuousServo.h"
#include "WString.h"

// pin definitions that can be utilized by other libraries
#define pinServoLeft         9
#define pinServoRight        7

typedef struct
{
	char ssid[32];
	uint16_t servoDeadzoneLeft;
	uint16_t servoDeadzoneRight;
	int16_t servoBiasLeft;
	int16_t servoBiasRight;
	uint16_t checksum;
}
bookworm_nvm_t;

class cBookWorm
{
public:
	void begin(void);

	void move(signed int left, signed int right);
	void moveLeftServo(signed int left);
	void moveRightServo(signed int right);
	void moveMixed(signed int throttle, signed int steer);

	// these functions below require more understanding of how servo signals work
	void calcMix(signed int throttle, signed int steer, signed int * left, signed int * right);
	//void setAccelLimit(unsigned int accel);
	//void enableAccelLimit(void);
	//void disableAccelLimit(void);
	void setServoDeadzoneLeft(unsigned int x);
	void setServoDeadzoneRight(unsigned int x);
	void setServoBiasLeft(signed int x);
	void setServoBiasRight(signed int x);
	void setServoStoppedNoPulse(bool);

	// these are for making it easier to debug
	int printf(const char *format, ...);

	char SSID[32];

	bool loadNvm();
	void saveNvm();
	void setSsid(char*);
private:
	bookworm_nvm_t nvm;
};

extern cBookWorm BookWorm; // declare user accessible instance

#endif
