#ifndef	__FLASHROM_H
#define	__FLASHROM_H

#include "main.h"
// STM32F103VCT6
#define FLASH_CONF_START_ADDR	0x0803C000		// 8K
#define FLASH_DATA_SIZE			0x4000			// 16K
#define CLIENT_COUNT			30

typedef struct
{
	uint8_t		mb_enable;					// Enable ����
	uint8_t		mb_id;						// Modbus ID ����
	uint8_t		mb_fcode;					// Modbus Function Code
	uint16_t	mb_address;					// Modbus Read Address
	uint16_t	mb_length;					// Modbus read length
	uint8_t		mb_pad;						// pad
} TModbusRtu;

typedef struct
{
	uint8_t		mb_enable;					// Enable ����
	uint16_t	mb_transaction;				// Transaction number
	uint8_t		mb_fcode;					// Modbus Function Code
	uint16_t	mb_address;					// Modbus Read Address
	uint16_t	mb_length;					// Modbus read length
} TModbusTcp;

typedef  struct
{
	uint32_t   	e2p_offset_addr;			// Offset Address

	uint8_t   	e2p_local_ip[4];			// Local Ip Address
	uint8_t   	e2p_local_gw[4];			// Local gateway Address

	uint8_t   	e2p_server_ip[4];			// Server Ip Address
	uint16_t   	e2p_server_port;			// Server Port number 10

	uint8_t   	e2p_station_ssid[20];		// Station SSID
	uint8_t   	e2p_station_pawd[20];		// Station Password 50

	uint32_t   	e2p_baudrate;				// RS485 Baud Rate 54
	uint32_t   	e2p_232_baudrate;			// RS232 Baud Rate 58
	
	uint8_t   	e2p_mac_addr[6];			// Set Mac Address 64
	uint8_t		e2p_set_mac;				// 65
	
	uint8_t   	e2p_mac_wifi[6];			// Wifi Mac Address
	uint8_t		e2p_set_wifimac;			// 72
	
	uint32_t	e2p_id;						// DEVICE ID  76
	
	uint8_t		e2p_modbus_sel;				// TCP:0, RTU:1			77
	uint8_t		e2p_req_interval;			// Request Interval time
	uint16_t	e2p_mod_req_inter;			// ��û���� 100ms ����	79
	
	uint8_t   	e2p_mb_sip[4];				// Tcp Server Ip Address
	uint16_t	e2p_mb_portnum;				// Tcp erver Port Number
	uint8_t   	e2p_232_int;				// RS232 Interval time
	uint8_t		e2p_485_int;				// RS485 Interval time

	//uint8_t 	e2p_modbus_sel_485;     // RTU - 485	

	TModbusRtu	e2p_mod_rtu[CLIENT_COUNT];			// RTU Client Record 8 x 30 = 240
	TModbusTcp	e2p_mod_tcp[CLIENT_COUNT];			// TCP Client Record 8 x 30 = 240  640 + 79 

} TE2PDataRec;


void FlashRom_Init(void);
void FlashRom_WriteData(void);
void Flash_Erash_Page(uint32_t Address);

#endif
