#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include "rs232c.h"
#include "repeater.h"
#include "wifi.h"
#include "flashrom.h"
#include "buzzer.h"
#include "debug.h"
#include "w5100s_proc.h"
#include "modbus.h"

_Bool		flg_next_rtu = 1;

_Bool 		urx4_comp;
uint8_t		urx4_buf[URX4_LEN];
uint8_t		utx4_buf[UTX4_LEN];
uint8_t		utx4_tmp[UTX4_LEN];

uint8_t		tx4_restart = 1;
uint16_t 	urx4_count = 0;
uint16_t	p4_out = 0;
uint16_t	p4_in = 0;
uint16_t	urx4_tout;

uint16_t	rtu_int_timeout = 0;

uint8_t		mod_rtu_indx = 0;
uint8_t		rtu_retry_count = 0;
//==========================================================================================================
extern	TE2PDataRec 	Flashdatarec;
extern 	UART_HandleTypeDef huart4;
extern 	uint8_t		modbus_send_buf[64];

extern 	_Bool		crc_tab16_init;
extern 	uint16_t	crc_tab16[256];

extern	uint8_t		wifi_send_buf[WBUF_COUNT][WBUF_LEN];
extern	uint16_t	wifi_send_len[WBUF_COUNT];
extern	uint16_t	wifi_rd_indx;
extern	uint16_t	wifi_wr_indx;
extern _Bool		flg_mrtu_enable;
extern _Bool		flg_ethernet_connected;
extern	uint16_t	send_232int_timeout;
//==========================================================================================================
void USART4_IRQ_Function(void)				// For debug 232
{	

	uint8_t data_char;
	
	uint32_t isrflags   = READ_REG(UART4->SR);
	uint32_t cr1its     = READ_REG(UART4->CR1);
	uint32_t cr3its     = READ_REG(UART4->CR3);

	uint32_t errorflags;

	/* If no error occurs */
	errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
	if (errorflags == 0U)
	{
		if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
		{
			data_char = UART4->DR & 0x00FF;
		
			if((!urx4_comp) && (urx4_count < URX4_LEN))
			{
				urx4_buf[urx4_count++] = data_char;
				urx4_tout = 3;
			}
		}
		
		// Transmission
		if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))		
		{
			if(p4_out != p4_in) 							
			{
				UART4->DR = utx4_buf[p4_out];
				p4_out++;
				tx4_restart = 0;
				p4_out %= UTX4_LEN; 
			}
			else 
			{
				tx4_restart = 1;
				CLEAR_BIT(UART4->CR1, USART_CR1_TXEIE);
			}
		}
	}
	else
	{
		__HAL_UART_CLEAR_PEFLAG(&huart4);
		__HAL_UART_CLEAR_FEFLAG(&huart4);
		__HAL_UART_CLEAR_NEFLAG(&huart4);
		__HAL_UART_CLEAR_OREFLAG(&huart4);
	}
}
//------------------------------------------------------------------------------------------
void Rs232c_proc(void) 
{
	char 		tmp_buffer[20] = {0,};
	uint8_t		sel_ret;
	uint16_t	i, wifi_tmp_indx;
	
	if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
	{
		if(urx4_comp)
		{
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				ModbusRtu_Recv_Proc();
			
			flg_next_rtu = 1;

			urx4_comp = 0;
			urx4_count = 0;
		}
		
		
		if(flg_next_rtu)
		{
			flg_next_rtu = 0;
			sel_ret = Next_Rtu_Index();
			rtu_retry_count = 0;
		}
		
		if(sel_ret)
		{
			if(rtu_int_timeout == 0)
			{
				rtu_retry_count++;
				if(rtu_retry_count > 3)
				{
					rtu_retry_count = 0;
					flg_next_rtu = 1;
				}
				else
				{
					Modbus_Rtu_Proc();
					rtu_int_timeout = Flashdatarec.e2p_mod_req_inter;
					//mprintf("rtu_int_timeout = %d\n", rtu_int_timeout);
				}
			}
		}
		else
			flg_next_rtu = 1;
	}
	else if(Flashdatarec.e2p_modbus_sel == 2)			// RS232
	{
		if(urx4_comp)
		{
			if(send_232int_timeout == 0)
			{
				wifi_tmp_indx = wifi_wr_indx + 1;
				wifi_tmp_indx %= WBUF_COUNT;
				
				if((wifi_rd_indx != wifi_tmp_indx) && (urx4_count <= (WBUF_LEN - 9)))
				{
					wifi_send_buf[wifi_wr_indx][0] = 0x00;
					wifi_send_buf[wifi_wr_indx][1] = 0xBA;
					wifi_send_buf[wifi_wr_indx][2] = (Flashdatarec.e2p_id & 0xFF000000) >> 24;
					wifi_send_buf[wifi_wr_indx][3] = (Flashdatarec.e2p_id & 0x00FF0000) >> 16;
					wifi_send_buf[wifi_wr_indx][4] = (Flashdatarec.e2p_id & 0x0000FF00) >> 8;
					wifi_send_buf[wifi_wr_indx][5] = (Flashdatarec.e2p_id & 0x000000FF);
					wifi_send_buf[wifi_wr_indx][6] = 0x1A;
					wifi_send_buf[wifi_wr_indx][7] = (urx4_count & 0xFF00) >> 8;					
					wifi_send_buf[wifi_wr_indx][8] = (urx4_count & 0x00FF);
					memcpy(&wifi_send_buf[wifi_wr_indx][9], urx4_buf, urx4_count);
					wifi_send_len[wifi_wr_indx] = urx4_count + 9;
					
					wifi_wr_indx++;
					wifi_wr_indx %= WBUF_COUNT;
				}
				
				send_232int_timeout = Flashdatarec.e2p_232_int * 10;
			}
			
			if((flg_mrtu_enable) && (flg_ethernet_connected))
			{
				sprintf(tmp_buffer,"Recv %d Byte: ", urx4_count);
				Send_Ethernet_Packet(tmp_buffer);
				
				for(i=0; i<urx4_count; i++)
				{
					sprintf(tmp_buffer,"%02X ", urx4_buf[i]);
					Send_Ethernet_Packet(tmp_buffer);
				}
				Send_Ethernet_Packet("\r\n");
			}
			
			urx4_comp = 0;
			urx4_count = 0;
		}
		
		flg_next_rtu = 1;
	}
	else
	{
		urx4_comp = 0;
		urx4_count = 0;
		flg_next_rtu = 1;
	}
}
//------------------------------------------------------------------------------------------
void SendChar4(uint8_t send_c) 
{
	uint16_t pin_temp;

	pin_temp = p4_in + 1;
	pin_temp %= UTX4_LEN;

	while(pin_temp == p4_out)
	{
		HAL_Delay(1);
	}
	
	utx4_buf[p4_in] = send_c;
	p4_in = pin_temp;

	if(tx4_restart) 
	{
		tx4_restart = 0;
		SET_BIT(UART4->CR1, USART_CR1_TXEIE);
	}
}

