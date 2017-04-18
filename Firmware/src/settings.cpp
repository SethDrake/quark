
#include "stm32f10x_pwr.h"
#include "stm32f10x_bkp.h"
#include "settings.h"


SettingsManager::SettingsManager() {
	PWR_BackupAccessCmd(ENABLE);
	readValuesFromBck();
}

SettingsManager::~SettingsManager() {
}


void SettingsManager::readValuesFromBck()
{
	uint16_t bckpDr2 = BKP_ReadBackupRegister(BKP_DR2);
	for (uint8_t i = 0; i < 16; i++)
	{
		bool_settings[i] = bckpDr2 & (1 << i);	
	}
	for (uint8_t i = 0; i < 5; i++)
	{
		int_settings[i] = BKP_ReadBackupRegister(BKP_DR3 + (i * 0x0004));
	}
}

bool SettingsManager::getBool(uint8_t key)
{
	assert_param(IS_KEY_LEQUAL_15(key));
	return bool_settings[key];
}

void SettingsManager::setBool(uint8_t key, bool value)
{
	assert_param(IS_KEY_LEQUAL_15(key));	
	if (bool_settings[key] != value)
	{
		bool_settings[key] = value;
		uint16_t bckpDr2 = BKP_ReadBackupRegister(BKP_DR2);
		if (bool_settings[key])
		{
			bckpDr2 |= (1 << key);	
		}
		else
		{
			bckpDr2 &= ~(1 << key);
		}
		BKP_WriteBackupRegister(BKP_DR2, bckpDr2);
	}
}

uint16_t SettingsManager::getInt(uint8_t key)
{
	assert_param(IS_KEY_LEQUAL_4(key));
	return int_settings[key];
}

void SettingsManager::setInt(uint8_t key, uint16_t value)
{
	assert_param(IS_KEY_LEQUAL_4(key));
	if (int_settings[key] != value) {
		int_settings[key] = value;
		BKP_WriteBackupRegister(BKP_DR3 + (key * 0x0004), int_settings[key]);
	}
}