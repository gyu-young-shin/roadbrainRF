#ifndef _RADIO_HAL_H_
#define _RADIO_HAL_H_
#include "main.h"

void radio_hal_AssertShutdown(void);
void radio_hal_DeassertShutdown(void);
void radio_hal_ClearNsel(void);
void radio_hal_SetNsel(void);
GPIO_PinState radio_hal_NirqLevel(void);

void radio_hal_SpiWriteByte(uint8_t byteToWrite);
uint8_t radio_hal_SpiReadByte(void);

void radio_hal_SpiWriteData(uint8_t byteCount, uint8_t* pData);
void radio_hal_SpiReadData(uint8_t byteCount, uint8_t* pData);

GPIO_PinState radio_hal_Gpio0Level(void);
GPIO_PinState radio_hal_Gpio1Level(void);
GPIO_PinState radio_hal_Gpio2Level(void);
GPIO_PinState radio_hal_Gpio3Level(void);

#endif //_RADIO_HAL_H_