//------------------------------------------------------------------------------------------
void mprintf4(const char *format, ...)
{
    uint16_t i;
	__va_list	arglist;

	va_start(arglist, format);
    vsprintf((char *)utx4_tmp, format, arglist);
	va_end(arglist);
	i = 0;
	
	while(utx4_tmp[i] && (i < UTX4_LEN))	// Null ���ڰ� �ƴѵ��� ���๮�ڸ� ���๮�� + Carrige Return ���� ��ȯ
	{
		if(utx4_tmp[i] == '\n')
			SendChar4('\r');
		SendChar4(utx4_tmp[i++]);
	}
}
//------------------------------------------------------------------------------------------
uint8_t	Next_Rtu_Index(void)
{
	uint8_t i, ret_var = 0, tmp_indx;
	
	
	tmp_indx = mod_rtu_indx;

	tmp_indx++;
	tmp_indx %= CLIENT_COUNT;

	for(i=0; i<CLIENT_COUNT; i++)
	{
		
		if(Flashdatarec.e2p_mod_rtu[tmp_indx].mb_enable)
		{
			mod_rtu_indx = tmp_indx;
			ret_var = 1;
			break;
		}
		
		tmp_indx++;
		tmp_indx %= CLIENT_COUNT;
	}
	
	return ret_var;
}
//------------------------------------------------------------------------------------------
void Modbus_Rtu_Proc(void)
{
	char 	tmp_buffer[20] = {0,};
	uint16_t	i, crc_16_modbus_val = 0xFFFF;
	
	modbus_send_buf[0] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_id;
	modbus_send_buf[1] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_fcode;
	modbus_send_buf[2] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_address & 0xFF00) >> 8;					
	modbus_send_buf[3] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_address & 0x00FF);				// Address
	modbus_send_buf[4] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_length & 0xFF00) >> 8;					
	modbus_send_buf[5] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_length & 0x00FF);				// Length
	
	for(i=0; i < 6; i++)
		crc_16_modbus_val = update_crc_16(crc_16_modbus_val, modbus_send_buf[i]);

	modbus_send_buf[6] = (uint8_t)(crc_16_modbus_val & 0x00FF);
	modbus_send_buf[7] = (uint8_t)(crc_16_modbus_val >> 8);
	
	for(i=0; i < 8; i++)
		SendChar4(modbus_send_buf[i]);
	
	if((flg_mrtu_enable) && (flg_ethernet_connected))
	{
		sprintf(tmp_buffer,"%02X %02X %02X %02X %02X %02X %02X %02X\r\n", modbus_send_buf[0],
			modbus_send_buf[1],modbus_send_buf[2],modbus_send_buf[3],modbus_send_buf[4],
			modbus_send_buf[5],modbus_send_buf[6],modbus_send_buf[7]);
		Send_Ethernet_Packet(tmp_buffer);
	}
}
//------------------------------------------------------------------------------------------
void ModbusRtu_Recv_Proc(void)
{
	char tmp_buffer[20]={0,};
	uint16_t i, data_send_len, wifi_tmp_indx;
	
	data_send_len = urx4_count + 6;
	wifi_tmp_indx = wifi_wr_indx + 1;
	wifi_tmp_indx %= WBUF_COUNT;
	
	if((wifi_rd_indx != wifi_tmp_indx) && (urx4_count <= (WBUF_LEN - 15)))
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
		wifi_send_buf[wifi_wr_indx][9] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_id;	// ����
		wifi_send_buf[wifi_wr_indx][10] = Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_fcode;
		wifi_send_buf[wifi_wr_indx][11] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_address & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][12] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_address & 0x00FF);
		wifi_send_buf[wifi_wr_indx][13] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_length & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][14] = (Flashdatarec.e2p_mod_rtu[mod_rtu_indx].mb_length & 0x00FF);
		
		memcpy(&wifi_send_buf[wifi_wr_indx][15], urx4_buf, urx4_count);
		
		wifi_send_len[wifi_wr_indx] = urx4_count + 15;
		
		if((flg_mrtu_enable) && (flg_ethernet_connected))
		{
			sprintf(tmp_buffer,"Recv %d Byte: ", urx4_count);
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
		
		mprintf("Rcv232 Len = %d, wifi_wr_indx = %d\n", urx4_count, wifi_wr_indx);
	}
	
}


