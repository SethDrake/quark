
#include <algorithm>
#include "stm32f10x_pwr.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_rtc.h"
#include "radiation.h"

static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

RadiationCounter::RadiationCounter() {
	pumpActiveSeconds = 0;
	sumCountsForInterval = 0;
	lastMinutePumpActiveSeconds = 0;
	maxLevel = 0;
	maxLevelToday = 0;
	countTime = 44;
	validateInterval = 44;
	fullDose = 0;
	fullDoseDays = 0;
	std::fill_n(counts, 44, 0);
	std::fill_n(radPerMinutes, 62, 0);
	std::fill_n(radPerHours, 26, 0);
	std::fill_n(radPerDays, 33, 0);
	time.year = 2016;
	time.month = 1;
	time.day = 1;
	fullDose = ((uint32_t)BKP_ReadBackupRegister(BKP_DR8) << 16) + BKP_ReadBackupRegister(BKP_DR9);
	fullDoseDays = BKP_ReadBackupRegister(BKP_DR10);
	uptime = 0;
}

RadiationCounter::~RadiationCounter() {
}

void RadiationCounter::TickImpulse()
{
	counts[0]++;
}

void RadiationCounter::IncPumpActiveDuration()
{
	pumpActiveSeconds++;
}

void RadiationCounter::TickOneSecond()
{
	SetTotalSeconds(RTC_GetCounter());
	for (uint8_t i = 42; i > 0; i--)
	{
		sumCountsForInterval += counts[i];
	}
	for (uint8_t i = 121; i > 0; i--)
	{
		counts[i] = counts[i - 1];
	}
	counts[0] = 0;
	//time.second++;
	if (validateInterval > 0)
	{
		validateInterval--;
	}
	if (countTime - validateInterval > 3) // сброс валидатора, для оперативного пересчета показаний при резкой смене фона, сравним элементы массива импульсов через 1
	{
		if (((counts[1] > counts[3]) && ((counts[1] - counts[3]) > impulse_threshold)) || ((counts[1] < counts[3]) && ((counts[3] - counts[1]) > impulse_threshold)))
		{
			validateInterval = countTime;
		}

	}

	
	//if (time.second == 60) // one minute elapsed
	if (time.second == 0) // one minute elapsed
	{
		//time.second = 0;
		radPerMinutes[time.minute] = sumCountsForInterval / 60;
		sumCountsForInterval = 0;
		
		if (validateInterval == 0)
		{
			int lastMinute = time.minute - 1;
			if (lastMinute < 0)
			{
				lastMinute = 59;
			}
			if (maxLevelToday < radPerMinutes[lastMinute]) {
				maxLevelToday = radPerMinutes[lastMinute];
			}
			if (maxLevel < maxLevelToday)
			{
				maxLevel = maxLevelToday;
			}
		}
		
		//time.minute++;
		lastMinutePumpActiveSeconds = pumpActiveSeconds;
		pumpActiveSeconds = 0;
		uptime++;
	}
	
	//if (time.minute == 60) // one hour elapsed
	if (time.minute == 0) // one hour elapsed
	{
		//time.minute = 0;
		for (uint8_t i = 0; i < 60; i++) {
			radPerHours[time.hour] += radPerMinutes[i];
		}
		radPerHours[time.hour] = radPerHours[time.hour] / 60; // 60 поминутных выборок усредняем - средний уровнь за час
		radPerDays[0] += radPerHours[time.hour];
		fullDose += radPerHours[time.hour];
		BKP_WriteBackupRegister(BKP_DR8, (uint16_t)(fullDose >> 16));
		BKP_WriteBackupRegister(BKP_DR9, (uint16_t)(fullDose & 0xFFFF));
		//time.hour++;
	}
	
	//if (time.hour == 24) // one day elapsed
	if (time.hour == 0) // one day elapsed
	{
		//time.hour = 0;
		for (uint8_t i = 31; i > 0; i--)
		{
			radPerDays[i] = radPerDays[i - 1]; // смещаем массив
		}
		radPerDays[0] = 0;
		//time.day++;
		fullDoseDays++;
		BKP_WriteBackupRegister(BKP_DR10, fullDoseDays);
		
		/*uint8_t daysInCurrentMonth = this->GetDaysInMonth(time.month, time.year);
		if (time.day > daysInCurrentMonth)
		{
			time.month++;
			time.day = 1;
			
			if (time.month > 12)
			{
				time.year++;
				time.month = 1;
			}
		}*/
	}
	
	//this->SetTime(time);
	this->CalculateCurrentLevelMkRh();
}


