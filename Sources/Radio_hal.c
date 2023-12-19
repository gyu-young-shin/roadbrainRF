#include "main.h"
#include "radio.h"
#include "radio_hal.h"


extern	SPI_HandleTypeDef hspi2;
//===========================================================
void radio_hal_AssertShutdown(void)
{
	PWRDN_ON;
}

void radio_hal_DeassertShutdown(void)
{
	PWRDN_OFF;
}

void radio_hal_ClearNsel(void)
{
	uint32_t i;
	
	for(i=0; i<10; i++) __nop();
    RF_NSEL_RESET;
	for(i=0; i<10; i++) __nop();
}

void radio_hal_SetNsel(void)
{
	uint32_t i;
	
	for(i=0; i<10; i++) __nop();
    RF_NSEL_SET;
	for(i=0; i<10; i++) __nop();
}

GPIO_PinState radio_hal_NirqLevel(void)
{
    return SPI_NIRQ;
}

void radio_hal_SpiWriteByte(uint8_t byteToWrite)
{
	HAL_SPI_Transmit(&hspi2, &byteToWrite, 1, 500);
}

uint8_t radio_hal_SpiReadByte(void)
{
	uint8_t	ret_var;
	
	HAL_SPI_Receive(&hspi2, &ret_var, 1, 500);
	
	return ret_var;
}

void radio_hal_SpiWriteData(uint8_t byteCount, uint8_t *pData)
{
	HAL_SPI_Transmit(&hspi2, pData, byteCount, 500);
}

void radio_hal_SpiReadData(uint8_t byteCount, uint8_t *pData)
{
	HAL_SPI_Receive(&hspi2, pData, byteCount, 500);
}

GPIO_PinState radio_hal_Gpio0Level(void)
{
	return RF_GPIO0;
}

GPIO_PinState radio_hal_Gpio1Level(void)
{
	return RF_GPIO1;
}

GPIO_PinState radio_hal_Gpio2Level(void)
{
	return RF_GPIO2;
}

GPIO_PinState radio_hal_Gpio3Level(void)
{
	return RF_GPIO3;
}
