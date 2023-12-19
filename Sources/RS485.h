#ifndef	__RS485_H
#define	__RS485_H
#include "main.h"

#define	URX1_LEN		512
#define	UTX1_LEN		128

void USART1_IRQ_Function(void);
void Rs485_proc(void);
void Request_Status(uint8_t kind);

void SendChar1(uint8_t send_c);
void mprintf1(const char *format, ...);

uint8_t	Next_Rtu_Index_485(void);
void Modbus_Rtu_Proc_485(void);
void ModbusRtu_Recv_Proc_485(void);


#endif
