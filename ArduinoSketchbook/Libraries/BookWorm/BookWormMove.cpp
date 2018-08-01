#include <Arduino.h>
#include "BookWorm.h"

#ifdef USE_CUSTOM_SERVO_LIB
#include "ContinuousServo.h"
#else
#include <Servo.h>
#endif

extern ContinuousServo servoLeft;
extern ContinuousServo servoRight;

#ifndef usToTicks
#define usToTicks(_us)    (_us)     // converts microseconds to tick (assumes prescale of 8)  // 12 Aug 2009
#endif
#ifndef ticksToUs
#define ticksToUs(_ticks) (_ticks)  // converts from ticks back to microseconds
#endif
#ifndef TRIM_DURATION
#define TRIM_DURATION     0         // compensation ticks to trim adjust for digitalWrite delays // 12 August 2009
#endif

#define SERVO_CENTER_TICKS    (usToTicks(1500 - TRIM_DURATION))

#define SERVO_ACCELERATION_LIMIT 2

void cBookWorm::move(signed int left, signed int right)
{
	if ((this->nvm.servoFlip & 0x01) != 0)
	{
		right *= -1;
	}
	if ((this->nvm.servoFlip & 0x02) != 0)
	{
		left *= -1;
	}

	if (this->robotFlip == false) {
		moveLeftServo(left);
		moveRightServo(right);
	}
	else { // robot is upside down, inverted drive
		moveLeftServo(-right);
		moveRightServo(-left);
	}
}

void cBookWorm::moveLeftServo(signed int x)
{
	int32_t ticks = SERVO_CENTER_TICKS;

	if (x > 0) {
		ticks += x;
		ticks += this->nvm.servoDeadzoneLeft;
		#ifdef USE_CUSTOM_SERVO_LIB
		ContinuousServo_BothForward |= (1 << 1);
		#endif
	}
	else if (x <= 0) {
		ticks += x;
		ticks -= this->nvm.servoDeadzoneLeft;
		#ifdef USE_CUSTOM_SERVO_LIB
		ContinuousServo_BothForward &= ~(1 << 1);
		#endif
	}
	if (x == 0 && this->nvm.servoStoppedNoPulse != false)
	{
		servoLeft.detach();
	}
	else if (x != 0 && servoLeft.attached() == false)
	{
		#ifndef ALL_SAFE_DEBUG_MODE
		servoLeft.attach(this->pinnumServoLeft);
		#endif
	}
	if (this->nvm.steeringBalance > 0)
	{
		ticks *= this->nvm.steeringBalance;
		ticks /= 1000;
	}
	ticks *= this->nvm.speedGain;
	ticks /= 1000;
	ticks += this->nvm.servoBiasLeft;

	#ifndef ALL_SAFE_DEBUG_MODE
	servoLeft.write(ticks);
	#endif
}

void cBookWorm::moveRightServo(signed int x)
{
	int32_t ticks = SERVO_CENTER_TICKS;

	x *= -1; // flip
	if (x > 0) {
		ticks += x;
		ticks += this->nvm.servoDeadzoneRight;
		#ifdef USE_CUSTOM_SERVO_LIB
		ContinuousServo_BothForward |= (1 << 0);
		#endif
	}
	else if (x <= 0) {
		ticks += x;
		ticks -= this->nvm.servoDeadzoneRight;
		#ifdef USE_CUSTOM_SERVO_LIB
		ContinuousServo_BothForward &= ~(1 << 0);
		#endif
	}
	if (x == 0 && this->nvm.servoStoppedNoPulse != false)
	{
		servoRight.detach();
	}
	else if (x != 0 && servoRight.attached() == false)
	{
		#ifndef ALL_SAFE_DEBUG_MODE
		servoRight.attach(this->pinnumServoRight);
		#endif
	}
	if (this->nvm.steeringBalance < 0)
	{
		ticks *= -this->nvm.steeringBalance;
		ticks /= 1000;
	}
	ticks *= this->nvm.speedGain;
	ticks /= 1000;
	ticks += this->nvm.servoBiasRight;

	#ifndef ALL_SAFE_DEBUG_MODE
	servoRight.write(ticks);
	#endif
}

void cBookWorm::moveMixed(signed int throttle, signed int steer)
{
	int32_t left, right;
	calcMix(throttle, steer, &left, &right);
	move(left, right);
}

