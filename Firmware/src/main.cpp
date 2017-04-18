
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rtc.h"
#include <stm32f10x_adc.h>
#include <stm32f10x_dbgmcu.h>
#include <stm32f10x_flash.h>
#include "objects.h"
#include "screens.h"
#include "ili9341.h"
#include "keyboard.h"
#include "sdcard.h"
#include "radiation.h"
#include "settings.h"
#include "delay.h"
#include "misc.h"
#include "periph_config.h"

#define DEFAULT_INACTIVE_PERIOD 120 //120 seconds - period of inactivity before device going sleep
#define REDRAW_INTERVAL 250 //250 ms
#define DEFAULT_ALERT_THRESHOLD 100 //100 uR/h
#define ALERT_DURATION 450

#define PULSES_TO_DISCHARGE 40
#define DEFAULT_PUMP_ACTIVATION_INTERVAL 1 //pump hv activate every second
#define DEFAULT_PUMP_PULSE_DURATION 15

RCC_MODE rccMode;
SYSTEM_MODE systemMode = LOADING;
ScreensManager screensManager;
SettingsManager settingsManager;
ADC_MODE adcMode = BATTERY_VOLTAGE;
ILI9341 display;
Keyboard keyboard;
SDCARD sdcard;

RadiationCounter radiationCounter;

bool isHVPumpActive = false;
bool isDisplayActive = false;
bool isSoundActive = false;

bool doDisplayOnline = false;
bool doImpulseIndication = false;
bool doAlert = false;
bool doSleep = false;

float battVoltage = 4.2f;
float cpuTemp = 0.0f;

uint8_t hvPumpPulseDuration = DEFAULT_PUMP_PULSE_DURATION;
uint8_t hvPumpActivationInterval = DEFAULT_PUMP_ACTIVATION_INTERVAL;

uint16_t inactiveSecondsCounter = 0;
uint16_t pumpSecondsCounter = 0;
uint16_t pumpDiCounter = 0;

extern void drawRadiationGraph(uint8_t y, uint16_t barColor, uint16_t bkgColor);
extern void drawMinutlyRadiationGraph(uint8_t y, uint16_t barColor, uint16_t bkgColor);
extern void switchHvPumpMode(bool enable);
extern void wakeUpFromSleep();
extern void readADCValue(); 
extern void switchDisplay(bool on);
extern void switchIndicationLED(bool on);
extern void switchSound(bool on);
extern void switchVibration(bool on);
extern void startBattVoltageMeasure();
extern void startTemperatureMeasure();

extern void applyKeyboardActions();

void Faults_Configuration()
{
	//DBGMCU_Config(DBGMCU_SLEEP | DBGMCU_STANDBY | DBGMCU_STOP, ENABLE); //Enable debug in powersafe modes
	
	SCB->CCR |= SCB_CCR_DIV_0_TRP;	
}

void SetClock()
{
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);

	// Wait till HSE is ready
	if (RCC_WaitForHSEStartUp() == SUCCESS)
	{
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
		FLASH_SetLatency(FLASH_Latency_2);    
		RCC_HCLKConfig(RCC_SYSCLK_Div1);   
		RCC_PCLK1Config(RCC_HCLK_Div2);
		RCC_PCLK2Config(RCC_HCLK_Div1); 
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_3);
		RCC_PLLCmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
		while (RCC_GetSYSCLKSource() != 0x08);
		SystemCoreClockUpdate();
		rccMode = HSE;
	}
  /* else
	{
		RCC_HSEConfig(RCC_HSE_OFF);
		RCC_HSICmd(ENABLE);
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
		FLASH_SetLatency(FLASH_Latency_2);    
		RCC_HCLKConfig(RCC_SYSCLK_Div1);   
		RCC_PCLK1Config(RCC_HCLK_Div2);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16); //64MHz
		RCC_PLLCmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
		while (RCC_GetSYSCLKSource() != 0x08);
		SystemCoreClockUpdate();
		rccMode = HSI;
	}*/
}

void RCC_Configuration()
{
	SetClock();
	SysTick_Config(SystemCoreClock / 1000);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2 | RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_BKP | RCC_APB1Periph_PWR, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1, ENABLE);
	
	//clock DMA1 controller
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

