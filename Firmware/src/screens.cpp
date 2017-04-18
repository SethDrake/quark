
#include "screens.h"
#include "objects.h"

ScreensManager::ScreensManager()
{
	screen = MAIN;
	actualMenuItemType = 255;
	
	menuItems[0] = { STATIC_ITEM, 50, 1, 1 };
	menuItems[1] = { DATE_YEAR_ITEM, 38, 2, 1 };
	menuItems[2] = { DATE_MONTH_ITEM, 20, 3, 1 };
	menuItems[3] = { DATE_DAY_ITEM, 20, 4, 1 };
	menuItems[4] = { STATIC_ITEM, 20, 5, 1 };
	menuItems[5] = { TIME_HOUR_ITEM, 20, 6, 1 };
	menuItems[6] = { TIME_MINUTE_ITEM, 20, 7, 1 };
	menuItems[7] = { TIME_SECOND_ITEM, 20, 8, 1 };
	
	menuItems[8] = { STATIC_ITEM, 60, 1, 2 };
	menuItems[9] = { ALERT_THRESHOLD_ITEM, 110, 2, 2 };
	menuItems[10] = { STATIC_ITEM, 60, 1, 3 };
	menuItems[11] = { BEFORE_SLEEP_TIME_ITEM, 70, 2, 3 };
	menuItems[12] = { STATIC_ITEM, 60, 1, 4 };
	menuItems[13] = { VIBRATION_ON_ITEM, 30, 2, 4 };
	menuItems[14] = { STATIC_ITEM, 60, 1, 5 };
	menuItems[15] = { SOUND_ON_ITEM, 30, 2, 5 };
	menuItems[16] = { STATIC_ITEM, 60, 1, 6 };
	menuItems[17] = { LED_ON_ITEM, 30, 2, 6 };
}

ScreensManager::~ScreensManager()
{
}

void ScreensManager::init(ILI9341* display, RadiationCounter* radiationCounter, SettingsManager* settingsManager)
{
	this->display = display;
	this->radiationCounter = radiationCounter;
	this->settingsManager = settingsManager;
}

void ScreensManager::switchToScreen(SCREEN newScreen)
{
	if (newScreen != screen)
	{
		screen = newScreen;
		display->clear(BLACK);
	}	
}

void ScreensManager::drawScreen(void)
{
	drawHeader();
	if (screen == MAIN)
	{
		drawMainScreen();
	}
	else if (screen == SETTINGS_MENU)
	{
		drawSettingMenu();
	}
	else if (screen == STATISTICS)
	{
		drawStatisticsScreen();
	}
}

void ScreensManager::drawHeader(void)
{
	time_s timeElapsed = *radiationCounter->GetTime();
	display->printf(5,305,YELLOW,BLACK,"%02u-%02u-%02u %02u:%02u:%02u",timeElapsed.year,timeElapsed.month,timeElapsed.day,timeElapsed.hour,timeElapsed.minute,timeElapsed.second);
			
	uint8_t battLvl = calcBattLevel();
	display->drawBattery(195, 305, battLvl, (battLvl < 1 ? RED : WHITE), BLACK);
	display->putChar(227, 305, (isHVPumpActive ? 0x85 : ' '), RED, BLACK);
	
	display->putChar(185, 305, (rccMode == HSE ? 'E' : 'I'), RED, BLACK);
	
	uint16_t intPart = cpuTemp;
	uint16_t floatPart = ((cpuTemp - intPart) * 10);
	display->printf(5, 293, WHITE, BLACK, "%02u.%01u\x81\x43", intPart, floatPart);
			
	intPart = battVoltage;
	floatPart = ((battVoltage - intPart) * 1000);
	display->printf(185, 292, WHITE, BLACK, "%01u.%03uV", intPart, floatPart);
}

void ScreensManager::drawFooter(const char* key1Title, const char* key2Title, const char* key3Title)
{
	display->fillScreen(0, 39, 239, 39, GRAY2);
	display->printf(0, 12, WHITE, BLACK, key1Title);
	display->fillScreen(79, 0, 79, 38, GRAY2);
	display->printf(80, 12, WHITE, BLACK, key2Title);
	display->fillScreen(159, 0, 159, 38, GRAY2);
	display->printf(160, 12, WHITE, BLACK, key3Title);
}

