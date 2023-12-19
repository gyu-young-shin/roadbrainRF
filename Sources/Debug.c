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

_Bool 		wifi_debug_enable = 0;

_Bool 		urx2_comp;
uint8_t		urx2_buf[URX2_LEN];
uint8_t		utx2_buf[UTX2_LEN];
uint8_t		utx2_tmp[UTX2_LEN];

uint8_t		tx2_restart = 1;
uint16_t 	urx2_count = 0;
uint16_t	p2_out = 0;
uint16_t	p2_in = 0;
uint16_t	urx2_tout;
uint8_t		flg_getline_comp = 0;

char		in_line[URX2_LEN];

#define MAX_HEX_STR_LENGTH  32
char hexStr[MAX_HEX_STR_LENGTH];

//--------------------------------------------------------------------------------------------------------------
extern		TE2PDataRec 	Flashdatarec;
//--------------------------------------------------------------------------------------------------------------
const	char help[] =
	"\n\n\n\n\n"
	"+-----------------------------------------------------------------------+\n"
	"|                         MC TECH Command Help                          |\n"
	"|-----------------------------------------------------------------------|\n"
	"|  HELP                                                                 |\n"
	"+-----------------------------------------------------------------------+\n";

const SCMD cmd[] = 
{
	"HELP",		cmd_help,
	
};

#define CMD_COUNT (sizeof (cmd) / sizeof (cmd[0]))
//==========================================================================================================
extern UART_HandleTypeDef huart2;
//==========================================================================================================
void USART2_IRQ_Function(void)				// For debug 232
{	

	uint8_t data_char;
	
	uint32_t isrflags   = READ_REG(USART2->SR);
	uint32_t cr1its     = READ_REG(USART2->CR1);
	uint32_t cr3its     = READ_REG(USART2->CR3);

	uint32_t errorflags;

	/* If no error occurs */
	errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
	if (errorflags == 0U)
	{
		if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
		{
			data_char = USART2->DR & 0x00FF;
		
			if((!urx2_comp) && (urx2_count < URX2_LEN))
			{
				urx2_buf[urx2_count++] = data_char;
				urx2_tout = 3;
			}
		}
		
		// Transmission
		if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))		
		{
			if(p2_out != p2_in) 							
			{
				USART2->DR = utx2_buf[p2_out];
				p2_out++;
				p2_out %= UTX2_LEN; 
				tx2_restart = 0;
			}
			else 
			{
				tx2_restart = 1;
				CLEAR_BIT(USART2->CR1, USART_CR1_TXEIE);
			}
		}
	}
	else
	{
		__HAL_UART_CLEAR_PEFLAG(&huart2);
		__HAL_UART_CLEAR_FEFLAG(&huart2);
		__HAL_UART_CLEAR_NEFLAG(&huart2);
		__HAL_UART_CLEAR_OREFLAG(&huart2);
	}
}


//------------------------------------------------------------------------------------------
void Debug_proc(void) 
{
	if(urx2_comp)
	{
		GetLine_Proc();  					
		UserCommand_Proc();
			
		urx2_comp = 0;
		urx2_count = 0;
	}
}
//------------------------------------------------------------------------------------------
void GetLine_Proc(void)												// Line 입력을 받는다.
{
	uint8_t 	temp_char;
	uint32_t	RxCnt_temp = urx2_count;

	temp_char = urx2_buf[urx2_count-1];
	
	if((temp_char == BACKSPACE) || (temp_char == DEL)) 				// 백스페이스, Del Key가 입력 되면
	{   
		urx2_comp = 0;
		if(RxCnt_temp > 1)
		{
			urx2_count				-= 2;
			urx2_buf[urx2_count]	 = 0;
			SendChar(BACKSPACE);
			SendChar(' ');
			SendChar(BACKSPACE);
		}
		else
		{
			urx2_count				= 0;
			urx2_buf[urx2_count] 	= 0;
		}
	  	
    }
	else if((urx2_count >= URX2_LEN)  || (temp_char == CRETURN)) 
	{
		urx2_comp = 1;
		SendChar(CRETURN);
		SendChar(LF);
		flg_getline_comp = 1;
		urx2_buf[urx2_count-1] = 0x0;		// Null Terminal Char
		urx2_count--;
    }
	else if(temp_char == ESC)
	{
		urx2_comp = 1;
		SendChar(CRETURN);
		SendChar(LF);
		flg_getline_comp = 1;
		urx2_buf[0] = 0x0;		// Null Terminal Char
		urx2_count = 0;
		wifi_debug_enable = 0;
	}
	else 
	{
		urx2_comp = 0;
		SendChar(temp_char);			// Echo 처리
	}
}

