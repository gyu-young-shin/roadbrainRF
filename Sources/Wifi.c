#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include "wifi.h"
#include "rs485.h"
#include "repeater.h"
#include "flashrom.h"
#include "debug.h"

_Bool 		wifi_init_ok = 0;
_Bool 		wifi_tcpinit_ok = 0;
_Bool		wifi_soket_connected = 0;

_Bool 		urx3_comp;
uint8_t		urx3_buf[URX3_LEN];
uint8_t		utx3_buf[UTX3_LEN];
uint8_t		utx3_tmp[UTX3_LEN];

uint8_t		tx3_restart = 1;
uint16_t 	urx3_count = 0;
uint16_t	p3_out = 0;
uint16_t	p3_in = 0;
uint16_t	urx3_tout = 0;

uint8_t		rssi_str[100] = {0,};
uint8_t		rssi_step = 0;
uint8_t		step_num = 0;
uint8_t		session_id = 0;
uint8_t		retry_count = 0;
uint8_t		packet_buf[URX3_LEN];
uint16_t	received_data_len = 0;
uint16_t	wifi_timeout = 0;
uint16_t	connect_timeout = 0;
uint16_t	rssi_timeout = 0;

uint8_t		wifi_send_buf[WBUF_COUNT][WBUF_LEN];
uint16_t	wifi_send_len[WBUF_COUNT] = {0,};
uint16_t	wifi_rd_indx = 0;
uint16_t	wifi_wr_indx = 0;

uint32_t 	prev_pulse_in1_count = 0;
//==========================================================================================================
extern UART_HandleTypeDef huart3;
extern TE2PDataRec 	Flashdatarec;

extern _Bool 		flg_E2p_write;
extern uint32_t 	pulse_in1_count;
//==========================================================================================================
void USART3_IRQ_Function(void)				// For wifi module
{	
	uint8_t data_char;
	
	uint32_t isrflags   = READ_REG(USART3->SR);
	uint32_t cr1its     = READ_REG(USART3->CR1);
	uint32_t cr3its     = READ_REG(USART3->CR3);

	uint32_t errorflags;

	/* If no error occurs */
	errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
	if(errorflags == RESET)
	{
		if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
		{
			data_char = USART3->DR & 0x00FF;
		
			if((!urx3_comp) && (urx3_count < URX3_LEN))
			{
				urx3_buf[urx3_count++] = data_char;
				urx3_tout = 3;
			}
		}
		
		// Transmission
		if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))		
		{
			if(p3_out != p3_in) 							
			{
				USART3->DR = utx3_buf[p3_out];
				p3_out++;
				p3_out %= UTX3_LEN; 
				tx3_restart = 0;
			}
			else 
			{
				tx3_restart = 1;
				CLEAR_BIT(USART3->CR1, USART_CR1_TXEIE);
			}
		}
	}
	else
	{
		__HAL_UART_CLEAR_PEFLAG(&huart3);
		__HAL_UART_CLEAR_FEFLAG(&huart3);
		__HAL_UART_CLEAR_NEFLAG(&huart3);
		__HAL_UART_CLEAR_OREFLAG(&huart3);
	}
}
//------------------------------------------------------------------------------------------
void Wifi_proc(void) 
{
	uint16_t i;
	
	if(urx3_comp)
	{
		for(i=0; i<urx3_count; i++)
			SendChar(urx3_buf[i]);
	}
	
	if(wifi_init_ok == 0)
	{
		Init_Station_mode();
	}
	else
	{
		if(wifi_tcpinit_ok == 0)
		{
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"ERROR") != NULL)
				{
					mprintf3("AT+CIPCLOSE\n");			// TCP CLOSE
					HAL_Delay(300);
					wifi_init_ok = 0;
					step_num = 0;
					
					CONNECTLED_OFF;
					
					memset(urx3_buf, 0, sizeof(urx3_buf));
					urx3_count = 0;
					urx3_comp = 0;
					
					return;
				}
				else if(strstr((const char *)urx3_buf,"STA_DISCONNECTED") != NULL)
				{
					wifi_init_ok = 0;
					step_num = 0;
					CONNECTLED_OFF;
					
					memset(urx3_buf, 0, sizeof(urx3_buf));
					urx3_count = 0;
					urx3_comp = 0;
					return;
				}
			}
			
			if(connect_timeout == 0)
				Connect_Tcp_Server();
			else
			{
				if(rssi_timeout == 0)
					Get_Signal_Strength();
			}
		}
		else
		{
			Make_packet();
			Tcp_Data_Send();
		}
	}
	
