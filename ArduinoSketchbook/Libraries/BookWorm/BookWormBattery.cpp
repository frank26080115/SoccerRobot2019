#include "BookWorm.h"
#include <Arduino.h>
#include <stdlib.h>
#include <math.h>

#ifdef ENABLE_BATTERY_MONITOR

double cBookWorm::adcToVoltage(uint16_t x)
{
	double y = x;
	double denom = this->nvm->vdiv_r1 + this->nvm->vdiv_r2;
	double numer = this->nvm->vdiv_r2;
	if (numer == 0) {
		numer = 1.0;
	}
	y = (denom * y) / (numer * 1023.0d);
	y *= 1000.d; // to millivolts
	return x;
}

double cBookWorm::readBatteryVoltageRaw(void)
{
	return adcToVoltage(analogRead(A0));
}

uint16_t cBookWorm::readBatteryVoltageFiltered(void)
{
	double x = readBatteryVoltageRaw();
	double y = this->batteryVoltageFiltered;
	if (y <= 0) {
		this->batteryVoltageFiltered = x;
		return (uint16_t)lround(x);
	}
	double filter_old = 1000 - this->nvm->vdiv_filter;
	double filter_new = this->nvm->vdiv_filter;
	y *= filter_old;
	x *= filter_new;
	y /= 1000.0d;
	x /= 1000.0d;
	this->batteryVoltageFiltered = y + x;
	return (uint16_t)lround(this->batteryVoltageFiltered);
}

uint16_t cBookWorm::readBatteryVoltageFilteredLast(void)
{
	return (uint16_t)lround(this->batteryVoltageFiltered);
}

bool cBookWorm::isBatteryLowWarning(void)
{
	uint16_t x = (uint16_t)lround(this->batteryVoltageFiltered);
	return (x < this->nvm->warning_voltage);
}

uint16_t cBookWorm::calcMaxBattVoltage(void)
{
	return (uint16_t)lround(adcToVoltage(1023.0d));
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