//------------------------------------------------------------------------------------------
void UserCommand_Proc(void)						// 사용자 명령어 처리
{
	uint32_t i;
	char *sp, *next;

	sp = 0;
	next = 0;

	if(flg_getline_comp)
	{
		if(urx2_count)
		{
			for(i=0; i<urx2_count; i++)
				urx2_buf[i] = toupper(urx2_buf[i]);
			strcpy(in_line, (char *)urx2_buf);
			
			sp = get_entry(&in_line[0], &next);

			if(*sp != 0)
			{
				for(i = 0; i < CMD_COUNT; i++) 
				{
					if(strcmp (sp, (const char *)&cmd[i].val)) 
						continue;
					cmd[i].func(next);                  
					break;
				}
				
				if(i == CMD_COUNT)
					mprintf ("\nCommand error\n");
			}
		}		
		
		mprintf("\nMcTech>");				// Prompt Print
		flg_getline_comp = 0;
		urx2_comp = 0;
		urx2_count = 0;
	}
}

//------------------------------------------------------------------------------------------
char *get_entry (char *cp, char **pnext) 
{
   char *sp;

   if(*cp == 0) 
   {
      *pnext = cp;
      return (cp);
   }

   for( ; *cp == ' '; cp++) 							// 앞부분 공백 처리 이후 cp는 첫 글자를 가리킴
   {
      *cp = 0;
   }
 
   for(sp=cp; (*sp != ' ') && (*sp != 0); sp++);

   if(*sp != 0)
   {
		for(; *sp == ' '; sp++)
			*sp = 0;
   }
   *pnext = sp;                /* &next entry                  */
   return (cp);
}

//------------------------------------------------------------------------------------------
void SendChar(uint8_t send_c) 
{
	uint16_t pin_temp;

	pin_temp = p2_in + 1;
	pin_temp %= UTX2_LEN;

	while(pin_temp == p2_out)
	{
		HAL_Delay(1);
	}
	
	utx2_buf[p2_in] = send_c;
	p2_in = pin_temp;

	if(tx2_restart) 
	{
		tx2_restart = 0;
		SET_BIT(USART2->CR1, USART_CR1_TXEIE);
	}
}

//------------------------------------------------------------------------------------------
void mprintf(const char *format, ...)
{
    uint16_t i;
	__va_list	arglist;

	va_start(arglist, format);
    vsprintf((char *)utx2_tmp, format, arglist);
	va_end(arglist);
	i = 0;
	
	while(utx2_tmp[i] && (i < UTX2_LEN))	// Null 문자가 아닌동안 개행문자를 개행문자 + Carrige Return 으로 변환
	{
		if(utx2_tmp[i] == '\n')
			SendChar('\r');
		SendChar(utx2_tmp[i++]);
	}
}

//------------------------------------------------------------------------------------------
void cmd_help(char *par)
{
	mprintf(help);
}
//----------------------------------------------------------------------------------------------------------------
void cmd_serverip(char *par)
{
	char *data_var, *next;
	uint8_t ip_addr[4] = {0,};

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		mprintf("SIP [xxx xxx xxx xxx] : 0~255\n"); 
		mprintf("SIP=%d.%d.%d.%d\n", Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
									Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3]); 
	}
	else
	{
		if(atoi(data_var) <= 255)
			ip_addr[0] = atoi(data_var);
		else
			goto ERR_PARAM;
		
		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[1] = atoi(data_var);
		else
			goto ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[2] = atoi(data_var);
		else
			goto ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[3] = atoi(data_var);
		else
			goto ERR_PARAM;
		
		Flashdatarec.e2p_server_ip[0] = ip_addr[0];
		Flashdatarec.e2p_server_ip[1] = ip_addr[1];
		Flashdatarec.e2p_server_ip[2] = ip_addr[2];
		Flashdatarec.e2p_server_ip[3] = ip_addr[3];
		FlashRom_WriteData();
		mprintf("SIP=%d.%d.%d.%d\n", Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
									Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3]); 
		
		return;
		
		ERR_PARAM:
			mprintf("Parameter Error!!\n");

	}
}
//----------------------------------------------------------------------------------------------------------------
void cmd_serverport(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		mprintf("SP [xxxxx] : 1024~65535\n"); 
		mprintf("SP=%d\n", Flashdatarec.e2p_server_port); 
	}
	else
	{
		if((atoi(data_var) >= 1024) && (atoi(data_var) <= 65535))
		{
			Flashdatarec.e2p_server_port = atoi(data_var);
			FlashRom_WriteData();
			mprintf("SP=%d\n", Flashdatarec.e2p_server_port); 
		}
		else
			mprintf("Parameter Error!!\n");
	}
}
//----------------------------------------------------------------------------------------------------------------
