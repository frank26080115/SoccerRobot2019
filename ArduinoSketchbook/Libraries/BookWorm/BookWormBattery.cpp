#include "BookWorm.h"
#include <Arduino.h>
#include <stdlib.h>
#include <math.h>

#ifdef ENABLE_BATTERY_MONITOR

uint32_t cBookWorm::adcToVoltage(uint16_t x, uint32_t r1, uint32_t r2)
{
	/*
	vout = vin * r2 / (r1 + r2)
	vin = vout * (r1 + r2) / r2
	vin = (adc / 1023) * (r1 + r2) / r2
	vin = adc * (r1 + r2) / (r2 * 1023)
	vin_mv = ADC_REF_MV * adc * (r1 + r2) / (r2 * 1023)
	*/
	uint32_t numer;
	uint32_t denom;
	uint32_t adc = x;
	bool isHuge = false;

	if (r2 <= 0) {
		return 0;
	}

	if (this->vdivGcd <= 0) {
		this->vdivGcd = calcResistorGcd(r1, r2);
	}
	r1 /= this->vdivGcd;
	r2 /= this->vdivGcd;

	numer = r1 + r2;
	numer *= adc;
	if (numer >= 1000000) {
		numer /= 1023;
		isHuge = true;
	}
	numer *= ADC_REF_MV;

	denom = r2;
	if (isHuge == false) {
		denom *= 1023;
	}

	return numer / denom;
}

uint32_t cBookWorm::adcToVoltage(uint16_t x)
{
	return adcToVoltage(x, nvm->vdiv_r1, nvm->vdiv_r2);
}

uint32_t cBookWorm::readBatteryVoltageRaw(void)
{
	uint16_t v = analogRead(A0);
	if (v >= 1023) {
		if (batteryOverloadCnt < BATTERY_OVERLOAD_FILTER) {
			batteryOverloadCnt++;
		}
	}
	else {
		if (batteryOverloadCnt > (BATTERY_OVERLOAD_FILTER / 8)) {
			batteryOverloadCnt -= (BATTERY_OVERLOAD_FILTER / 8);
		}
		else {
			batteryOverloadCnt = 0;
		}
	}
	return adcToVoltage(v);
}

bool cBookWorm::isBatteryOverload(void) {
	return batteryOverloadCnt >= (BATTERY_OVERLOAD_FILTER / 2) && nvm->vdiv_r2 > 0;
}

uint16_t cBookWorm::readBatteryVoltageFiltered(void)
{
	uint32_t x = readBatteryVoltageRaw();
	uint32_t y = this->batteryVoltageFiltered;
	if (y <= 0) {
		this->batteryVoltageFiltered = x;
		return (uint16_t)x;
	}
	uint32_t filter_old = 1000 - this->nvm->vdiv_filter;
	uint32_t filter_new = this->nvm->vdiv_filter;
	y *= filter_old;
	x *= filter_new;
	y /= 1000;
	x /= 1000;
	this->batteryVoltageFiltered = y + x;
	return (uint16_t)this->batteryVoltageFiltered;
}

uint16_t cBookWorm::readBatteryVoltageFilteredLast(void)
{
	return (uint16_t)this->batteryVoltageFiltered;
}

bool cBookWorm::isBatteryLowWarning(void)
{
	uint16_t x = (uint16_t)lround(this->batteryVoltageFiltered);
	return (x < this->nvm->warning_voltage);
}

uint16_t cBookWorm::calcMaxBattVoltage(void)
{
	return (uint16_t)adcToVoltage(1023);
}

void cBookWorm::setVdivR1(uint32_t x)
{
	this->nvm->vdiv_r1 = x;
	this->vdivGcd = -1;
}

void cBookWorm::setVdivR2(uint32_t x)
{
	this->nvm->vdiv_r2 = x;
	this->vdivGcd = -1;
}

void cBookWorm::setVdivFilter(uint16_t x)
{
	if (x > 1000 || x == 0) {
		x = 1000;
	}
	this->nvm->vdiv_filter = x;
}

void cBookWorm::setBatteryWarningVoltage(uint16_t x)
{
	this->nvm->warning_voltage = x;
}

int32_t cBookWorm::calcResistorGcd(uint32_t u, uint32_t v)
{
	// https://en.wikipedia.org/wiki/Binary_GCD_algorithm

	int shift;

	/* GCD(0,v) == v; GCD(u,0) == u, GCD(0,0) == 0 */
	if (u == 0) return v;
	if (v == 0) return u;

	/* Let shift := lg K, where K is the greatest power of 2
		dividing both u and v. */
	/* If gcc is used, this can be replaced by
		shift = __builtin_ctz(u | v);
		u >>= shift; v >>= shift;
		to take advantage of CPU instruction */
	for (shift = 0; ((u | v) & 1) == 0; ++shift) {
		 u >>= 1;
		 v >>= 1;
	}

	while ((u & 1) == 0) {
		u >>= 1;
	}

	/* From here on, u is always odd. */
	do {
		/* remove all factors of 2 in v -- they are not common */
		/*   note: v is not zero, so while will terminate */
		/*   v >>= __builtin_ctz(v); if using gcc */
		while ((v & 1) == 0) {
			/* Loop X */
			v >>= 1;
		}

		/* Now u and v are both odd. Swap if necessary so u <= v,
		  then set v = v - u (which is even). For bignums, the
		  swapping is just pointer movement, and the subtraction
		  can be done in-place. */
		if (u > v) {
			unsigned int t = v; v = u; u = t; // Swap u and v.
		}
		v = v - u; // Here v >= u.

	} while (v != 0);

	/* restore common factors of 2 */
	return u << shift;
}

#endif