void RadiationCounter::CalculateCurrentLevelMkRh()
{
	uint32_t rad = 0;
	bool isRadiationCounted = false;
	for (int8_t i = 1; i < 45; i++) // в counts[0] неполнaя инфа, не юзаем.
	{
		rad += counts[i];
		if (!isRadiationCounted)
		{	
			switch (i) // секунда
			{
			case 6:
				if (rad > 2000) // фон больше 22000 мкр/ч интервал расчета 6 сек
				{
					rad *= 11;
					isRadiationCounted = true;
					if (countTime != 6)
					{
						countTime = 6;
						validateInterval = countTime;
					}
				}
				break;
				
			case 11:
				if (rad > 200) // фон больше 400 мкр/ч интервал расчета 11 сек
				{
					rad *= 4;
					isRadiationCounted = true;
					if (countTime != 11)
					{
						countTime = 11;
						validateInterval = countTime;
					}
				}
				break;

			case 22:
				if (rad > 100) // фон больше 200 мкр/ч интервал расчета 22 сек
				{
					rad *= 2;
					isRadiationCounted = true;
					if (countTime != 22)
					{
						countTime = 22;
						validateInterval = countTime;
					}
				}
				break;
    
			case 44:
				isRadiationCounted = true;
				if (countTime != 44)
				{
					countTime = 44;
					validateInterval = countTime;
				}
				break;
			}
		}
	}
	if ((validateInterval > 0) && (countTime - validateInterval > 4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
	{
		uint8_t approx = (countTime - validateInterval);
		rad = 0;
		for (uint8_t i = 1; i < approx + 1; i++)
		{
			rad = rad + counts[i];
		}
		rad = rad * 44 / approx;
	}
	this->currentLevel = rad;
}

uint32_t RadiationCounter::GetCurrentLevelMkRh()
{
	return this->currentLevel;	
}


uint32_t RadiationCounter::GetMaxLevelTodayMkRh()
{
	return this->maxLevelToday;
}

uint16_t RadiationCounter::GetLastSecondImpulseCount()
{
	return counts[1];	
}


uint16_t RadiationCounter::GetCurrentSecondImpulseCount()
{
	return counts[0];
}

uint32_t RadiationCounter::GetTotalSeconds()
{
	uint8_t a;
	uint16_t y;
	uint8_t m;
	uint32_t JDN;

	a = (14 - time.month) / 12;
	y = time.year + 4800 - a;
	m = time.month + (12*a) - 3;
	JDN = time.day;
	JDN += (153*m + 2) / 5;
	JDN += 365*y;
	JDN += y / 4;
	JDN += -y / 100;
	JDN += y / 400;
	JDN += -32045;
	JDN += -JD0;
	JDN *= 86400; 
	JDN += (time.hour * 3600);
	JDN += (time.minute * 60);
	JDN += (time.second);
	return JDN;
}

void RadiationCounter::SetTotalSeconds(uint32_t val)
{
	uint32_t ace;
	uint8_t b;
	uint8_t d;
	uint8_t m;

	ace = (val / 86400) + 32044 + JD0;
	b = (4*ace + 3) / 146097;
	ace = ace - ((146097*b) / 4);
	d = (4*ace + 3) / 1461;
	ace = ace - ((1461*d) / 4);
	m = (5*ace + 2) / 153;
	time.day = ace - ((153*m + 2) / 5) + 1;
	time.month = m + 3 - (12*(m / 10));
	time.year = 100*b + d - 4800 + (m / 10);
	time.hour = (val / 3600) % 24;
	time.minute = (val / 60) % 60;
	time.second = (val % 60);
}

time_s* RadiationCounter::GetTime(void)
{
	return &time;
}

void RadiationCounter::SetTime(time_s tm)
{
	this->time = tm;
	RTC_SetCounter(this->GetTotalSeconds());
	RTC_WaitForLastTask();
	RTC_SetAlarm(RTC_GetCounter() + 1);
}

uint8_t RadiationCounter::GetCountInterval()
{
	return countTime;
}


uint8_t RadiationCounter::GetValidateInterval()
{
	return validateInterval;
}

bool RadiationCounter::HasPulsesInLast5Seconds()
{
	return (counts[1] + counts[2] + counts[3] + counts[4] + counts[5] > 0);
}

uint16_t* RadiationCounter::GetPerSecondCounts()
{
	return counts;
}

uint16_t* RadiationCounter::GetPerMinuteCounts()
{
	return radPerMinutes;	
}

uint16_t* RadiationCounter::GetPerHourCounts()
{
	return radPerDays;
}

uint8_t RadiationCounter::GetDaysInMonth(uint8_t month, uint16_t year)
{
	uint8_t	daysInMonth = days_in_month[month - 1];
	if (month == 2)
	{
		bool isLeapYear = (year % 4 == 0) && ((year % 4 != 0) || (year % 400 == 0));
		if (isLeapYear)
		{
			daysInMonth++;
		}
	}
	return daysInMonth;
}

uint32_t RadiationCounter::GetFullDose()
{
	return this->fullDose;
}

uint32_t RadiationCounter::GetFullDoseDays()
{
	return this->fullDoseDays;
}

uint8_t RadiationCounter::GetPumpActivDurationPerMinute()
{
	return this->lastMinutePumpActiveSeconds;
}

uint32_t RadiationCounter::GetUptimeMinutes()
{
	return this->uptime;
}