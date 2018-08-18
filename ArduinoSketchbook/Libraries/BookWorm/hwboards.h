#ifndef _HWBOARDS_H_
#define _HWBOARDS_H_

// pick only one of these
//#define HWBOARD_ESP01
#define HWBOARD_ESP12_NANO

#define pinBoot 2

#ifdef HWBOARD_ESP01
	#define pinServoLeft         2
	#define pinServoRight        0

	#ifdef ENABLE_WEAPON
	#define pinServoWeapon       2
	#define pinServoLeftAlt      1
	#endif

	#define pinLed LED_BUILTIN // WARNING: ESP-01, usage of LED will disable serial port
	#define pinLedOnState LOW
	#define pinLedOffState HIGH

	#define VDIV_R2_DEFAULT 0
	#define VDIV_R1_DEFAULT 0
	#define ADC_REF_MV 1000
#endif

#ifdef HWBOARD_ESP12
	#define pinServoLeft         13
	#define pinServoRight        15

	#ifdef ENABLE_WEAPON
	#define pinServoWeapon       4
	#endif

	#define pinLed1 2
	#define pinLed2 5
	#define pinLed1OnState LOW
	#define pinLed1OffState HIGH
	#define pinLed2OnState HIGH
	#define pinLed2OffState LOW

	#define VDIV_R2_FACTORY 1000
	#define VDIV_R2_DEFAULT VDIV_R2_FACTORY
	#define VDIV_R1_DEFAULT 7500
	#define ADC_REF_MV 1000
#endif

#ifdef HWBOARD_ESP12_NANO
	#define pinServoLeft         13
	#define pinServoRight        15

	#ifdef ENABLE_WEAPON
	#define pinServoWeapon       4
	#endif

	#define pinLed1 2
	#define pinLed2 5
	#define pinLed1OnState LOW
	#define pinLed1OffState HIGH
	#define pinLed2OnState HIGH
	#define pinLed2OffState LOW

	#define VDIV_R2_FACTORY 470
	#define VDIV_R2_DEFAULT VDIV_R2_FACTORY
	#define VDIV_R1_DEFAULT 5600
	#define ADC_REF_MV 1000
#endif

#endif
