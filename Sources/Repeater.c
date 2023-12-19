#include "main.h"
#include <string.h>
#include "debug.h"
#include "repeater.h"
#include "buzzer.h"
#include "flashrom.h"
#include "segment.h"


_Bool 		flg_E2p_write = 0;
_Bool		flg_di1_event = 0;

uint8_t 	key_timeout = 0;

uint16_t 	dio_in1_timeout = 0;
uint32_t 	pulse_in1_count = 0;

//=============================================================================================
extern _Bool		flg_toggle;

extern uint8_t		rssi_str[10];
extern uint32_t		maintatin_timeout;

extern uint8_t  menu_key[KEY_COUNT];
extern uint8_t  keyin_buf[KEY_COUNT][3];
extern uint8_t  key_pushed[KEY_COUNT];
extern uint8_t  key_continued[KEY_COUNT];

extern __IO uint32_t RegularConvData_Tab[4];
extern TE2PDataRec 	Flashdatarec;
//=============================================================================================
void Repeater_Proc(void)
{
	if(flg_E2p_write)
	{
		flg_E2p_write = 0;
		FlashRom_WriteData();
	}
}
//-------------------------------------------------------------------------
void InputKey_Proc(void)
{
	menu_key[MENU_KEY] = 0;
	menu_key[ENTER_KEY] = 0;
	menu_key[UP_KEY] = 0;
	menu_key[DOWN_KEY] = 0;
}
//-------------------------------------------------------------------------
void Disp_Segment(void)
{
	
}
//-------------------------------------------------------------------------
void Output_Proc(void)
{
	
}
//-------------------------------------------------------------------------
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == EXIT_DI1_Pin)
	{
		if(dio_in1_timeout == 0)
		{
			dio_in1_timeout = 50;			// 50ms
			pulse_in1_count++;
			flg_di1_event = 1;
		}
	}
}
//-------------------------------------------------------------------------
// MCU 내부온도 측정
void Convert_Int_Temp(void)
{
	float vrefint, Tsense, Vsense;	// 3.3:4095=x:tab[0]
	
	vrefint = (float)(3.3 * RegularConvData_Tab[1]) / 4095;
	Vsense = (float)(vrefint * RegularConvData_Tab[0]) / 4095;
	mprintf("Vrefint = %.3fV\n", vrefint);

	Tsense = (1.43 - Vsense) / 0.0043 + 25; 
	mprintf("TEMP = %.3fC\n", Tsense);	
}
//-------------------------------------------------------------------------
void Disp_Version(void)
{
	Seg_data_set(0, 25, 0);	// R
	Seg_data_set(1, 11, 0);	// b
	Seg_data_set(2, VERSION_HIGH, 1);
	Seg_data_set(3, VERSION_LOW, 0);
	Segment_Disp();			// LED Driver로 Segment 및 LED Data를 전송
	HAL_Delay(500);
}