void ScreensManager::drawMainScreen(void)
{
	display->fillScreen(0, 220, 239, 220, GRAY2);
	display->printf(80, 206, WHITE, BLACK, "RADIATION");
	display->fillScreen(0, 129, 239, 129, GRAY2);
	
	display->printf(185, 280, RED, BLACK, "%03u", radiationCounter->GetPumpActivDurationPerMinute()); //pump activity sum duration at last minute
	display->printf(215, 280, WHITE, BLACK, "%02u", radiationCounter->GetValidateInterval()); //validate interval
			
	uint32_t sumDose = radiationCounter->GetFullDose();
	if (sumDose < 1000)
	{
		display->printf(5, 280, YELLOW, BLACK, "Sum dose:%3u \x83R   ", sumDose);	
	}
	else
	{
		float sumDoseMRh = sumDose * 1.0f / 1000;
		uint8_t intPart = sumDoseMRh;
		uint16_t floatPart = ((sumDoseMRh - intPart) * 1000);
		display->printf(5, 280, YELLOW, BLACK, "Sum dose:%3u.%02u mR   ", intPart, floatPart);	
	}
			
	uint32_t radLevel = radiationCounter->GetCurrentLevelMkRh();
	uint16_t radColor = calcRadColor(radLevel);
	if (radLevel < 1000)
	{
		display->printf(5, 250, radColor, BLACK, "RAD: %3u \x83R/h        ", radLevel);	
	}
	else
	{
		float radLevelMRh = radLevel * 1.0f / 1000;
		uint8_t intPart = radLevelMRh;
		uint16_t floatPart = ((radLevelMRh - intPart) * 1000);
		display->printf(5, 250, radColor, BLACK, "RAD: %3u.%02u mR/h        ", intPart, floatPart);	
	}
	drawRadiationBar(221, radColor, BLACK);		
	drawRadiationGraph(130, radColor, BLACK);
	drawMinutlyRadiationGraph(40, WHITE, BLACK);
	
	drawFooter("   MENU   ", settingsManager->getBool(SETTINGS_BOOL_SOUND_ALLOWED) ? "  SND ON  " : "  SND OFF ", settingsManager->getBool(SETTINGS_BOOL_LED_ALLOWED) ? "  LED ON  " : "  LED OFF ");
}

void ScreensManager::drawSettingMenu(void)
{
	time_s time = *radiationCounter->GetTime();
	display->printf(80, 292, WHITE, BLACK, "SETTINGS");
	
	drawItem(0, "DATE:", 0);
	drawItem(1, "%04u", time.year);
	drawItem(2, "%02u", time.month);
	drawItem(3, "%02u", time.day);
	drawItem(4, "  ", 0);
	drawItem(5, "%02u", time.hour);
	drawItem(6, "%02u", time.minute);
	drawItem(7, "%02u", time.second);
	
	drawItem(8, "ALERT:", 0);
	drawItem(9, "%08u \x83R/h", settingsManager->getInt(SETTINGS_INT_ALERT_THRESHOLD));
	drawItem(10, "SLEEP:", 0);
	drawItem(11, "%04u sec", settingsManager->getInt(SETTINGS_INT_SLEEP_INTERVAL));
	drawItem(12, "VIBR:", 0);
	drawItem(13, settingsManager->getBool(SETTINGS_BOOL_VIBRATION_ALLOWED) ? "YES" : "NO ", 0);
	drawItem(14, "SOUND:", 0);
	drawItem(15, settingsManager->getBool(SETTINGS_BOOL_SOUND_ALLOWED) ? "YES" : "NO ", 0);
	drawItem(16, "LED:", 0);
	drawItem(17, settingsManager->getBool(SETTINGS_BOOL_LED_ALLOWED) ? "YES" : "NO ", 0);
	
	//hide prev selection
	uint8_t prevSelectedItemIdx = (actualMenuItemType == 255) ? LAST_ITEM : actualMenuItemType - 1;
	prevSelectedItemIdx = getItemIndexByType(prevSelectedItemIdx);
	if (prevSelectedItemIdx == 255)
	{
		prevSelectedItemIdx = LAST_ITEM;
	}
	menu_item_s prevSelectedItem = menuItems[prevSelectedItemIdx];
	uint16_t prevYpos = 270 - (prevSelectedItem.row - 1) * ITEM_HEIGHT;
	uint8_t prevXpos = 5;
	if ((prevSelectedItemIdx > 0) && (prevSelectedItem.col > 1))
	{
		for (int8_t i = prevSelectedItemIdx - 1; i >= (prevSelectedItemIdx + 1 - prevSelectedItem.col); i--)
		{
			prevXpos += menuItems[i].width;	
		}
	}
	display->drawBorder(prevXpos - 1, prevYpos - 2, prevSelectedItem.width - 1, ITEM_HEIGHT-2, 1, BLACK);	
	
	if (actualMenuItemType == 255)
	{
		drawFooter("   STAT   ", "   SET    ", "          ");
	}
	else
	{
		drawFooter("   NEXT   ", "    +     ", "    -     ");
	}
}

