
#pragma once
#ifndef __RADIATION_H_
#define __RADIATION_H_

#include "stm32f10x.h"

typedef struct
{
	uint16_t year;
	int8_t month;
	int8_t day;
	int8_t hour;
	int8_t minute;
	int8_t second;
} time_s;

#define JD0 2451911 // days 01 Jan 2001
#define impulse_threshold 10

class RadiationCounter {
public:
	RadiationCounter();
	~RadiationCounter();
	
	void TickImpulse(void);
	void TickOneSecond(void);
	void IncPumpActiveDuration(void);
	uint32_t GetCurrentLevelMkRh(void);
	uint32_t GetMaxLevelTodayMkRh(void);
	uint16_t GetLastSecondImpulseCount(void);
	uint16_t GetCurrentSecondImpulseCount(void);
	uint16_t* GetPerSecondCounts(void);
	uint16_t* GetPerMinuteCounts(void);
	uint16_t* GetPerHourCounts(void);
	uint32_t GetTotalSeconds(void);
	uint32_t GetFullDose(void);
	uint32_t GetFullDoseDays(void);
	void SetTotalSeconds(uint32_t val);
	time_s* GetTime(void);
	void SetTime(time_s tm);
	uint8_t GetCountInterval(void);
	uint8_t GetValidateInterval(void);
	uint8_t GetPumpActivDurationPerMinute(void);
	bool HasPulsesInLast5Seconds(void);
	uint8_t GetDaysInMonth(uint8_t month, uint16_t year);
	uint32_t GetUptimeMinutes(void);
	
protected:
	void CalculateCurrentLevelMkRh(void);

private:
	uint16_t counts[122];
	uint16_t radPerMinutes[62];
	uint16_t radPerHours[26];
	uint16_t radPerDays[33];
	uint32_t levelsPerYear[14];
	uint32_t currentLevel;
	uint32_t fullDose;
	uint16_t fullDoseDays;
	uint32_t sumCountsForInterval;
	uint32_t maxLevel;
	uint32_t maxLevelToday;
	time_s   time;
	uint8_t  countTime;	
	uint8_t  validateInterval;	
	uint8_t  lastMinutePumpActiveSeconds;	
	uint8_t  pumpActiveSeconds;	
	uint32_t  uptime;	
};

#endif
