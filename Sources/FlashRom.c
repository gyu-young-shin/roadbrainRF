#include "main.h"
#include <string.h>
#include "flashrom.h"
#include "debug.h"

TE2PDataRec 	Flashdatarec;

volatile uint32_t Start_Address, offset_data, offset_var;
volatile uint32_t *p_data;

extern void FLASH_PageErase(uint32_t PageAddress);
//===============================================================================
void FlashRom_Init(void)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError = 0;

	Start_Address = FLASH_CONF_START_ADDR;	
	// 11번째 Flash Secor의 첫번째 Word Data를 읽는다.
	offset_data = *(volatile uint32_t *)(Start_Address);

	if(offset_data == 0xFFFFFFFF)			// 해당값과 틀리다면 아무것도 안 써졌음.
	{
		memset(&Flashdatarec.e2p_offset_addr, 0, sizeof(Flashdatarec));
		Flashdatarec.e2p_offset_addr = 0x00;

		// 공유기 ssid, password
		strcpy((char *)Flashdatarec.e2p_station_ssid, "MCTECH-2.4G");	// Station SSID
		strcpy((char *)Flashdatarec.e2p_station_pawd, "0325082047");	// Station Password
		
		Flashdatarec.e2p_local_ip[0] = 192;							// Local Ip Address
		Flashdatarec.e2p_local_ip[1] = 168;
		Flashdatarec.e2p_local_ip[2] = 0;
		Flashdatarec.e2p_local_ip[3] = 100;

		Flashdatarec.e2p_local_gw[0] = 192;							// Local Gateway Address
		Flashdatarec.e2p_local_gw[1] = 168;
		Flashdatarec.e2p_local_gw[2] = 0;
		Flashdatarec.e2p_local_gw[3] = 1;

		Flashdatarec.e2p_server_ip[0] = 192;							// Server Ip Address
		Flashdatarec.e2p_server_ip[1] = 168;
		Flashdatarec.e2p_server_ip[2] = 0;
		Flashdatarec.e2p_server_ip[3] = 21;

		Flashdatarec.e2p_mb_sip[0] = 192;							// Server Ip Address
		Flashdatarec.e2p_mb_sip[1] = 168;
		Flashdatarec.e2p_mb_sip[2] = 0;
		Flashdatarec.e2p_mb_sip[3] = 21;
		
		Flashdatarec.e2p_mb_portnum = 502;							// Tcp Server Port number
		Flashdatarec.e2p_232_int = 1;								// 1 sec
		Flashdatarec.e2p_485_int = 1;								// 1 sec

		Flashdatarec.e2p_server_port = 39000;						// Server Port number
		
		//0x00,0x0A,0xDC,0x57,0xf7,0x88
		Flashdatarec.e2p_mac_addr[0] = 0x00;
		Flashdatarec.e2p_mac_addr[1] = 0x0A;
		Flashdatarec.e2p_mac_addr[2] = 0xDC;
		Flashdatarec.e2p_mac_addr[3] = 0x57;
		Flashdatarec.e2p_mac_addr[4] = 0xF7;
		Flashdatarec.e2p_mac_addr[5] = 0x88;
		
		Flashdatarec.e2p_baudrate = 115200;
		Flashdatarec.e2p_232_baudrate = 115200;
		
		Flashdatarec.e2p_set_mac = 0;
		
		Flashdatarec.e2p_mac_wifi[0] = 0x00;
		Flashdatarec.e2p_mac_wifi[1] = 0x08;
		Flashdatarec.e2p_mac_wifi[2] = 0xDC;
		Flashdatarec.e2p_mac_wifi[3] = 0x5C;
		Flashdatarec.e2p_mac_wifi[4] = 0x88;
		Flashdatarec.e2p_mac_wifi[5] = 0xBC;

		Flashdatarec.e2p_set_wifimac = 0;
		
		Flashdatarec.e2p_id = 0;
		
		Flashdatarec.e2p_req_interval = 10;		// 100ms 단위 1sec
		Flashdatarec.e2p_mod_req_inter = 10;
		
		memset(Flashdatarec.e2p_mod_rtu, 0, sizeof(TModbusRtu) * CLIENT_COUNT);
		memset(Flashdatarec.e2p_mod_tcp, 0, sizeof(TModbusTcp) * CLIENT_COUNT);
		
		__disable_irq();						// Global Interrupt Disable
		HAL_FLASH_Unlock();						// 내부 Flash 사용하기 위해 Unlock 한다
		
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR); 
		
		// 나머지 전체 영역을 지운다.
		offset_data = 0;
		

		while(offset_data < FLASH_DATA_SIZE)
		{
			/* Erase the user Flash area */
			EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
			EraseInitStruct.PageAddress = (FLASH_CONF_START_ADDR + offset_data); //User defined addr
			EraseInitStruct.NbPages     = 1;		

			if(HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
			{
				HAL_FLASH_Lock(); 
				__enable_irq();
				
				mprintf("Flash Erase Error!!\n");
				return;
			}
			offset_data += 0x800;		// 2K
		}
		
		p_data = (volatile uint32_t *)(&Flashdatarec);
		
		while(Start_Address < (FLASH_CONF_START_ADDR + sizeof(Flashdatarec)))
		{
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Start_Address, *p_data) != HAL_OK)
			{
				HAL_FLASH_Lock(); 
				__enable_irq();

				mprintf("Flash Program Error!!\n");
				return;
			}
			Start_Address += 4;
			p_data++;
		}
		
		HAL_FLASH_Lock(); 
		__enable_irq();
	}
	else
	{
		offset_data = sizeof(TE2PDataRec);		// 2번째 부터 읽는다.
		// 마지막 데이터를 읽는다.
		while(offset_data < (FLASH_DATA_SIZE - sizeof(TE2PDataRec)))
		{
			offset_var = *(volatile uint32_t *)(FLASH_CONF_START_ADDR + offset_data);
			if(offset_var == 0xFFFFFFFF)
			{
				offset_data -= sizeof(TE2PDataRec);
				break;
			}
			else
				offset_data += sizeof(TE2PDataRec);
		}

		p_data = (volatile uint32_t *)(&Flashdatarec);
		
		Start_Address = FLASH_CONF_START_ADDR + offset_data;
		
		while(Start_Address < (FLASH_CONF_START_ADDR + offset_data + sizeof(TE2PDataRec)))
		{
			*p_data = *(volatile uint32_t *)Start_Address;
			Start_Address += 4;
			p_data++;
		}
	}
}

