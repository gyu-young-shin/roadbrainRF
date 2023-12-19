#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include "debug.h"
#include "rs485.h"
#include "repeater.h"
#include "w5100s_proc.h"
#include "flashrom.h"
#include "wifi.h"
#include "modbus.h"
_Bool		flg_next_rtu_485 = 1;

_Bool 		urx1_comp;
_Bool		flg_485_event = 0;

uint8_t		urx1_buf[URX1_LEN];
uint8_t		utx1_buf[UTX1_LEN];
uint8_t		utx1_tmp[UTX1_LEN];


uint8_t		tx1_restart = 1;
uint16_t 	urx1_count = 0;
uint16_t	p1_out = 0;
uint16_t	p1_in = 0;
uint16_t	urx1_tout;

uint16_t	rtu_int_timeout_485 = 0;

uint8_t		mod_rtu_indx_485 = 0;
uint8_t		rtu_retry_count_485 = 0;

uint8_t e2p_485_modbus_sel = 1;

//==========================================================================================================
extern UART_HandleTypeDef 	huart1;
extern TE2PDataRec 			Flashdatarec;
extern 	uint8_t		modbus_send_buf[64];

extern 	_Bool		crc_tab16_init;
extern 	uint16_t	crc_tab16[256];

extern	uint8_t		wifi_send_buf[WBUF_COUNT][WBUF_LEN];
extern	uint16_t	wifi_send_len[WBUF_COUNT];
extern	uint16_t	wifi_rd_indx;
extern	uint16_t	wifi_wr_indx;
extern	_Bool		flg_ethernet_connected;

extern _Bool		flg_mrtu_485_enable;

extern	_Bool		flg_mrs_enable;
extern	char 		eth_sendbuf[ETH_MAX_BUF_SIZE];
extern	uint16_t	send_485int_timeout;

