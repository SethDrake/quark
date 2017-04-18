
#pragma once
#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#include "stm32f10x.h"

#define BCK_DONE 0xA5A5

#define IS_KEY_LEQUAL_15(KEY) ( KEY <= 15 )
#define IS_KEY_LEQUAL_4(KEY) ( KEY <= 4 )

typedef enum
{ 
	SETTINGS_BOOL_LED_ALLOWED = 0,
	SETTINGS_BOOL_SOUND_ALLOWED,
	SETTINGS_BOOL_VIBRATION_ALLOWED
} SETTINGS_BOOL;

typedef enum
{ 
	SETTINGS_INT_ALERT_THRESHOLD = 0,
	SETTINGS_INT_SLEEP_INTERVAL
} SETTINGS_INT;

class SettingsManager {
public:
	SettingsManager();
	~SettingsManager();
	bool getBool(uint8_t key);
	void setBool(uint8_t key, bool value);
	uint16_t getInt(uint8_t key);
	void setInt(uint8_t key, uint16_t value);
protected:
private:
	void readValuesFromBck();
	bool bool_settings[16];
	uint16_t int_settings[5];
};

#endif
