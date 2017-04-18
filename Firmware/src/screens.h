
#pragma once
#ifndef __SCREENS_H
#define __SCREENS_H

#include "radiation.h"
#include "ili9341.h"
#include "settings.h"

#define NUM_MENU_ITEMS 18
#define ITEM_HEIGHT 16
#define LAST_ITEM 9

typedef enum
{ 
	MAIN          = 0,
	SETTINGS_MENU,
	STATISTICS
} SCREEN;

typedef enum
{      
	DATE_YEAR_ITEM = 0, 
	DATE_MONTH_ITEM,
	DATE_DAY_ITEM,
	TIME_HOUR_ITEM,
	TIME_MINUTE_ITEM,
	TIME_SECOND_ITEM,
	ALERT_THRESHOLD_ITEM,
	BEFORE_SLEEP_TIME_ITEM,
	VIBRATION_ON_ITEM,
	SOUND_ON_ITEM,
	LED_ON_ITEM,
	
	STATIC_ITEM = 250
} SETTINGS_MENU_ITEMS;

typedef struct
{
	uint8_t itemType;
	uint8_t width;
	uint8_t col;
	uint8_t row;
} menu_item_s;


class ScreensManager {
	public:
		ScreensManager();
		~ScreensManager();
		void init(ILI9341* display, RadiationCounter* radiationCounter, SettingsManager* settingsManager);
		void updateKeys(bool key1State, bool key2State, bool key3State);
		void drawScreen();
		SCREEN getActualScreen();
	protected:
		void switchToScreen(SCREEN newScreen);
		void drawHeader(void);
		void drawFooter(const char* key1Title, const char* key2Title, const char* key3Title);
		void drawMainScreen(void);
		void drawSettingMenu(void);
		void drawStatisticsScreen(void);
	private:
		SCREEN screen;
		uint8_t actualMenuItemType;
		ILI9341* display;
		RadiationCounter* radiationCounter;
		SettingsManager* settingsManager;
		void drawRadiationGraph(uint8_t y, uint16_t barColor, uint16_t bkgColor);
		void drawMinutlyRadiationGraph(uint8_t y, uint16_t barColor, uint16_t bkgColor);
		void drawRadiationBar(uint8_t y, uint16_t barColor, uint16_t bkgColor);
	static uint8_t calcBattLevel(void);
	static uint16_t calcRadColor(uint32_t radLevel);
		void drawItem(uint8_t itemNumber, const char* text, uint16_t val);
		uint8_t getItemIndexByType(uint8_t itemType);
		
		menu_item_s menuItems[NUM_MENU_ITEMS];
};

#endif /* __SCREENS_H */


