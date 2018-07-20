#include <Arduino.h>
#include "stdio.h"
#include "stdarg.h"
#include "BookWorm.h"
#include "WString.h"

#ifdef PRINTF_USE_FPRINTF
static int printf_putchar(char c, FILE *fp)
{
	Serial.write((uint8_t)c);
	return 0;
}
#endif

int cBookWorm::printf(const char *format, ...)
{
	#ifdef PRINTF_USE_FPRINTF
	FILE f;
	f.put = printf_putchar;
	f.flags = 0x02; // write
	#endif

	va_list ap;
	va_start(ap, format);
	#ifdef PRINTF_USE_FPRINTF
	return vfprintf(&f, format, ap);
	#else
	return vprintf(format, ap);
	#endif
}