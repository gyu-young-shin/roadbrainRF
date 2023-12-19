#ifndef	__MODBUS_H_
#define	__MODBUS_H_
#include "main.h"

#define		CRC_POLY_16			0xA001
#define		CRC_START_MODBUS	0xFFFF

uint16_t update_crc_16(uint16_t crc, uint8_t c);
void init_crc16_tab(void) ;

#endif