//==========================================================================================================
void USART1_IRQ_Function(void)				// For rs485
{	
	uint8_t data_char;
	
	uint32_t isrflags   = READ_REG(USART1->SR);
	uint32_t cr1its     = READ_REG(USART1->CR1);
	uint32_t cr3its     = READ_REG(USART1->CR3);

	uint32_t errorflags;

	/* If no error occurs */
	errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
	if(errorflags == RESET)
	{
		if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
		{
			data_char = USART1->DR & 0x00FF;
		
			if((!urx1_comp) && (urx1_count < URX1_LEN))
			{
				urx1_buf[urx1_count++] = data_char;
				urx1_tout = 3;
			}
		}
		
		// Transmission
		if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))		
		{
			if(p1_out != p1_in) 							
			{
				USART1->DR = utx1_buf[p1_out];
				p1_out++;
				p1_out %= UTX1_LEN; 
				tx1_restart = 0;
			}
			else 
			{
				tx1_restart = 1;
				CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
				SET_BIT(USART1->CR1, USART_CR1_TCIE);
			}
		}
		
		if (((isrflags & USART_SR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))		
		{
			RS485_DE1_LOW;
			CLEAR_BIT(USART1->CR1, USART_CR1_TCIE);
		}
	}
	else
	{
		__HAL_UART_CLEAR_PEFLAG(&huart1);
		__HAL_UART_CLEAR_FEFLAG(&huart1);
		__HAL_UART_CLEAR_NEFLAG(&huart1);
		__HAL_UART_CLEAR_OREFLAG(&huart1);
	}
}
//------------------------------------------------------------------------------------------
void Rs485_proc(void) 
{	
	char 		tmp_buffer[20] = {0,};
	uint8_t		sel_ret;
	uint16_t	i, wifi_tmp_indx;
	if(Flashdatarec.e2p_modbus_sel == 3)		// RTU
	{
		if(urx1_comp)
		{
			if(Flashdatarec.e2p_modbus_sel == 3)		// RTU
				ModbusRtu_Recv_Proc_485();
			
			flg_next_rtu_485 = 1;

			urx1_comp = 0;
			urx1_count = 0;
		}
		
		
		if(flg_next_rtu_485)
		{
			flg_next_rtu_485 = 0;
			sel_ret = Next_Rtu_Index_485();
			rtu_retry_count_485 = 0;
		}
		
		if(sel_ret)
		{
			if(rtu_int_timeout_485 == 0)
			{
				rtu_retry_count_485++;
				if(rtu_retry_count_485 > 3)
				{
					rtu_retry_count_485 = 0;
					flg_next_rtu_485 = 1;
				}
				else
				{
					Modbus_Rtu_Proc_485();
					rtu_int_timeout_485 = Flashdatarec.e2p_mod_req_inter;
					//mprintf("rtu_int_timeout = %d\n", rtu_int_timeout);
				}
			}
		}
		else
			flg_next_rtu_485 = 1;
	}
	else if(Flashdatarec.e2p_modbus_sel == 4)			// RS-485
	{

		if(urx1_comp)
		{
			if(send_485int_timeout == 0)
			{
				wifi_tmp_indx = wifi_wr_indx + 1;
				wifi_tmp_indx %= WBUF_COUNT;
				
				if((wifi_rd_indx != wifi_tmp_indx) && (urx1_count <= (WBUF_LEN - 9)))
				{
					wifi_send_buf[wifi_wr_indx][0] = 0x00;
					wifi_send_buf[wifi_wr_indx][1] = 0xBA;
					wifi_send_buf[wifi_wr_indx][2] = (Flashdatarec.e2p_id & 0xFF000000) >> 24;
					wifi_send_buf[wifi_wr_indx][3] = (Flashdatarec.e2p_id & 0x00FF0000) >> 16;
					wifi_send_buf[wifi_wr_indx][4] = (Flashdatarec.e2p_id & 0x0000FF00) >> 8;
					wifi_send_buf[wifi_wr_indx][5] = (Flashdatarec.e2p_id & 0x000000FF);
					wifi_send_buf[wifi_wr_indx][6] = 0x10;
					wifi_send_buf[wifi_wr_indx][7] = (urx1_count & 0xFF00) >> 8;					
					wifi_send_buf[wifi_wr_indx][8] = (urx1_count & 0x00FF);
					memcpy(&wifi_send_buf[wifi_wr_indx][9], urx1_buf, urx1_count);
					wifi_send_len[wifi_wr_indx] = urx1_count + 9;
					
					wifi_wr_indx++;
					wifi_wr_indx %= WBUF_COUNT;
				}
				
				send_485int_timeout = Flashdatarec.e2p_485_int * 10;
			}
			
			if(flg_mrs_enable && flg_ethernet_connected)
			{
				sprintf(eth_sendbuf, "%d:[%s]\n\r", urx1_count, urx1_buf); 
				Send_Ethernet_Packet(eth_sendbuf); 
				
				for(i=0; i<urx1_count; i++)
				{
					sprintf(tmp_buffer,"%02X ", urx1_buf[i]);
					Send_Ethernet_Packet(tmp_buffer);
				}
				Send_Ethernet_Packet("\r\n");
			}
			
			urx1_count = 0;
			urx1_comp = 0;
		}
		flg_next_rtu_485 = 1;
	}
	else
	{
		urx1_comp = 0;
		urx1_count = 0;
		flg_next_rtu_485 = 1;
	}
}
//------------------------------------------------------------------------------------------
void SendChar1(uint8_t send_c) 
{
	uint16_t pin_temp;

	pin_temp = p1_in + 1;
	pin_temp %= UTX1_LEN;

	while(pin_temp == p1_out)
	{
		HAL_Delay(1);
	}
	
	utx1_buf[p1_in] = send_c;
	p1_in = pin_temp;

	if(tx1_restart) 
	{
		tx1_restart = 0;
		RS485_DE1_HIGH;
		SET_BIT(USART1->CR1, USART_CR1_TXEIE);
	}
}

