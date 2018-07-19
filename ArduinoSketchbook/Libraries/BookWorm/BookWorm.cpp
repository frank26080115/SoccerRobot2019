#include <Arduino.h>
#include "BookWorm.h"
#include "ContinuousServo.h"

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