void GPIO_Configuration()
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	    /* Configure SPI2 pins */
	GPIO_InitStructure.GPIO_Pin = SPI_SCK | SPI_MOSI | SPI_MISO;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_PORT, &GPIO_InitStructure);
	
	   /* Configure HV pins */
	GPIO_InitStructure.GPIO_Pin = HV_PUMP_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(HV_PUMP_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = HV_PULSE_PIN | HV_TEST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(HV_PUMP_PORT, &GPIO_InitStructure);
	   /* Configure INDICATION pins */
	GPIO_InitStructure.GPIO_Pin = IND_PULSE_PIN | IND_VIBRATE_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(INDICATION_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = IND_SOUND_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(INDICATION_PORT, &GPIO_InitStructure);
	
	/* Configure POWER CONTROL pins */
	GPIO_InitStructure.GPIO_Pin = PWR_CONTROL_DISP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(PWR_CONTROL_PORT, &GPIO_InitStructure);
	
	/* Configure BATT CONTROL pins */
	GPIO_InitStructure.GPIO_Pin = BATT_CTL_V_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(BATT_CTRL_PORT, &GPIO_InitStructure);
	
	/* Configure KEYBOARD pins */
	GPIO_InitStructure.GPIO_Pin = KEY_1_PIN | KEY_2_PIN | KEY_3_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(KBRD_PORT, &GPIO_InitStructure);

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); //disable JTAG
}

void I2C_Configuration()
{
	I2C_InitTypeDef  I2C_InitStructure;

	    /* I2C configuration */
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;

	    /* I2C Peripheral Enable */
	I2C_Cmd(I2C1, ENABLE);
	/* Apply I2C configuration after enabling it */
	I2C_Init(I2C1, &I2C_InitStructure);
}

void USART_Configuration()
{
	USART_InitTypeDef  USART_InitStructure;

	    /* USART configuration */
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	
	    /* USART Peripheral Enable */
	USART_Cmd(USART1, ENABLE);
	/* Apply USART configuration after enabling it */
	USART_Init(USART1, &USART_InitStructure);
}

void SPI_Configuration()
{
	SPI_InitTypeDef spiStruct;
	SPI_Cmd(SPI2, DISABLE);
	spiStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spiStruct.SPI_Mode = SPI_Mode_Master;
	spiStruct.SPI_DataSize = SPI_DataSize_8b;
	spiStruct.SPI_CPOL = SPI_CPOL_Low;
	spiStruct.SPI_CPHA = SPI_CPHA_1Edge;
	spiStruct.SPI_NSS = SPI_NSS_Soft;
	spiStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; /* PCLK2/2 = 72Mhz/2/2 = 18MHz SPI */
	spiStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	spiStruct.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &spiStruct);
	SPI_Cmd(SPI2, ENABLE);
	SPI_CalculateCRC(SPI2, DISABLE);
}

void TIM_Configuration()
{
	TIM_TimeBaseInitTypeDef TIM_BaseConfig;
	TIM_OCInitTypeDef TIM_OCConfig;

	// PWM
	TIM_BaseConfig.TIM_Prescaler = (uint16_t)(SystemCoreClock / 2500000) - 1;
	// Period - 100 tacts => 2500kHz/100 = 25 kHz
	TIM_BaseConfig.TIM_Period = 100-1; //40uS
	TIM_BaseConfig.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_BaseConfig.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_BaseConfig);

	TIM_OCConfig.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCConfig.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCConfig.TIM_Pulse = hvPumpPulseDuration;
	TIM_OCConfig.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM2, &TIM_OCConfig);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_Cmd(TIM2, ENABLE);
	
	// PWM - 320kHz
	TIM_BaseConfig.TIM_Prescaler = (uint16_t)(SystemCoreClock / 3200000) - 1;
	// Period - 312 tacts => 3200000/1000 = 3200Hz
	TIM_BaseConfig.TIM_Period = 1000 - 1;
	TIM_BaseConfig.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_BaseConfig.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_BaseConfig);

	TIM_OCConfig.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCConfig.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCConfig.TIM_Pulse = 400;
	TIM_OCConfig.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &TIM_OCConfig);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_Cmd(TIM3, ENABLE);
}

void EXTI_Configuration()
{
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_ClearITPendingBit(EXTI_Line2);
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);
	EXTI_ClearITPendingBit(EXTI_Line3);
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Configuration()
{

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(EXTI2_IRQn);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(EXTI2_IRQn);
	
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(RTCAlarm_IRQn);
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(ADC1_2_IRQn);
}

