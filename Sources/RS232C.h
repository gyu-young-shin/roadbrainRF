#ifndef	__RS232C_H
#define	__RS232C_H
#include "main.h"

#define	URX4_LEN		512
#define	UTX4_LEN		128

void USART4_IRQ_Function(void);
void Rs232c_proc(void);

void SendChar4(unsigned char send_c);
void mprintf4(const char *format, ...);					// For RS232 Debug

uint8_t	Next_Rtu_Index(void);
void Modbus_Rtu_Proc(void);
void ModbusRtu_Recv_Proc(void);

#endif
