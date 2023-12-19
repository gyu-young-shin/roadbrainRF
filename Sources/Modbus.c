#include "main.h"
#include "modbus.h"



_Bool		crc_tab16_init = 0;
uint16_t	crc_tab16[256];
//--------------------------------------------------------------------------------------
uint16_t update_crc_16(uint16_t crc, uint8_t c) 
{
	uint16_t tmp;
	uint16_t short_c;

	short_c = 0x00ff & (uint16_t) c;

	if(!crc_tab16_init) 
		init_crc16_tab();

	tmp =  crc ^ short_c;
	crc = (crc >> 8) ^ crc_tab16[tmp & 0xff];

	return crc;

}  /* update_crc_16 */
//--------------------------------------------------------------------------------------
void init_crc16_tab(void) 
{
	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for(i=0; i<256; i++) 
	{
		crc = 0;
		c   = i;

		for(j=0; j<8; j++) 
		{
			if((crc ^ c) & 0x0001) 
				crc = (crc >> 1) ^ CRC_POLY_16;
			else                      
				crc = crc >> 1;

			c = c >> 1;
		}

		crc_tab16[i] = crc;
	}

	crc_tab16_init = 1;

}  /* init_crc16_tab */
//--------------------------------------------------------------------------------------