uint8_t ScreensManager::getItemIndexByType(uint8_t itemType)
{
	for (int8_t i = 0; i < NUM_MENU_ITEMS; i++)
	{
		if (menuItems[i].itemType == itemType)
		{
			return i;
		}	
	}
	return 255;
}

void ScreensManager::drawItem(uint8_t itemNumber, const char* text, uint16_t val)
{
	menu_item_s item = menuItems[itemNumber];
	uint16_t ypos = 270 - (item.row - 1) * ITEM_HEIGHT;
	uint8_t xpos = 5;
	if ((itemNumber > 0) && (item.col > 1))
	{
		for (int8_t i = itemNumber - 1; i >= (itemNumber + 1 - item.col); i--)
		{
			xpos += menuItems[i].width;	
		}
	}
	display->printf(xpos, ypos, WHITE, BLACK, text, val);
	if (item.itemType == actualMenuItemType)
	{
		//draw selection frame
		display->drawBorder(xpos - 1, ypos - 2, item.width - 1, ITEM_HEIGHT-2, 1, GREEN);	
	}
}

void ScreensManager::drawStatisticsScreen(void)
{
	display->printf(80, 292, WHITE, BLACK, "STATISTICS");
	
	drawFooter("   MAIN   ", "    +     ", "  CLEAR   ");
}

