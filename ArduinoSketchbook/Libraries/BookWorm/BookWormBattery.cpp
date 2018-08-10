#include "BookWorm.h"
#include <Arduino.h>
#include <stdlib.h>
#include <math.h>

#ifdef ENABLE_BATTERY_MONITOR

uint32_t cBookWorm::adcToVoltage(uint16_t x)
{
	/*
	vout = vin * r2 / (r1 + r2)
	vin = vout * (r1 + r2) / r2
	vin = (adc / 1023) * (r1 + r2) / r2
	vin = adc * (r1 + r2) / (r2 * 1023)
	vin_mv = 1000 * adc * (r1 + r2) / (r2 * 1023)
	*/
	uint32_t numer;
	uint32_t denom;

	if (nvm->vdiv_r2 <= 0) {
		return 0;
	}

	numer = nvm->vdiv_r1 + nvm->vdiv_r2;
	numer *= x;
	numer *= 1000;

	denom = nvm->vdiv_r2;
	denom *= 1023;

	return numer / denom;
}

uint32_t cBookWorm::readBatteryVoltageRaw(void)
{
	return adcToVoltage(analogRead(A0));
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
}

void cBookWorm::setVdivR2(uint32_t x)
{
	this->nvm->vdiv_r2 = x;
}

void cBookWorm::setVdivFilter(uint16_t x)
{
	if (x > 1000) {
		x = 1000;
	}
	this->nvm->vdiv_filter = x;
}

void cBookWorm::setBatteryWarningVoltage(uint16_t x)
{
	this->nvm->warning_voltage = x;
}

#endif