//	if(urx3_comp)
//	{
//		urx3_count = 0;
//		urx3_comp = 0;
//	}

}
//-----------------------------------------------------------------------------------------------------
void Init_Init_mode(void)
{
	switch(step_num)
	{
		case 0:
			WIZ_RESET_LOW;						// H/W Reset
			wifi_timeout = 2;					// 200ms;
			step_num = 1;
			break;
		case 1:									
			if(wifi_timeout == 0)
			{
				WIZ_RESET_HIGH;					// H/W Reset
				wifi_timeout = 5;				// 500ms;
				step_num = 2;
			}
			break;
		case 2:
			wifi_init_ok = 1;
			step_num = 0;
			wifi_timeout = 10;
			mprintf("wifi reset ok~~!!\n");
			break;
		default:
			break;
	}
}
//-----------------------------------------------------------------------------------------------------
void Init_Station_mode(void)
{
	switch(step_num)
	{
		case 0:
			WIFILED_OFF;
			WIZ_RESET_LOW;						// H/W Reset
			wifi_timeout = 5;					// 200ms;
			step_num = 1;

			urx3_count = 0;
			urx3_comp = 0;
			break;
		case 1:									
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				WIZ_RESET_HIGH;					// H/W Reset
				wifi_timeout = 5;				// 500ms;
				step_num = 2;
				retry_count = 0;
			}
			break;
		case 2:					
			urx3_count = 0;
			urx3_comp = 0;
			
			if(wifi_timeout == 0)
			{
				mprintf3("ATE0\n");				// Echo Disable
				mprintf("ATE0\n");				// Echo Disable
				wifi_timeout = 2;				// 200ms;
				step_num = 50;
			}
			break;
		case 50:
			urx3_count = 0;
			urx3_comp = 0;
			
			if(wifi_timeout == 0)
			{
				mprintf3("AT+GMR\n");				// Echo Disable
				mprintf("AT+GMR\n");				// Echo Disable
				wifi_timeout = 2;				// 200ms;
				step_num = 3;
			}
			break;
		case 3:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"OK") != NULL)
				{
					wifi_timeout = 1;				// 100ms;
					step_num = 4;
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 2;
				retry_count++;
				if(retry_count >= 3)
				{
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 4:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				mprintf3("AT+CWMODE_CUR=1\n");			// Station Mode 설정
				mprintf("AT+CWMODE_CUR=1\n");			// Station Mode 설정
				wifi_timeout = 2;						// 200ms;
				step_num = 5;
			}
			break;
		case 5:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"OK") != NULL)
				{
					wifi_timeout = 1;				// 100ms;

					if(Flashdatarec.e2p_set_wifimac == 0)
						step_num = 20;
					else
						step_num = 6;
					
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 4;
				retry_count++;
				if(retry_count >= 3)
				{
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 6:
			// AT+CWDHCP_CUR=<mode>,<en>
			// <mode>
			//	0: WizFi360 SoftAP 설정
			//	1: WizFi360 Station 설정
			//	2: SoftAP, Station 모두 설정
			// <en>
			//  0: Disable DHCP
			//  1: Enable DHCP		
			urx3_count = 0;
			urx3_comp = 0;
		
			if(wifi_timeout == 0)
			{
				mprintf3("AT+CWDHCP_CUR=1,1\n");		// DHCP를 활성화한다.
				mprintf("AT+CWDHCP_CUR=1,1\n");		// DHCP를 활성화한다.
				wifi_timeout = 2;						// 200ms;
				step_num = 7;
			}
			break;
		case 7:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"OK") != NULL)
				{
					wifi_timeout = 1;				// 100ms;
					step_num = 10;
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 6;
				retry_count++;
				if(retry_count >= 3)
				{
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 8:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				mprintf3("AT+CWLAP\n");					// 사용가능한 AP 리스트를 확인한다.
				mprintf("AT+CWLAP\n");					// 사용가능한 AP 리스트를 확인한다.
				wifi_timeout = 50;						// 3SEC;
				step_num = 9;
			}
			break;
		case 9:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf, (const char *)Flashdatarec.e2p_station_ssid) != NULL)
				{
					wifi_timeout = 1;				// 100ms;
					step_num = 10;
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 8;
				retry_count++;
				if(retry_count >= 3)
				{
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 10:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				mprintf3("AT+CWJAP_CUR=\"%s\",\"%s\"\n", Flashdatarec.e2p_station_ssid, Flashdatarec.e2p_station_pawd);	// 사용 가능한 AP에 연결한다.
				mprintf("AT+CWJAP_CUR=\"%s\",\"%s\"\n", Flashdatarec.e2p_station_ssid, Flashdatarec.e2p_station_pawd);	// 사용 가능한 AP에 연결한다.
				wifi_timeout = 130;						// 8SEC;
				step_num = 11;
			}
			break;
		case 11:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf, "WIFI GOT IP") != NULL)
				{
					wifi_timeout = 2;				// 100ms;
					step_num = 12;
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				wifi_timeout = 10;
				step_num = 10;
				retry_count++;
				if(retry_count >= 3)
				{
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 12:
			wifi_init_ok = 1;
			wifi_tcpinit_ok = 0;
			step_num = 0;
			wifi_timeout = 10;
			WIFILED_ON;
			retry_count = 0;
			mprintf("wifi station mode init ok~~!!\n");
			urx3_count = 0;
			urx3_comp = 0;
			break;
		case 20:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				mprintf3("AT+CIPSTAMAC_DEF=\"%02X:%02X:%02X:%02X:%02X:%02X\"\n", Flashdatarec.e2p_mac_wifi[0], \
						Flashdatarec.e2p_mac_wifi[1],Flashdatarec.e2p_mac_wifi[2],Flashdatarec.e2p_mac_wifi[3],
						Flashdatarec.e2p_mac_wifi[4],Flashdatarec.e2p_mac_wifi[5]);			// Station Mode MAC 설정
				mprintf("AT+CIPSTAMAC_DEF=\"%02X:%02X:%02X:%02X:%02X:%02X\"\n", Flashdatarec.e2p_mac_wifi[0], \
						Flashdatarec.e2p_mac_wifi[1],Flashdatarec.e2p_mac_wifi[2],Flashdatarec.e2p_mac_wifi[3],
						Flashdatarec.e2p_mac_wifi[4],Flashdatarec.e2p_mac_wifi[5]);			// Station Mode MAC 설정
				wifi_timeout = 2;						// 200ms;
				step_num = 21;
			}
			break;
		case 21:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"OK") != NULL)
				{
					wifi_timeout = 1;				// 100ms;
					Flashdatarec.e2p_set_wifimac = 1;
					flg_E2p_write = 1;
					step_num = 6;
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 20;
				wifi_timeout = 1;				// 100ms;
				
				retry_count++;
				if(retry_count >= 3)
				{
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		default:
			break;
	}
}
//-------------------------------------------------------------------------------------------------
void Get_Signal_Strength(void)
{
	uint8_t i;
	char *pstr;
	
	switch(rssi_step)
	{
		case 0:
			mprintf3("AT+CWJAP_CUR?\n");	// WizFi360 Station이 연결한 AP를 확인한다.
			wifi_timeout = 2;					// 200ms;
			rssi_step = 1;
			break;
		case 1:
			if(urx3_comp)
			{
				urx3_buf[urx3_count] = 0;
				pstr = strtok((char *)urx3_buf,",");
				
				i = 1;
				while(pstr != NULL)
				{
					i++;
					pstr = strtok(NULL,",");
					if(i == 4)
					{
						if(strlen(pstr) >= 2)
						{
							for(i=0; i<strlen(pstr); i++)
							{
								if((pstr[i] != '-') && ((pstr[i] <= '0') || (pstr[i] >= '9'))) 
								{
									pstr[i] = 0;
									break;
								}
							}
							strcpy((char *)rssi_str, pstr);
						}
						break;
					}
				}
				

				rssi_timeout = 10;			// 1sec
				urx3_comp = 0;
				urx3_count = 0;
				rssi_step = 0;
			}
			else if(wifi_timeout == 0)
			{
				rssi_timeout = 10;			// 1sec
				urx3_comp = 0;
				urx3_count = 0;
				rssi_step = 0;
			}
			break;
		default:
			break;
		
	}
}
//------------------------------------------------------------------------------------------
void Connect_Tcp_Server(void)
{
	switch(step_num)
	{
		case 0:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				CONNECTLED_OFF;
				
				mprintf3("AT+CIPSTART=\"TCP\",\"%d.%d.%d.%d\",%d\n", 
						Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
						Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3],
						Flashdatarec.e2p_server_port);			// TCP Server에 연결한다. Request


				mprintf("AT+CIPSTART=\"TCP\",\"%d.%d.%d.%d\",%d\n", 
						Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
						Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3],
						Flashdatarec.e2p_server_port);			// TCP Server에 연결한다. Request
				
				wifi_timeout = 250;						// 25sec
				step_num = 1;
			}
			break;
		case 1:		
			if(urx3_comp)
			{
				if((strstr((const char *)urx3_buf,"CONNECT") != NULL) ||
					(strstr((const char *)urx3_buf,"OK") != NULL) || 
					(strstr((const char *)urx3_buf,"ALREADY CONNECTED") != NULL))
				{
					wifi_timeout = 3;				// 100ms;
					step_num = 2;
					retry_count = 0;
				}
				
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 0;
				retry_count++;
				if(retry_count >= 3)
				{
					wifi_init_ok = 0;
					retry_count = 0;
					step_num = 0;
				}
			}
			break;
		case 2:		
			wifi_tcpinit_ok = 1;
			retry_count = 0;
			step_num = 0;
			urx3_count = 0;
			urx3_comp = 0;
			CONNECTLED_ON;
			break;
		default:
			break;
	}
}
//------------------------------------------------------------------------------------------	
void Tcp_Data_Send(void)
{
	if((strstr((const char *)urx3_buf,"ERROR") != NULL) &&
		(urx3_comp == 1))
	{
		mprintf3("AT+CIPCLOSE\n");			// TCP CLOSE
		HAL_Delay(300);
		
		memset(urx3_buf, 0, sizeof(urx3_buf));
		step_num = 0;
		wifi_tcpinit_ok = 0;
		CONNECTLED_OFF;
		urx3_count = 0;
		urx3_comp = 0;
		
		return;
	}
	else if((strstr((const char *)urx3_buf,"STA_DISCONNECTED") != NULL) &&
			(urx3_comp == 1))		
	{
		wifi_init_ok = 0;
		step_num = 0;
		CONNECTLED_OFF;
		
		memset(urx3_buf, 0, sizeof(urx3_buf));
		urx3_count = 0;
		urx3_comp = 0;
		
		return;
	}
	else if((strstr((const char *)urx3_buf,"CLOSED") != NULL) &&
			(urx3_comp == 1))
	{
		mprintf("socket closed...");
		CONNECTLED_OFF;
		wifi_timeout = 3;				// 100ms;
		step_num = 0;
		wifi_tcpinit_ok = 0;
		retry_count = 0;
				
		memset(urx3_buf, 0, sizeof(urx3_buf));
		urx3_count = 0;
		urx3_comp = 0;
		
		return;
	}
	
	switch(step_num)
	{
		case 0:
			if(wifi_rd_indx != wifi_wr_indx)
				step_num = 1;
			
			urx3_count = 0;
			urx3_comp = 0;
			break;
		case 1:
			wifi_timeout = 1;				// 100ms;
			step_num = 2;
			retry_count = 0;
		
			urx3_count = 0;
			urx3_comp = 0;
			break;
		case 2:
			urx3_count = 0;
			urx3_comp = 0;
			
			if(wifi_timeout == 0)
			{
				memset(urx3_buf, 0, sizeof(urx3_buf));

				mprintf3("AT+CIPBUFRESET\n");			// send buffer reset
				mprintf("AT+CIPBUFRESET\n");			// send buffer reset
				wifi_timeout = 5;						// 200ms;
				step_num = 3;
			}
			break;
		case 3:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"OK") != NULL)
				{
					wifi_timeout = 3;				// 100ms;
					step_num = 4;
					retry_count = 0;
				}
				
				memset(urx3_buf, 0, sizeof(urx3_buf));
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				wifi_timeout = 5;
				step_num = 2;
				
				retry_count++;
				if(retry_count >= 3)
				{
					wifi_init_ok = 0;
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 4:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				memset(urx3_buf, 0, sizeof(urx3_buf));
				
				mprintf3("AT+CIPSENDBUF=%d\n", wifi_send_len[wifi_rd_indx]);	// SEND SIZE
				mprintf("AT+CIPSENDBUF=%d\n", wifi_send_len[wifi_rd_indx]);			// SEND SIZE
				wifi_timeout = 5;						// 200ms;
				step_num = 5;
			}
			break;
		case 5:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"OK") != NULL)
				{
					wifi_timeout = 3;				// 100ms;
					step_num = 6;
					retry_count = 0;
				}
				
				memset(urx3_buf, 0, sizeof(urx3_buf));
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				wifi_timeout = 7;
				step_num = 4;
				
				retry_count++;
				if(retry_count >= 3)
				{
					wifi_init_ok = 0;
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		case 6:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				memset(urx3_buf, 0, sizeof(urx3_buf));
				Send_packet();
				
				wifi_timeout = 5;
				step_num = 7;
			}
			break;
		case 7:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"SEND OK") != NULL)
				{
					wifi_timeout = 5;				// 100ms;
					step_num = 30;
				}
				
				memset(urx3_buf, 0, sizeof(urx3_buf));
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				wifi_timeout = 3;
				step_num = 30;
			}
			break;
		case 30:
			step_num = 0;
			break;
		case 8:
			urx3_count = 0;
			urx3_comp = 0;

			if(wifi_timeout == 0)
			{
				memset(urx3_buf, 0, sizeof(urx3_buf));
				
				mprintf3("AT+CIPCLOSE\n");			// TCP CLOSE
				mprintf("AT+CIPCLOSE\n");			// TCP CLOSE
				wifi_timeout = 5;						// 200ms;
				step_num = 8;
			}
			break;
		case 9:
			if(urx3_comp)
			{
				if(strstr((const char *)urx3_buf,"CLOSED") != NULL)
				{
					CONNECTLED_OFF;
					wifi_timeout = 3;				// 100ms;
					step_num = 0;
					wifi_tcpinit_ok = 0;
					connect_timeout = 30;			// 30 sec
					//connect_timeout = 10;			// 30 sec
					retry_count = 0;
					mprintf("socket closed... step 9");
				}
				
				memset(urx3_buf, 0, sizeof(urx3_buf));
				urx3_count = 0;
				urx3_comp = 0;
			}
			else if(wifi_timeout == 0)
			{
				urx3_count = 0;
				urx3_comp = 0;
				step_num = 8;
				
				retry_count++;
				if(retry_count >= 3)
				{
					wifi_init_ok = 0;
					step_num = 0;
					retry_count = 0;
				}
			}
			break;
		default:
			break;
	}
}
//------------------------------------------------------------------------------------------	
void Send_packet(void)
{
	uint16_t i;
	
	for(i=0; i<wifi_send_len[wifi_rd_indx]; i++)
		SendChar3(wifi_send_buf[wifi_rd_indx][i]);
	
	wifi_rd_indx++;
	wifi_rd_indx %= WBUF_COUNT;
}
//------------------------------------------------------------------------------------------	
void Make_packet(void)
{
	uint16_t wifi_tmp_indx;
	
	wifi_tmp_indx = wifi_wr_indx + 1;
	wifi_tmp_indx %= WBUF_COUNT;
	
	if((wifi_rd_indx != wifi_tmp_indx) && (prev_pulse_in1_count != pulse_in1_count))
	{
		prev_pulse_in1_count = pulse_in1_count;
		
		wifi_send_buf[wifi_wr_indx][0] = 0x00;
		wifi_send_buf[wifi_wr_indx][1] = 0xBA;
		wifi_send_buf[wifi_wr_indx][2] = (Flashdatarec.e2p_id & 0xFF000000) >> 24;
		wifi_send_buf[wifi_wr_indx][3] = (Flashdatarec.e2p_id & 0x00FF0000) >> 16;
		wifi_send_buf[wifi_wr_indx][4] = (Flashdatarec.e2p_id & 0x0000FF00) >> 8;
		wifi_send_buf[wifi_wr_indx][5] = (Flashdatarec.e2p_id & 0x000000FF);
		wifi_send_buf[wifi_wr_indx][6] = 0x01;
		wifi_send_buf[wifi_wr_indx][7] = 0x00;
		wifi_send_buf[wifi_wr_indx][8] = 0x04;
		wifi_send_buf[wifi_wr_indx][9] = (pulse_in1_count & 0xFF000000) >> 24;
		wifi_send_buf[wifi_wr_indx][10] = (pulse_in1_count & 0x00FF0000) >> 16;
		wifi_send_buf[wifi_wr_indx][11] = (pulse_in1_count & 0x0000FF00) >> 8;
		wifi_send_buf[wifi_wr_indx][12] = (pulse_in1_count & 0x000000FF);
		
		wifi_send_len[wifi_wr_indx] = 13;
		
		wifi_wr_indx++;
		wifi_wr_indx %= WBUF_COUNT;
	}
}
//------------------------------------------------------------------------------------------	
void Send_wifi_packet(uint16_t send_len)
{
//	uint16_t i;
//	
//	urx3_count = 0;
//	urx3_comp = 0;

//	mprintf3("AT+CIPSENDBUF=%d,%d\n", session_id, send_len);
//	
//	i = 0;
//	while(urx3_comp == 0)
//	{
//		HAL_Delay(1);
//		i++;
//		if(i>500)
//			return;
//	}
//	
//	for(i=0; i<send_len; i++)
//		SendChar3(wifi_send_buf[i]);
}
//------------------------------------------------------------------------------------------	
void SendChar3(uint8_t send_c) 
{
	uint16_t pin_temp;

	pin_temp = p3_in + 1;
	pin_temp %= UTX3_LEN;

	while(pin_temp == p3_out)
	{
		HAL_Delay(1);
	}
	
	utx3_buf[p3_in] = send_c;
	p3_in = pin_temp;

	if(tx3_restart) 
	{
		tx3_restart = 0;
		SET_BIT(USART3->CR1, USART_CR1_TXEIE);
	}
}

//------------------------------------------------------------------------------------------
void mprintf3(const char *format, ...)
{
    uint16_t i;
	__va_list	arglist;

	va_start(arglist, format);
    vsprintf((char *)utx3_tmp, format, arglist);
	va_end(arglist);
	i = 0;
	
	while(utx3_tmp[i] && (i < UTX3_LEN))	// Null 문자가 아닌동안 개행문자를 개행문자 + Carrige Return 으로 변환
	{
		if(utx3_tmp[i] == '\n')
			SendChar3('\r');
		SendChar3(utx3_tmp[i++]);
	}
}

//------------------------------------------------------------------------------------------