//------------------------------------------------------------------------------------------
void mprintf1(const char *format, ...)
{
    uint16_t i;
	__va_list	arglist;

	va_start(arglist, format);
    vsprintf((char *)utx1_tmp, format, arglist);
	va_end(arglist);
	i = 0;
	
	while(utx1_tmp[i] && (i < UTX1_LEN))	// Null ¹®ÀÚ°¡ ¾Æ´Ñµ¿¾È °³Çà¹®ÀÚ¸¦ °³Çà¹®ÀÚ + Carrige Return À¸·Î º¯È¯
	{
		if(utx1_tmp[i] == '\n')
			SendChar1('\r');
		SendChar1(utx1_tmp[i++]);
	}
}
//------------------------------------------------------------------------------------------
uint8_t	Next_Rtu_Index_485(void)
{
	uint8_t i, ret_var = 0, tmp_indx;
	
	
	tmp_indx = mod_rtu_indx_485;

	tmp_indx++;
	tmp_indx %= CLIENT_COUNT;

	for(i=0; i<CLIENT_COUNT; i++)
	{
		
		if(Flashdatarec.e2p_mod_rtu[tmp_indx].mb_enable)
		{
			mod_rtu_indx_485 = tmp_indx;
			ret_var = 1;
			break;
		}
		
		tmp_indx++;
		tmp_indx %= CLIENT_COUNT;
	}
	
	return ret_var;
}
//------------------------------------------------------------------------------------------
void Modbus_Rtu_Proc_485(void)
{
	char 	tmp_buffer[20] = {0,};
	uint16_t	i, crc_16_modbus_val = 0xFFFF;
	
	modbus_send_buf[0] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_id;
	modbus_send_buf[1] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_fcode;
	modbus_send_buf[2] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_address & 0xFF00) >> 8;					
	modbus_send_buf[3] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_address & 0x00FF);				// Address
	modbus_send_buf[4] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_length & 0xFF00) >> 8;					
	modbus_send_buf[5] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_length & 0x00FF);				// Length
	
	for(i=0; i < 6; i++)
		crc_16_modbus_val = update_crc_16(crc_16_modbus_val, modbus_send_buf[i]);

	modbus_send_buf[6] = (uint8_t)(crc_16_modbus_val & 0x00FF);
	modbus_send_buf[7] = (uint8_t)(crc_16_modbus_val >> 8);
	
	for(i=0; i < 8; i++)
		SendChar1(modbus_send_buf[i]);
	
	if((flg_mrtu_485_enable) && (flg_ethernet_connected))
	{
		sprintf(tmp_buffer,"%02X %02X %02X %02X %02X %02X %02X %02X\r\n", modbus_send_buf[0],
			modbus_send_buf[1],modbus_send_buf[2],modbus_send_buf[3],modbus_send_buf[4],
			modbus_send_buf[5],modbus_send_buf[6],modbus_send_buf[7]);
		Send_Ethernet_Packet(tmp_buffer);
	}
}
//------------------------------------------------------------------------------------------

void ModbusRtu_Recv_Proc_485(void)
{
	char tmp_buffer[20]={0,};
	uint16_t i, data_send_len, wifi_tmp_indx;
	
	data_send_len = urx1_count + 6;
	wifi_tmp_indx = wifi_wr_indx + 1;
	wifi_tmp_indx %= WBUF_COUNT;
	
	if((wifi_rd_indx != wifi_tmp_indx) && (urx1_count <= (WBUF_LEN - 15)))
	{
		wifi_send_buf[wifi_wr_indx][0] = 0x00;
		wifi_send_buf[wifi_wr_indx][1] = 0xBA;
		wifi_send_buf[wifi_wr_indx][2] = (Flashdatarec.e2p_id & 0xFF000000) >> 24;
		wifi_send_buf[wifi_wr_indx][3] = (Flashdatarec.e2p_id & 0x00FF0000) >> 16;
		wifi_send_buf[wifi_wr_indx][4] = (Flashdatarec.e2p_id & 0x0000FF00) >> 8;
		wifi_send_buf[wifi_wr_indx][5] = (Flashdatarec.e2p_id & 0x000000FF);
		wifi_send_buf[wifi_wr_indx][6] = 0x60;								// Command for RTU
		wifi_send_buf[wifi_wr_indx][7] = (data_send_len & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][8] = (data_send_len & 0x00FF);
		wifi_send_buf[wifi_wr_indx][9] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_id;	// ï¿½ï¿½ï¿½ï¿½
		wifi_send_buf[wifi_wr_indx][10] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_fcode;
		wifi_send_buf[wifi_wr_indx][11] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_address & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][12] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_address & 0x00FF);
		wifi_send_buf[wifi_wr_indx][13] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_length & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][14] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx_485].mb_length & 0x00FF);
		
		memcpy(&wifi_send_buf[wifi_wr_indx][15], urx1_buf, urx1_count);
		
		wifi_send_len[wifi_wr_indx] = urx1_count + 15;
		
		if((flg_mrtu_485_enable) && (flg_ethernet_connected))
		{
			sprintf(tmp_buffer,"Recv %d Byte: ", urx1_count);
			Send_Ethernet_Packet(tmp_buffer);
			
			for(i=0; i<wifi_send_len[wifi_wr_indx]; i++)
			{
				sprintf(tmp_buffer,"%02X ", wifi_send_buf[wifi_wr_indx][i]);
				Send_Ethernet_Packet(tmp_buffer);
			}
			Send_Ethernet_Packet("\r\n");
		}

		wifi_wr_indx++;
		wifi_wr_indx %= WBUF_COUNT;
		
		mprintf("Rcv232 Len = %d, wifi_wr_indx = %d\n", urx1_count, wifi_wr_indx);
	}
	
}