void cBookWorm::calcMix(int32_t throttle, int32_t steer, int32_t * left, int32_t * right)
{
	double throttled, steerd;
	double diff;
	double leftd, rightd;
	double outputScale, maxTicks, maxRadius;

	maxTicks = this->nvm.servoMax;
	maxRadius = this->nvm.stickRadius;
	outputScale = maxTicks / maxRadius;

	steer *= this->nvm.steeringSensitivity;
	steer /= 1000;

	throttled = throttle;
	steerd = steer;

	if (steerd > maxRadius) {
		steerd = maxRadius;
	}
	else if (steerd < -maxRadius) {
		steerd = -maxRadius;
	}

	if (throttled > maxRadius) {
		throttled = maxRadius;
	}
	else if (throttled < -maxRadius) {
		throttled = -maxRadius;
	}

	leftd = steerd;
	rightd = -steerd;
	leftd += throttled;
	rightd += throttled;
	leftd *= outputScale;
	rightd *= outputScale;

	*left = (signed int)lround(leftd);
	*right = (signed int)lround(rightd);
}

void cBookWorm::loadPinAssignments()
{
	int tmp;
	this->pinnumServoLeft = pinServoLeft;
	this->pinnumServoRight = pinServoRight;
	#ifdef ENABLE_WEAPON
	if (this->nvm.enableWeapon)
	{
		#if pinServoWeapon == pinServoLeft || pinServoWeapon == pinServoRight
			#ifdef pinServoLeftAlt
				this->pinnumServoLeft = pinServoLeftAlt;
			#elif defined(pinServoRightAlt)
				this->pinnumServoRight = pinServoRightAlt;
			#else
				#error impossible weapon servo signal assignment
			#endif
		#endif
		this->pinnumServoWeapon = pinServoWeapon;
		if (this->pinsHaveLoaded == false) {
			#ifndef ALL_SAFE_DEBUG_MODE
			pinMode(this->pinnumServoWeapon, OUTPUT);
			digitalWrite(this->pinnumServoWeapon, LOW);
			#endif
		}
	}
	#endif
	if ((this->nvm.servoFlip & 0x04) != 0) // need to swap left and right
	{
		tmp = this->pinnumServoLeft;
		this->pinnumServoLeft = this->pinnumServoRight;
		this->pinnumServoRight = tmp;
	}
	if (this->pinsHaveLoaded == false)
	{
		#ifndef ALL_SAFE_DEBUG_MODE
		pinMode(this->pinnumServoLeft, OUTPUT);
		digitalWrite(this->pinnumServoLeft, LOW);
		pinMode(this->pinnumServoRight, OUTPUT);
		digitalWrite(this->pinnumServoRight, LOW);
		#endif
	}
	this->pinsHaveLoaded = true;
}

void cBookWorm::setServoDeadzoneLeft(unsigned int x)
{
	this->nvm.servoDeadzoneLeft = x;
}

void cBookWorm::setServoDeadzoneRight(unsigned int x)
{
	this->nvm.servoDeadzoneRight = x;
}

void cBookWorm::setServoBiasLeft(signed int x)
{
	this->nvm.servoBiasLeft = x;
}

void cBookWorm::setServoBiasRight(signed int x)
{
	this->nvm.servoBiasRight = x;
}

void cBookWorm::setServoStoppedNoPulse(bool x)
{
	this->nvm.servoStoppedNoPulse = x;
}

void cBookWorm::setServoMax(unsigned int x)
{
	this->nvm.servoMax = x;
}

void cBookWorm::setSpeedGain(signed int x)
{
	this->nvm.speedGain = x;
}

void cBookWorm::setSteeringSensitivity(signed int x)
{
	this->nvm.steeringSensitivity = x;
}

void cBookWorm::setSteeringBalance(signed int x)
{
	this->nvm.steeringBalance = x;
}

void cBookWorm::setStickRadius(uint16_t x)
{
	this->nvm.stickRadius = x;
}

void cBookWorm::setServoFlip(uint8_t x)
{
	this->nvm.servoFlip = x;
}

void cBookWorm::setRobotFlip(bool x)
{
	this->robotFlip = x;
}

void cBookWorm::togRobotFlip()
{
	this->robotFlip ^= true;
}

bool cBookWorm::getRobotFlip()
{
	return this->robotFlip;
}