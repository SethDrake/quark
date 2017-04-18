
#include <algorithm>
#include <stm32f10x_gpio.h>
#include "keyboard.h"

Keyboard::Keyboard() {
	reqInterval = 30; //30 ms
	std::fill_n(keysState, NUM_KEYS, 0);
}

Keyboard::~Keyboard() {
}

void Keyboard::setupHw(GPIO_TypeDef* kbrdPort, uint16_t keys[])
{
	this->kbrdPort = kbrdPort;
	std::copy(keys, keys + NUM_KEYS, this->kbrdPins);
}

void Keyboard::readKeys()
{
	uint8_t i;
	//read all KBRD_PORT
	uint16_t kbrdPortValues = GPIO_ReadInputData(this->kbrdPort);
	for (i = 0; i < NUM_KEYS; i++)
	{
		bool currentState = !(kbrdPortValues & kbrdPins[i]);
		//if (currentState == prevKeysState[i]) //if key state not changed from last scan - confirm state
		{
			keysState[i] = currentState;
		}
		prevKeysState[i] = currentState;
	}
}

bool* Keyboard::GetKeysState()
{
	return this->keysState;
}

bool Keyboard::IsKeyPressed(uint8_t keyNumber)
{
	return this->keysState[keyNumber];
}

bool Keyboard::IsAnyKeyPressed()
{
	uint8_t i;
	for (i = 0; i < NUM_KEYS; i++)
	{
		if (this->keysState[i])
		{
			return true;
		}
	}
	return false;
}