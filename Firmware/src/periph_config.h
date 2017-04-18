
#pragma once
#ifndef __PERIPH_CONFIG_H_
#define __PERIPH_CONFIG_H_

/* PA13, PA14 - SWD - DON'T USE!!! */
/*        PA11, PA12 - USB		   */

#define HV_PUMP_PORT		GPIOA
#define HV_PUMP_PIN			GPIO_Pin_1
#define HV_PULSE_PIN		GPIO_Pin_2
#define HV_TEST_PIN			GPIO_Pin_3

#define INDICATION_PORT		GPIOA
#define IND_PULSE_PIN		GPIO_Pin_4
#define IND_VIBRATE_PIN		GPIO_Pin_5
#define IND_SOUND_PIN		GPIO_Pin_6

#define PWR_CONTROL_PORT	GPIOA
#define PWR_CONTROL_DISP	GPIO_Pin_7

#define SD_PORT				GPIOA
#define SD_PIN_CS			GPIO_Pin_8

#define BATT_CTRL_PORT		GPIOB
#define BATT_CTL_V_PIN		GPIO_Pin_0

#define KBRD_PORT			GPIOB
#define KEY_1_PIN			GPIO_Pin_1
#define KEY_2_PIN			GPIO_Pin_2
#define KEY_3_PIN			GPIO_Pin_3

#define ILI9341_PORT        GPIOB
#define ILI9341_PIN_RS      GPIO_Pin_10
#define ILI9341_PIN_RESET   GPIO_Pin_11
#define ILI9341_PIN_CS      GPIO_Pin_12

#define SPI_PORT			GPIOB
#define SPI_SCK				GPIO_Pin_13
#define SPI_MISO			GPIO_Pin_14
#define SPI_MOSI			GPIO_Pin_15

#endif //__PERIPH_CONFIG_H_
