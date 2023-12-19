#include "main.h"
#include "buzzer.h"
#include "debug.h"
#include "mytimer.h"
#include "repeater.h"
#include "segment.h"
#include "rs485.h"
#include "rs232c.h"
#include "flashrom.h"

_Bool	flg_toggle = 0;
_Bool	flg_irtoggle = 0;
_Bool	flg_ir_send = 0;

_Bool	flg_doorkey = 0;
_Bool	flg_exitkey = 0;

_Bool	flg_capture_done = 0;
_Bool	flg_irsend_done = 0;
_Bool	flg_bat_toggle = 0;

_Bool	flg_di1_read = 0;
_Bool	flg_di2_read = 0;

uint8_t callback_1ms = 0;
uint8_t callback_10ms = 0;
uint8_t callback_100ms = 0;
uint8_t callback_500ms = 0;
uint8_t callback_1sec = 0;

uint8_t  menu_key[KEY_COUNT] = {0,};
uint8_t  keyin_buf[KEY_COUNT][3];
uint8_t  key_pushed[KEY_COUNT] = {0,};
uint8_t  key_continued[KEY_COUNT] = {0,};
uint8_t  key_idx = 0;
uint8_t  key_cont_count[KEY_COUNT] = {0,};


uint32_t ether_led_timeout = 0;

void Key_Scan(void);
//======================================================================
extern	IWDG_HandleTypeDef hiwdg;
extern	TE2PDataRec 	Flashdatarec;

extern	_Bool 		urx1_comp;
extern	uint16_t	urx1_tout;				// For RS485
extern	_Bool 		urx2_comp;
extern	uint16_t	urx2_tout;				// For Debug
extern	_Bool 		urx3_comp;
extern	uint16_t	urx3_tout;				// For	Wifi
extern	_Bool 		urx4_comp;
extern	uint16_t	urx4_tout;

extern	_Bool		flg_w5100s_init_ok;
extern	_Bool		flg_dns_init_ok;
extern	_Bool		flg_ethernet_connected;

extern	uint8_t 	ledoff_timeout;
extern	uint8_t 	key_timeout;
extern	uint16_t 	dio_in1_timeout;

extern	uint16_t	wifi_timeout;
extern	uint16_t	connect_timeout;
extern	uint16_t	ether_timeout;
extern	uint16_t	ethmod_timeout;
extern	uint16_t	rssi_timeout;
extern	uint16_t	rtu_int_timeout;

extern	uint16_t	rtu_int_timeout_485;


extern	_Bool		flg_reset_enable;
extern	uint32_t 	test_timeout;
extern	uint16_t	eth_req_int_timeout;

extern	uint16_t	send_232int_timeout;
extern	uint16_t	send_485int_timeout;

//======================================================================
void Systic_Callback_Proc(void)
{
	callback_1ms++;
	
	Segment_Disp();
	
	if(dio_in1_timeout)
		dio_in1_timeout--;

	// For RS485
	if(urx1_tout)
	{
		urx1_tout--;
		if(urx1_tout == 0)
			urx1_comp = 1;
	}

	// For Debug
	if(urx2_tout)
	{
		urx2_tout--;
		if(urx2_tout == 0)
			urx2_comp = 1;
	}

	// For Wifi
	if(urx3_tout)
	{
		urx3_tout--;
		if(urx3_tout == 0)
			urx3_comp = 1;
	}
	
	// For RS232
	if(urx4_tout)
	{
		urx4_tout--;
		if(urx4_tout == 0)
			urx4_comp = 1;
	}

	if(callback_1ms >= 10)
	{
		callback_1ms = 0;
		callback_10ms++;
		
		if(ether_led_timeout)
			ether_led_timeout--;
		
		if(key_timeout)
			key_timeout--;
		
		if(test_timeout)
			test_timeout--;
		
		Key_Scan();
		Buzzer_Sequence();          // Buzzer
		
		
		if(flg_reset_enable == 0)
			HAL_IWDG_Refresh(&hiwdg);
	}
}
//====================================================================================
void Timer_Proc(void)
{
	if(callback_10ms >= 10)
	{
		callback_10ms = 0;
		callback_100ms++;
		callback_500ms++;
		
		if(wifi_timeout)
			wifi_timeout--;
		
		if(rssi_timeout)
			rssi_timeout--;
		
		if(eth_req_int_timeout)
			eth_req_int_timeout--;
		
		if(ethmod_timeout)
			ethmod_timeout--;

		if(rtu_int_timeout)
			rtu_int_timeout--;

		if(rtu_int_timeout_485)
			rtu_int_timeout_485--;
		
		if(send_485int_timeout)
			send_485int_timeout--;
		
		if(send_232int_timeout)
			send_232int_timeout--;
	}

	if(callback_500ms == 5)
	{
		callback_500ms = 0;
		flg_toggle = !flg_toggle;
		HAL_GPIO_TogglePin(OPER_LED0_GPIO_Port, OPER_LED0_Pin);
	}

	if(callback_100ms >= 10)
	{
		callback_100ms = 0;
		callback_1sec++;
		
		if(connect_timeout)
			connect_timeout--;
		
		if(ether_timeout)
			ether_timeout--;
	}
	
	if(callback_1sec >= 60)
	{
		callback_1sec = 0;
		
	}
}

//----------------------------------------------------------------------------
void Key_Scan(void)
{
    uint8_t i;

	keyin_buf[MENU_KEY][key_idx] = MENU_BUTTON;
	keyin_buf[UP_KEY][key_idx] = UP_BUTTON;
	keyin_buf[DOWN_KEY][key_idx] = DOWN_BUTTON;
	keyin_buf[ENTER_KEY][key_idx] = ENT_BUTTON;

    key_idx++;
	if(key_idx >= 3)
		key_idx = 0;

    for(i=0; i < KEY_COUNT; i++)
    {
        if((keyin_buf[i][0] == 0) && (keyin_buf[i][1] == 0) && (keyin_buf[i][2] == 0))         // Keyï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?
        {
            if(key_pushed[i] == 0)                        // Key Pushed Flag Ã³ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?
            {
                key_pushed[i] = 1;
                menu_key[i] = 1;
                key_cont_count[i] = 0;
            }
            else
            {
                if(key_continued[i] == 0)
                {
                    if (key_cont_count[i] < 50)                                // 500ms
                        key_cont_count[i]++;
                    else
                    {
                        key_continued[i] = 1;
                        key_cont_count[i] = 0;
                    }
                }
            }
        }
        else
        {
			key_pushed[i] = 0;
            key_continued[i] = 0;
            key_cont_count[i] = 0;
        }
    }
}

