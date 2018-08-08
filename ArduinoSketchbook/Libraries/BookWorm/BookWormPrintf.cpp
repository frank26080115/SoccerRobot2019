#include <Arduino.h>
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "BookWorm.h"
#include "WString.h"

//#define PRINTF_USE_FPRINTF // doesn't work
#define PRINTF_USE_STRING

#if defined(PRINTF_USE_FPRINTF) || defined(PRINTF_USE_STRING)
static int printf_putchar(char c, FILE *fp)
{
	Serial.write((uint8_t)c);
	return 0;
}
#endif

int cBookWorm::printf(const char *format, ...)
{
	#if defined(ENABLE_WEAPON) && (pinServoWeapon == pinServoLeft || pinServoWeapon == pinServoRight)
	if (this->nvm.enableWeapon != false)
	{
		return 0;
	}
	#endif

	if (this->serialHasBegun == false) {
		return 0;
	}

	#ifdef PRINTF_USE_FPRINTF
	FILE f;
	f.put = printf_putchar;
	f.flags = 0x02; // write
	#endif

	va_list ap;
	va_start(ap, format);

	while (Serial.availableForWrite() < strlen(format)) { }

	#ifdef PRINTF_USE_FPRINTF
	return vfprintf(&f, format, ap);
	#elif defined(PRINTF_USE_STRING)
	char* str = (char*)malloc(system_get_free_heap_size() / 8);
	int ret = vsprintf(str, format, ap);
	int idx;
	for (idx = 0; idx < ret; idx++) {
		printf_putchar(str[idx], NULL);
	}
	free(str);
	return ret;
	#else
	return vprintf(format, ap);
	#endif
}

int cBookWorm::debugf(const char *format, ...)
{
	#ifndef BOOKWORM_DEBUG
	return 0;
	#endif

	#if defined(ENABLE_WEAPON) && (pinServoWeapon == pinServoLeft || pinServoWeapon == pinServoRight)
	if (this->nvm.enableWeapon != false)
	{
		return 0;
	}
	#endif

	if (this->serialHasBegun == false) {
		return 0;
	}

	#ifdef PRINTF_USE_FPRINTF
	FILE f;
	f.put = printf_putchar;
	f.flags = 0x02; // write
	#endif

	va_list ap;
	va_start(ap, format);

	while (Serial.availableForWrite() < strlen(format)) { }

	#ifdef PRINTF_USE_FPRINTF
	return vfprintf(&f, format, ap);
	#elif defined(PRINTF_USE_STRING)
	int sz = system_get_free_heap_size() / 8;
	char* str = (char*)malloc(sz);
	int ret = vsnprintf(str, sz - 1, format, ap);
	int idx;
	for (idx = 0; idx < ret; idx++) {
		printf_putchar(str[idx], NULL);
	}
	free(str);
	return ret;
	#else
	return vprintf(format, ap);
	#endif
}