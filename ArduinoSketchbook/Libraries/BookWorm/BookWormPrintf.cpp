#include <Arduino.h>
#include "stdio.h"
#include "stdarg.h"
#include "BookWorm.h"
#include "WString.h"

static int printf_putchar(char c, FILE *fp)
{
	Serial.write((uint8_t)c);
	return 0;
}

int cBookWorm::printf(const char *format, ...)
{
	FILE f;
	va_list ap;
	va_start(ap, format);
	return vprintf(format, ap);
}