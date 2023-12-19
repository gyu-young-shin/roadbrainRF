#ifndef	__WIFI_H
#define	__WIFI_H

#define	URX3_LEN		512
#define	UTX3_LEN		512
#define	WBUF_LEN		256
#define	WBUF_COUNT		100
#include "main.h"

#define BACKSPACE			0x08			// Backspace
#define CRETURN				0x0D			// Carriage Return
#define LF					0x0A			// Line Feed
#define ESC					0x1B			// Escape
#define DEL					0x7F			// Del


void USART3_IRQ_Function(void);
void Wifi_proc(void);


void Init_Station_mode(void);
void Init_SoftAP_mode(void);
void Connect_Tcp_Server(void);
void Open_Tcp_Server(void);
void Station_mode_proc(void);
void SoftAP_mode_proc(void);
void Answer_status(uint8_t	cmd_kind);
void Send_wifi_packet(uint16_t send_len);
void Answer_0x02(void);
void Answer_0x04(void);
void Init_Init_mode(void);
void Tcp_Data_Send(void);
void Make_packet(void);
void Send_packet(void);
void Get_Signal_Strength(void);


void SendChar3(unsigned char send_c);
void mprintf3(const char *format, ...);					// For RS232 Debug


#endif