void ScreensManager::updateKeys(bool key1Pressed, bool key2Pressed, bool key3Pressed)
{
	if (!key1Pressed && !key2Pressed && !key3Pressed) return;
	if (screen == MAIN)
	{
		if (key1Pressed)
		{
			switchToScreen(SETTINGS_MENU);
			return;
		}	
	}
	if (screen == SETTINGS_MENU)
	{
		if (actualMenuItemType == 255)
		{
			if (key1Pressed)
			{
				switchToScreen(STATISTICS);
				return;
			}
			if (key2Pressed)
			{
				actualMenuItemType = 0;
			}
		}
		else
		{
			if (key1Pressed)
			{
				
				actualMenuItemType++;
				if (actualMenuItemType > LAST_ITEM)
				{
					actualMenuItemType = 255;
				}
			}
			time_s t = *radiationCounter->GetTime();
			if (actualMenuItemType == DATE_YEAR_ITEM)
			{
				if (key2Pressed)
				{
					t.year++;
					radiationCounter->SetTime(t);
				}
				if (key3Pressed)
				{
					t.year--;
					radiationCounter->SetTime(t);
				}	
			}
			if (actualMenuItemType == DATE_MONTH_ITEM)
			{
				if (key2Pressed)
				{
					t.month++;
					if (t.month > 12)
					{
						t.month = 1;
					}
					radiationCounter->SetTime(t);
				}
				if (key3Pressed)
				{
					t.month--;
					if (t.month < 1)
					{
						t.month = 12;
					}
					radiationCounter->SetTime(t);
				}	
			}
			if (actualMenuItemType == DATE_DAY_ITEM)
			{
				uint8_t maxDayInMonth = radiationCounter->GetDaysInMonth(t.month, t.year);
				if (key2Pressed)
				{
					t.day++;
					if (t.month > maxDayInMonth)
					{
						t.day = 1;
					}
					radiationCounter->SetTime(t);
				}
				if (key3Pressed)
				{
					t.day--;
					if (t.day < 1)
					{
						t.day = maxDayInMonth;
					}
					radiationCounter->SetTime(t);
				}	
			}
			if (actualMenuItemType == TIME_HOUR_ITEM)
			{
				if (key2Pressed)
				{
					t.hour++;
					if (t.hour > 23)
					{
						t.hour = 0;
					}
					radiationCounter->SetTime(t);
				}
				if (key3Pressed)
				{
					t.hour--;
					if (t.hour < 0)
					{
						t.hour = 23;
					}
					radiationCounter->SetTime(t);
				}	
			}
			if (actualMenuItemType == TIME_MINUTE_ITEM)
			{
				if (key2Pressed)
				{
					t.minute++;
					if (t.minute > 59)
					{
						t.minute = 0;
					}
					radiationCounter->SetTime(t);
				}
				if (key3Pressed)
				{
					t.minute--;
					if (t.minute < 0)
					{
						t.minute = 59;
					}
					radiationCounter->SetTime(t);
				}	
			}
			if (actualMenuItemType == TIME_SECOND_ITEM)
			{
				if (key2Pressed)
				{
					t.second++;
					if (t.second > 59)
					{
						t.second = 0;
					}
					radiationCounter->SetTime(t);
				}
				if (key3Pressed)
				{
					t.second--;
					if (t.second < 0)
					{
						t.second = 59;
					}
					radiationCounter->SetTime(t);
				}	
			}
			if (actualMenuItemType == ALERT_THRESHOLD_ITEM)
			{
				if (key2Pressed)
				{
					settingsManager->setInt(SETTINGS_INT_ALERT_THRESHOLD, settingsManager->getInt(SETTINGS_INT_ALERT_THRESHOLD) + 50);
				}
				if (key3Pressed)
				{
					settingsManager->setInt(SETTINGS_INT_ALERT_THRESHOLD, settingsManager->getInt(SETTINGS_INT_ALERT_THRESHOLD) - 50);
				}	
			}
			if (actualMenuItemType == BEFORE_SLEEP_TIME_ITEM)
			{
				if (key2Pressed)
				{
					settingsManager->setInt(SETTINGS_INT_SLEEP_INTERVAL, settingsManager->getInt(SETTINGS_INT_SLEEP_INTERVAL) + 10);
				}
				if (key3Pressed)
				{
					settingsManager->setInt(SETTINGS_INT_SLEEP_INTERVAL, settingsManager->getInt(SETTINGS_INT_SLEEP_INTERVAL) - 10);
				}	
			}
			if (actualMenuItemType == VIBRATION_ON_ITEM)
			{
				if (key2Pressed || key3Pressed)
				{
					settingsManager->setBool(SETTINGS_BOOL_VIBRATION_ALLOWED, !settingsManager->getBool(SETTINGS_BOOL_VIBRATION_ALLOWED));
				}
			}
			if (actualMenuItemType == SOUND_ON_ITEM)
			{
				if (key2Pressed || key3Pressed)
				{
					settingsManager->setBool(SETTINGS_BOOL_SOUND_ALLOWED, !settingsManager->getBool(SETTINGS_BOOL_SOUND_ALLOWED));
				}
			}
			if (actualMenuItemType == LED_ON_ITEM)
			{
				if (key2Pressed || key3Pressed)
				{
					settingsManager->setBool(SETTINGS_BOOL_LED_ALLOWED, !settingsManager->getBool(SETTINGS_BOOL_LED_ALLOWED));
				}
			}
		}
	}
	else if (screen == STATISTICS)
	{
		if (key1Pressed)
		{
			switchToScreen(MAIN);
			return;
		}
	}
	
}

SCREEN ScreensManager::getActualScreen()
{
	return screen;
}

