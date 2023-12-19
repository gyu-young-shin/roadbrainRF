#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include "debug.h"
#include "repeater.h"
#include "wifi.h"
#include "flashrom.h"
#include "buzzer.h"
#include "test.h"
#include "segment.h"
#include "rs485.h"
#include "rs232c.h"

_Bool 	flg_testmode = 0;
_Bool 	flg_wifi_test = 0;

uint8_t	flg_startout = 0;
uint8_t	test_num = 0;
uint8_t	menu_num = 0;
uint8_t	led_num = 0;
uint8_t	led_cnt = 0;
uint8_t	seg_num = 0;


uint32_t test_timeout = 0;
//===========================================================================
extern	uint8_t  	menu_key[KEY_COUNT];
extern	uint8_t 	segdata[4];

extern	_Bool 		urx1_comp;
extern	uint8_t		urx1_buf[URX1_LEN];
extern	uint16_t 	urx1_count;
extern	_Bool 		urx4_comp;
extern	uint8_t		urx4_buf[URX4_LEN];
extern	uint16_t 	urx4_count;

extern	_Bool 		wifi_init_ok;
extern	_Bool 		wifi_tcpinit_ok;
extern	_Bool		wifi_soket_connected;
//===========================================================================
void TestMode_Proc(void)
{
	if(menu_key[MENU_KEY])					// 출력 테스트 
	{
		AllOff_Output();
		if(flg_startout)
			flg_startout = 0;
		else
		{
			test_num = 0;
			seg_num = 0;
			led_num = 1;
			flg_startout = 1;
		}
		
		Play_Buzzer(1);
		
		menu_key[MENU_KEY] = 0;
		menu_num = 0;
	}
	else if(menu_key[UP_KEY])					// 입력 테스트
	{
		Play_Buzzer(1);
		
		if(flg_startout)
		{
			flg_startout = 0;
			AllOff_Output();
		}
		menu_key[UP_KEY] = 0;
		menu_num = 1;
	}
	else if(menu_key[DOWN_KEY])					// RS232C, RS485 테스트
	{
		Play_Buzzer(1);
		
		if(flg_startout)
		{
			flg_startout = 0;
			AllOff_Output();
		}
		test_num = 0;
		menu_key[DOWN_KEY] = 0;
		menu_num = 2;
	}
	else if(menu_key[ENTER_KEY])				// Wifi 접속 테스트
	{
		AllOff_Output();

		if(flg_startout)
			flg_startout = 0;
		else
		{
			test_num = 0;
			flg_startout = 1;
			segdata[0] = FND_r;
			segdata[1] = FND_A;
			segdata[2] = FND_d;
			segdata[3] = FND_1;
		}

		menu_key[ENTER_KEY] = 0;
		menu_num = 3;
	}
	
	switch(menu_num)
	{
		case 0:						
			if(flg_startout)
				Output_Test();
			else
			{
				segdata[0] = FND_0;
				segdata[1] = FND_u;
				segdata[2] = FND_t;
				segdata[3] = 0xFF;
			}
			flg_wifi_test = 0;
			break;
		case 1:						
			Disp_Input();
			flg_wifi_test = 0;
			break;
		case 2:						
			Comm_Test();
			flg_wifi_test = 0;
			break;
		case 3:
			if(flg_startout)
				Wifi_Test();
			else
			{
				flg_wifi_test = 0;
			}
			break;
		default:
			break;
	}
}
//---------------------------------------------------------------------
void Output_Test(void)
{
	switch(test_num)
	{
		case 0:
			if(test_timeout == 0)
			{
				segdata[seg_num] &= ~led_num;		
				if(led_num == 0xFF)
				{
					if(seg_num == 3)
						test_num++;
					else
					{
						seg_num++;
						led_num = 1;
					}
				}
				else
				{
					led_num <<= 1;
					led_num |= 1;
				}
				test_timeout = 10;
			}
			break;
		case 1:
			if(test_timeout == 0)
			{
				ERRORLED_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		case 2:
			if(test_timeout == 0)
			{
				ETHLED_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		case 3:
			if(test_timeout == 0)
			{
				WIFILED_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		case 4:
			if(test_timeout == 0)
			{
				CONNECTLED_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		case 5:
			if(test_timeout == 0)
			{
				DO_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		case 6:
			if(test_timeout == 0)
			{
				DO1_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		case 7:
			if(test_timeout == 0)
			{
				DO2_ON;
				test_timeout = 20;
				test_num++;
			}
			break;
		default:
			break;
	}
}
//---------------------------------------------------------------------
void Disp_Input(void)
{
	segdata[0] = BLANK;
	segdata[2] = BLANK;
	
	if(DIO_IN1)
		segdata[1] = FND_1;
	else
		segdata[1] = FND_0;
	
	if(DIO_IN2)
		segdata[3] = FND_1;
	else
		segdata[3] = FND_0;
}
//---------------------------------------------------------------------
void Comm_Test(void)
{
	uint16_t i;
	
	segdata[0] = FND_C;
	segdata[1] = FND_o;
	segdata[2] = FND_n;
	segdata[3] = FND_n;
	
	if(urx1_comp)
	{
		for(i=0; i<urx1_count; i++)
			SendChar1(urx1_buf[i]);
		
		urx1_count = 0;
		urx1_comp = 0;
	}
	
	if(urx4_comp)
	{
		for(i=0; i<urx4_count; i++)
			SendChar4(urx4_buf[i]);
		
		urx4_count = 0;
		urx4_comp = 0;
	}
}
//---------------------------------------------------------------------
void Wifi_Test(void)
{
	switch(test_num)
	{
		case 0:
			wifi_init_ok = 0;
			wifi_tcpinit_ok = 0;
			wifi_soket_connected = 0;
			test_num  = 1;
			test_timeout = 20;
			break;
		case 1:
			flg_wifi_test = 1;
			test_num  = 2;
			break;
		default:
			break;
	}
}
//---------------------------------------------------------------------
void AllOff_Output(void)
{
	uint8_t i;
	
	for(i=0; i < 7; i++)		// All Sement and led Off
		segdata[i] = 0xFF;
	
	ERRORLED_OFF;
	ETHLED_OFF;
	WIFILED_OFF;
	CONNECTLED_OFF;

	DO_OFF;
	DO1_OFF;
	DO2_OFF;
}


