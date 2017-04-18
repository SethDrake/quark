#pragma once
#ifndef __OBJECTS_H_
#define __OBJECTS_H_

#include "ili9341.h"

template<class ForwardIt>
	ForwardIt max_element(ForwardIt first, ForwardIt last)
	{
		if (first == last) {
			return last;
		}
		ForwardIt largest = first;
		++first;
		for (; first != last; ++first) {
			if (*largest < *first) {
				largest = first;
			}
		}
		return largest;
	}

typedef enum
{ 
	LOADING = 0,
	ACTIVE,
	SLEEP,
	STANDBY
} SYSTEM_MODE;

typedef enum
{ 
	BATTERY_VOLTAGE = 0,
	TEMPERATURE
} ADC_MODE;

typedef enum
{ 
	HSE = 0,
	HSI
} RCC_MODE;

extern ILI9341 display;
extern SYSTEM_MODE systemMode;
extern RCC_MODE rccMode;

extern bool isHVPumpActive;

extern float battVoltage;
extern float cpuTemp;

extern void processGeigerImpulse(void);
extern void processHVTestImpulse(void);
extern void processPeriodicTasks(void);
extern void readADCValue(void);

#endif //__OBJECTS_H_