void RTC_Configuration()
{
	PWR_BackupAccessCmd(ENABLE);
	if (BKP_ReadBackupRegister(BKP_DR1) != BCK_DONE) //cold boot
	{
		RCC_LSEConfig(RCC_LSE_ON);
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
		RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
		RTC_WaitForLastTask();
		
		radiationCounter.SetTime({ 2017, 1, 29, 22, 56, 0 }); //default settings
		settingsManager.setInt(SETTINGS_INT_ALERT_THRESHOLD, DEFAULT_ALERT_THRESHOLD);
		settingsManager.setInt(SETTINGS_INT_SLEEP_INTERVAL, DEFAULT_INACTIVE_PERIOD);
		settingsManager.setBool(SETTINGS_BOOL_LED_ALLOWED, true);
		settingsManager.setBool(SETTINGS_BOOL_SOUND_ALLOWED, false);
		settingsManager.setBool(SETTINGS_BOOL_VIBRATION_ALLOWED, false);
		BKP_WriteBackupRegister(BKP_DR1, BCK_DONE);
	}
	else //warm boot
	{
		RTC_WaitForSynchro();
		radiationCounter.SetTotalSeconds(RTC_GetCounter());
	}
	RTC_WaitForLastTask();
	RTC_ClearITPendingBit(RTC_IT_ALR);
	RTC_WaitForLastTask();
	RTC_ITConfig(RTC_IT_ALR, ENABLE);
	//RTC_ITConfig(RTC_IT_SEC, ENABLE);
	RTC_WaitForLastTask();
	RTC_SetAlarm(RTC_GetCounter() + 1);
	RTC_WaitForLastTask();
}

void ADC_Configuration()
{
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);
	ADC_DeInit(ADC1);
	ADC_InitTypeDef  ADC_InitStructure;

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	
	ADC_Cmd(ADC1, ENABLE);
	
	//calibrate ADC
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	
	ADC_Cmd(ADC1, DISABLE);
}