void FlashRom_WriteData(void)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError = 0;

	volatile uint32_t Start_Address;
	volatile uint32_t *p_data;
	
	Start_Address = FLASH_CONF_START_ADDR + Flashdatarec.e2p_offset_addr + sizeof(TE2PDataRec);
	
	__disable_irq();			// Global Interrupt Disable
	HAL_FLASH_Unlock();				// 내부 Flash 사용하기 위해 Unlock 한다
	
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR); 

	// Write할 주소가 마지막으로부터 데이터 크기를 뺀 것보다 크거나 같다면
	if(Start_Address >= (FLASH_CONF_START_ADDR + FLASH_DATA_SIZE - (sizeof(TE2PDataRec) * 4)))
	{
		// 나머지 전체 영역을 지운다.
		offset_data = 0;
		while(offset_data < FLASH_DATA_SIZE)
		{
			/* Erase the user Flash area */
			EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
			EraseInitStruct.PageAddress = (FLASH_CONF_START_ADDR + offset_data); //User defined addr
			EraseInitStruct.NbPages     = 1;		

			if(HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
			{
				HAL_FLASH_Lock(); 
				__enable_irq();
				
				mprintf("Flash Erase Error!!\n");
				return;
			}
			offset_data += 0x800;		// 2K
		}
		Flashdatarec.e2p_offset_addr = 0;
	}
	else
		Flashdatarec.e2p_offset_addr += sizeof(TE2PDataRec);  // Write 할 주소 증가
	
	
	Start_Address = FLASH_CONF_START_ADDR + Flashdatarec.e2p_offset_addr;
	p_data = (volatile uint32_t *)(&Flashdatarec);
	
	while(Start_Address < (FLASH_CONF_START_ADDR + Flashdatarec.e2p_offset_addr + sizeof(TE2PDataRec)))
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Start_Address, *p_data) != HAL_OK)
		{
			HAL_FLASH_Lock(); 
			__enable_irq();

			mprintf("Flash Program Error!!\n");
			return;
		}
		Start_Address += 4;
		p_data++;
	}

	HAL_FLASH_Lock(); 
	__enable_irq();
}

//=====================================================================
void Flash_Erash_Page(uint32_t Address)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError = 0;
	
	/* Erase the user Flash area */
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = Address; 					//User defined addr
	EraseInitStruct.NbPages     = 1;		

	if(HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
	{
		HAL_FLASH_Lock(); 
		__enable_irq();
		
		mprintf("Flash Erase Error!!\n");
	}
}