void ScreensManager::drawRadiationGraph(uint8_t y, uint16_t barColor, uint16_t bkgColor) 
{
	uint8_t i, j;
	uint8_t barWidth = 2;
	uint8_t maxHeight = 70;
	uint16_t buf[barWidth * maxHeight];
	uint16_t t;
	uint16_t* counts = radiationCounter->GetPerSecondCounts(); 
	uint16_t maxValue = *max_element(counts, counts + 120);
	float coeff = (maxHeight - 10) * 1.0f / maxValue;
	for (int8_t r = 119; r >= 0; r--)
	{
		uint16_t lineHeight = counts[r + 1] * coeff + 1;
		if (lineHeight > maxHeight)
		{
			lineHeight = maxHeight;
		}
		t = 0;
		for (i = maxHeight; i > 0; i--)
		{
			for (j = 0; j < barWidth; j++)
			{
				if (i <= lineHeight)
				{
					buf[t++] = barColor;
				}
				else
				{
					buf[t++] = bkgColor;
				}
			}		
		}
		display->bufferDraw(r * barWidth, y, barWidth, maxHeight - 1, buf);
	}
}

void ScreensManager::drawMinutlyRadiationGraph(uint8_t y, uint16_t barColor, uint16_t bkgColor) 
{
	uint8_t i, j;
	uint8_t barWidth = 4;
	uint8_t maxHeight = 90;
	uint16_t buf[barWidth * maxHeight];
	uint16_t t;
	uint16_t* counts = radiationCounter->GetPerMinuteCounts(); 
	uint16_t maxValue = *max_element(counts, counts + 60);
	uint16_t fillColor;
	uint8_t currentMinute = radiationCounter->GetTime()->minute;
	float coeff = (maxHeight - 10) * 1.0f / maxValue;
	for (int8_t r = 59; r >= 0; r--)
	{
		uint16_t lineHeight = counts[r + 1] * coeff + 1;
		if (r == currentMinute)
		{
			lineHeight = maxHeight;
			fillColor = GRAY2;
		}
		else
		{
			fillColor = barColor;
		}
		if (lineHeight > maxHeight)
		{
			lineHeight = maxHeight;
		}
		t = 0;
		for (i = maxHeight; i > 0; i--)
		{
			for (j = 0; j < barWidth; j++)
			{
				if (i <= lineHeight)
				{
					buf[t++] = fillColor;
				}
				else
				{
					buf[t++] = bkgColor;
				}
			}		
		}
		display->bufferDraw(r * barWidth, y, barWidth, maxHeight - 1, buf);
	}
}

void ScreensManager::drawRadiationBar(uint8_t y, uint16_t barColor, uint16_t bkgColor) 
{
	uint8_t i, j;
	uint8_t barHeight = 4;
	uint8_t maxWidth = 240;
	uint16_t buf[barHeight * maxWidth];
	uint16_t t = 0;
	uint32_t currentLevel = radiationCounter->GetCurrentLevelMkRh(); 
	uint32_t maxLevelToday = radiationCounter->GetMaxLevelTodayMkRh();
	uint32_t relLevel = currentLevel / (maxLevelToday * 1.0f / (maxWidth-40));
	for (j = 0; j < barHeight; j++)
	{
		for (i = 0; i < maxWidth; i++)
		{
			if (i <= relLevel)
			{
				buf[t++] = barColor;
			}
			else
			{
				buf[t++] = bkgColor;
			}
		}		
	}
	display->bufferDraw(0, y, maxWidth, barHeight, buf);
}

uint8_t ScreensManager::calcBattLevel(void)
{
	if (battVoltage >= 4.2f)
	{
		return 5; //100%	
	}
	if (battVoltage >= 3.95f)
	{
		return 4; //80%	
	}
	if (battVoltage >= 3.8f)
	{
		return 3; //60%	
	}
	if (battVoltage >= 3.7f)
	{
		return 2; //40%	
	}
	if (battVoltage >= 3.65f)
	{
		return 1; //20%	
	}
	return 0;
}

uint16_t ScreensManager::calcRadColor(uint32_t radLevel)
{
	if (radLevel >= 50)
	{
		return RED;
	}
	if (radLevel >= 25)
	{
		return YELLOW;
	} 
	if (radLevel >= 15)
	{
		return WHITE;
	}
	return GREEN;
}