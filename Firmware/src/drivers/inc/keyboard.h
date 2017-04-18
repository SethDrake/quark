
#pragma once
#ifndef __CO2_KEYBOARD_H_
#define __CO2_KEYBOARD_H_

#include "stm32f10x.h"

#define NUM_KEYS 3

class Keyboard {
public:
	Keyboard();
	~Keyboard();
	void setupHw(GPIO_TypeDef* kbrdPort, uint16_t keys[]);
	void readKeys(void);
	bool* GetKeysState(void);
	bool IsKeyPressed(uint8_t keyNumber);
	bool IsAnyKeyPressed();
	
private:
	GPIO_TypeDef* kbrdPort;
	uint16_t kbrdPins[NUM_KEYS];
	bool prevKeysState[NUM_KEYS];
	bool keysState[NUM_KEYS];
	uint16_t reqInterval;
	uint32_t lastTicksCount;
};

#endif