void SleepModePreparation()
{
	ADC_Cmd(ADC1, DISABLE);
	//switch pins to AIN mode
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & !(HV_PULSE_PIN); //disable port A excluding impulse input
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & !(ILI9341_PIN_RESET); //disable port B excl display pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//disable peripheral clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2 | RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1, DISABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

void SleepMode()
{
	if (systemMode == SLEEP)
	{
		return;
	}
	display.resetIsDataSending();
	SleepModePreparation();
	switchIndicationLED(false);
	switchSound(false);
	systemMode = SLEEP;
	PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
}

void StandbyMode()
{
	if (systemMode == STANDBY)
	{
		return;
	}
	SleepModePreparation();
	switchIndicationLED(false);
	switchSound(false);
	RCC_RTCCLKCmd(DISABLE);
	systemMode = STANDBY;
	PWR_EnterSTANDBYMode();
}

void wakeUpFromSleep()
{
	if (systemMode != SLEEP)
	{
		return;
	}
	RCC_Configuration();
	GPIO_Configuration();
	switchIndicationLED(false);
	systemMode = ACTIVE;
} 


void processGeigerImpulse()
{ //tick on every radiation impulse
	if (systemMode == LOADING)
	{
		return;
	}
	if (systemMode == SLEEP) //awake from sleep - reinit peripheral
	{
		wakeUpFromSleep();
	}
	doImpulseIndication = true;
	if (!isDisplayActive)
	{
		if (settingsManager.getBool(SETTINGS_BOOL_LED_ALLOWED))
		{
			switchIndicationLED(true); //short blink in sleep
		}
		if (settingsManager.getBool(SETTINGS_BOOL_SOUND_ALLOWED))
		{
			switchSound(true);
		}
	}
	radiationCounter.TickImpulse();
	DelayManager::DelayUs(40);
	if (radiationCounter.GetCurrentSecondImpulseCount() > PULSES_TO_DISCHARGE)
	{
		switchHvPumpMode(true);
	}
	if (!isDisplayActive)
	{
		if (settingsManager.getBool(SETTINGS_BOOL_LED_ALLOWED))
		{
			switchIndicationLED(false); //short blink in sleep
		}
		if (settingsManager.getBool(SETTINGS_BOOL_SOUND_ALLOWED) && !doAlert)
		{
			switchSound(false);
		}
	}
	if (!isDisplayActive && !doDisplayOnline && !doAlert && !isHVPumpActive)
	{ 
		doSleep = true; //sleep if display inactive
	}
}

void processHVTestImpulse()
{ //tick on HV is enough
	if (systemMode == LOADING)
	{
		return;
	}
	switchHvPumpMode(false);
	if (!isDisplayActive && !doDisplayOnline && !doAlert)
	{ 
		doSleep = true; //sleep if display inactive
	}
}

void processPeriodicTasks()
{ //tick 1 second
	if (systemMode == LOADING)
	{
		return;
	}
	if (systemMode == SLEEP) //awake from sleep - reinit peripheral
	{
		wakeUpFromSleep();
	}
	radiationCounter.TickOneSecond();
	if (isHVPumpActive)
	{
		radiationCounter.IncPumpActiveDuration();
	}
	if (pumpSecondsCounter >= hvPumpActivationInterval)
	{
		pumpSecondsCounter = 0;
		switchHvPumpMode(true);	
	}
	pumpSecondsCounter++;
	if ((settingsManager.getInt(SETTINGS_INT_ALERT_THRESHOLD) > 0) && (radiationCounter.GetCurrentLevelMkRh() >=  settingsManager.getInt(SETTINGS_INT_ALERT_THRESHOLD)))
	{
		doAlert = true;
		if (!isDisplayActive)
		{
			doDisplayOnline = true;
		}
	}
	if (isDisplayActive)
	{	
		inactiveSecondsCounter++;
	}
	else
	{ 
		keyboard.readKeys();
		if (keyboard.IsAnyKeyPressed() && !isDisplayActive) //activate display & other
		{
			doDisplayOnline = true;
		}
		if (!doDisplayOnline && !doAlert && !isHVPumpActive)
		{
			doSleep = true; //sleep if display inactive
		}
	}
}


void applyKeyboardActions()
{
	//main screen shortcuts
	if (screensManager.getActualScreen() == MAIN)
	{
		if (keyboard.IsKeyPressed(1) && keyboard.IsKeyPressed(2))
		{
			inactiveSecondsCounter = 0;
			switchDisplay(false);
			doSleep = true;
			return;
		}
		if (keyboard.IsKeyPressed(1))
		{
			settingsManager.setBool(SETTINGS_BOOL_SOUND_ALLOWED, !settingsManager.getBool(SETTINGS_BOOL_SOUND_ALLOWED));	
		}
		if (keyboard.IsKeyPressed(2))
		{
			settingsManager.setBool(SETTINGS_BOOL_LED_ALLOWED, !settingsManager.getBool(SETTINGS_BOOL_LED_ALLOWED));	
		}
	}
	screensManager.updateKeys(keyboard.IsKeyPressed(0), keyboard.IsKeyPressed(1), keyboard.IsKeyPressed(2));
	
	if (keyboard.IsAnyKeyPressed() && settingsManager.getBool(SETTINGS_BOOL_SOUND_ALLOWED) && !doAlert)
	{
		switchSound(true);
		DelayManager::DelayMs(80);
		switchSound(false);
	}
}

int main()
{
	Faults_Configuration();
	RCC_Configuration();
	GPIO_Configuration();
	TIM_Configuration();
	//USART_Configuration();
	//I2C_Configuration();
	SPI_Configuration();
	ADC_Configuration();
	RTC_Configuration();
	EXTI_Configuration();
	NVIC_Configuration();
	
	switchHvPumpMode(true);
	
	/* setup display pins */
	display.setupHw(SPI2, SPI_BaudRatePrescaler_4, ILI9341_PORT, ILI9341_PIN_RS, ILI9341_PIN_RESET, ILI9341_PIN_CS);
	/* setup keyboard pins */
	uint16_t pins[] = { KEY_1_PIN, KEY_2_PIN, KEY_3_PIN };
	keyboard.setupHw(KBRD_PORT, pins);
	/* setup sdcard pins */
	sdcard.setupHw(SPI2, SPI_BaudRatePrescaler_4, SD_PORT, SD_PIN_CS);
	
	display.setPortrait();
	display.init();
	
	screensManager.init(&display, &radiationCounter, &settingsManager);

	systemMode = ACTIVE;
	
	doDisplayOnline = true;
	
	uint16_t indCounter = 0;
	uint16_t alertCounter = 0;
	
	uint32_t lastTickCount = DelayManager::GetSysTickCount();
	
	doImpulseIndication = true;
	
	startTemperatureMeasure();
	
	while (true)
	{
		if (doSleep)
		{
			doSleep = false;
			SleepMode();
		}
		
		if (doDisplayOnline)
		{
			switchIndicationLED(false);
			switchDisplay(true);		
			display.clear(BLACK);
			startBattVoltageMeasure();
			doDisplayOnline = false;
		}
		
		/*if ((battVoltage < 3.20f) && (battVoltage > 1.00f)) //deep discharge (if battery detected)
		{
			switchDisplay(false); //switch off display
			switchHvPumpMode(false);
			switchIndicationLED(false);
			switchSound(false);
			StandbyMode(); //stand by	
		}*/
		
		if (doAlert)
		{
			if (alertCounter == 0)
			{
				switchSound(true);	
			}
			alertCounter++;
			if (alertCounter > ALERT_DURATION * 100)
			{
				switchSound(false);
				alertCounter = 0;
				doAlert = false;
			}
		}
		
		if (isDisplayActive)
		{
			if (doImpulseIndication)
			{
				if (indCounter == 0)
				{
					if (settingsManager.getBool(SETTINGS_BOOL_LED_ALLOWED))
					{
						switchIndicationLED(true);
					}
					if (settingsManager.getBool(SETTINGS_BOOL_SOUND_ALLOWED))
					{
						switchSound(true);
					}
					if (settingsManager.getBool(SETTINGS_BOOL_VIBRATION_ALLOWED))
					{
						switchVibration(true);
					}
				}
				indCounter++;
				if (indCounter > 500)
				{
					switchIndicationLED(false);
					if (!doAlert)
					{
						switchSound(false);	
						switchVibration(false);	
					}
					indCounter = 0;
					doImpulseIndication = false;
				}
			}
			
			uint32_t ticksCount = DelayManager::GetSysTickCount();
		
			if (ticksCount - lastTickCount >= REDRAW_INTERVAL)
			{
				keyboard.readKeys();
				if (keyboard.IsAnyKeyPressed())
				{
					applyKeyboardActions();
					inactiveSecondsCounter = 0;
				}
				screensManager.drawScreen();
				lastTickCount = ticksCount;
				
				if ((inactiveSecondsCounter >  settingsManager.getInt(SETTINGS_INT_SLEEP_INTERVAL)) && (inactiveSecondsCounter > 0)) //if user inactivity time > sleepInterval
				{
					inactiveSecondsCounter = 0;
					switchDisplay(false); //switch off display
					doSleep = true; //sleep
				}

				if (radiationCounter.GetTime()->second == 30)
				{
					startBattVoltageMeasure();
				}
			}
		}
	}
}

#pragma region Switch external sensors and actuators
void switchHvPumpMode(bool enable) 
{
	if (enable)
	{
		TIM2->CCER |= TIM_CCER_CC2E;
	}
	else
	{
		TIM2->CCER &= ~TIM_CCER_CC2E;
	}
	isHVPumpActive = enable;
}

void switchSound(bool on)
{
	if (on == isSoundActive)
	{
		return;
	}
	if (on)
	{
		
		TIM3->CCER |= TIM_CCER_CC1E;
	}
	else
	{
		TIM3->CCER &= ~TIM_CCER_CC1E;
	}
	isSoundActive = on;
}

void switchDisplay(bool on)
{
	if (on)
	{
		display.sleep(false);
		display.enable(true);
		PWR_CONTROL_PORT->BSRR = PWR_CONTROL_DISP;	
	}
	else
	{
		display.resetIsDataSending();
		PWR_CONTROL_PORT->BRR = PWR_CONTROL_DISP;
		display.enable(false);
		display.sleep(true);
	}
	isDisplayActive = on;
}

void switchIndicationLED(bool on)
{
	if (on)
	{
		INDICATION_PORT->BSRR = IND_PULSE_PIN;	
	}
	else
	{
		INDICATION_PORT->BRR = IND_PULSE_PIN;	
	}
}

void switchVibration(bool on)
{
	if (on)
	{
		INDICATION_PORT->BSRR = IND_VIBRATE_PIN;	
	}
	else
	{
		INDICATION_PORT->BRR = IND_VIBRATE_PIN;	
	}
}

#pragma endregion

#pragma region Helpers
void readADCValue()
{
	ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
	ADC_SoftwareStartConvCmd(ADC1, DISABLE);
	uint16_t val = ADC_GetConversionValue(ADC1);
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	if (adcMode == BATTERY_VOLTAGE)
	{
		battVoltage = 2 * (val * 3.06f) / 4095;	
	}
	else if (adcMode == TEMPERATURE)
	{
		ADC_TempSensorVrefintCmd(DISABLE);
		float v25 = 1.43f;
		float avgSlope = 4.3f;
		cpuTemp = ((v25 - ((val * 3.3f) / 4095)) / avgSlope) + 25;
	}
	ADC_Cmd(ADC1, DISABLE);
}

void startBattVoltageMeasure()
{
	adcMode = BATTERY_VOLTAGE;
	ADC_Cmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);
	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void startTemperatureMeasure()
{
	adcMode = TEMPERATURE;
	ADC_Cmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_41Cycles5);
	ADC_TempSensorVrefintCmd(ENABLE);
	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

#pragma endregion

