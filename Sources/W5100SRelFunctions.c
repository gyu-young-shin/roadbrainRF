#include "main.h"
#include "W5100SRelFunctions.h"
#include "debug.h"

//extern DMA_HandleTypeDef hdma_memtomem_dma1_channel5;		// TX
//extern DMA_HandleTypeDef hdma_memtomem_dma1_channel4;		// RX
//===========================================================================
void W5100SInitialze(void)
{
	intr_kind temp;
	unsigned char W5100S_AdrSet[2][4] = {{2,2,2,2},{2,2,2,2}};
	/*
	 */
	temp = IK_DEST_UNREACH;

	if(ctlwizchip(CW_INIT_WIZCHIP,(void*)W5100S_AdrSet) == -1)
	{
		mprintf("W5100S initialized fail.\r\n");
	}

	if(ctlwizchip(CW_SET_INTRMASK,&temp) == -1)
	{
		mprintf("W5100S interrupt\r\n");
	}

}

void busWriteByte(uint32_t addr, iodata_t data)
{
//	(*((volatile uint8_t*)(W5100SAddress+1))) = (uint8_t)((addr &0xFF00)>>8);
//	(*((volatile uint8_t*)(W5100SAddress+2))) = (uint8_t)((addr) & 0x00FF);
//	(*((volatile uint8_t*)(W5100SAddress+3))) = data;
//
	(*(volatile uint8_t*)(addr)) = data;

}


iodata_t busReadByte(uint32_t addr)
{
//	(*((volatile uint8_t*)(W5100SAddress+1))) = (uint8_t)((addr &0xFF00)>>8);
//	(*((volatile uint8_t*)(W5100SAddress+2))) = (uint8_t)((addr) & 0x00FF);
//	return  (*((volatile uint8_t*)(W5100SAddress+3)));
	return (*((volatile uint8_t*)(addr)));

}

void busWriteBurst(uint32_t addr, uint8_t* pBuf ,uint32_t len)
{
//	while(HAL_DMA_Start(&hdma_memtomem_dma1_channel5, (uint32_t)pBuf, addr, len) != HAL_OK)
//	{
//		if(HAL_DMA_GetError(&hdma_memtomem_dma1_channel5) == HAL_ERROR)
//		{
//			mprintf("busWriteBurst Error\n");
//			return;
//		}
//	}
//	
//	while(HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_channel5, HAL_DMA_FULL_TRANSFER, 1000) != HAL_OK)
//	{
//		if(HAL_DMA_GetError(&hdma_memtomem_dma1_channel5) == HAL_ERROR)
//		{
//			mprintf("busWriteBurst Error\n");
//			return;
//		}
//	}
}


void busReadBurst(uint32_t addr,uint8_t* pBuf, uint32_t len)
{
//	while(HAL_DMA_Start(&hdma_memtomem_dma1_channel4, addr, (uint32_t)pBuf, len) != HAL_OK)
//	{
//		if(HAL_DMA_GetError(&hdma_memtomem_dma1_channel4) == HAL_ERROR)
//		{
//			mprintf("busWriteBurst Error\n");
//			return;
//		}
//	}
//	
//	while(HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_channel4, HAL_DMA_FULL_TRANSFER, 1000) != HAL_OK)
//	{
//		if(HAL_DMA_GetError(&hdma_memtomem_dma1_channel4) == HAL_ERROR)
//		{
//			mprintf("busWriteBurst Error\n");
//			return;
//		}
//	}
}

inline void csEnable(void)
{
	//GPIO_ResetBits(W5100S_CS_PORT, W5100S_CS_PIN);
}

inline void csDisable(void)
{
	//GPIO_SetBits(W5100S_CS_PORT, W5100S_CS_PIN);
}

inline void resetAssert(void)
{
	HAL_GPIO_WritePin(W5100_RST_GPIO_Port, W5100_RST_Pin, GPIO_PIN_RESET);
}

inline void resetDeassert(void)
{
	HAL_GPIO_WritePin(W5100_RST_GPIO_Port, W5100_RST_Pin, GPIO_PIN_SET);
}

void W5100SReset(void)
{
	HAL_GPIO_WritePin(W5100_RST_GPIO_Port, W5100_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(W5100_RST_GPIO_Port, W5100_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(100);
}

/*
uint8_t spiReadByte(void)
{
	while (SPI_I2S_GetFlagStatus(W5100S_SPI, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(W5100S_SPI, 0xff);
	while (SPI_I2S_GetFlagStatus(W5100S_SPI, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(W5100S_SPI);
}

void spiWriteByte(uint8_t byte)
{
	while (SPI_I2S_GetFlagStatus(W5100S_SPI, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(W5100S_SPI, byte);
	while (SPI_I2S_GetFlagStatus(W5100S_SPI, SPI_I2S_FLAG_RXNE) == RESET);
	SPI_I2S_ReceiveData(W5100S_SPI);
}

uint8_t spiReadBurst(uint8_t* pBuf, uint16_t len)
{
	unsigned char tempbuf =0xff;
	DMA_TX_InitStructure.DMA_BufferSize = len;
	DMA_TX_InitStructure.DMA_MemoryBaseAddr = &tempbuf;
	DMA_Init(W5100S_DMA_CHANNEL_TX, &DMA_TX_InitStructure);

	DMA_RX_InitStructure.DMA_BufferSize = len;
	DMA_RX_InitStructure.DMA_MemoryBaseAddr = pBuf;
	DMA_Init(W5100S_DMA_CHANNEL_RX, &DMA_RX_InitStructure);
	// Enable SPI Rx/Tx DMA Request
	DMA_Cmd(W5100S_DMA_CHANNEL_RX, ENABLE);
	DMA_Cmd(W5100S_DMA_CHANNEL_TX, ENABLE);
	// Waiting for the end of Data Transfer
	while(DMA_GetFlagStatus(DMA_TX_FLAG) == RESET);
	while(DMA_GetFlagStatus(DMA_RX_FLAG) == RESET);


	DMA_ClearFlag(DMA_TX_FLAG | DMA_RX_FLAG);

	DMA_Cmd(W5100S_DMA_CHANNEL_TX, DISABLE);
	DMA_Cmd(W5100S_DMA_CHANNEL_RX, DISABLE);

}

void spiWriteBurst(uint8_t* pBuf, uint16_t len)
{
	unsigned char tempbuf;
	DMA_TX_InitStructure.DMA_BufferSize = len;
	DMA_TX_InitStructure.DMA_MemoryBaseAddr = pBuf;
	DMA_Init(W5100S_DMA_CHANNEL_TX, &DMA_TX_InitStructure);

	DMA_RX_InitStructure.DMA_BufferSize = 1;
	DMA_RX_InitStructure.DMA_MemoryBaseAddr = &tempbuf;
		DMA_RX_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_Init(W5100S_DMA_CHANNEL_RX, &DMA_RX_InitStructure);

	DMA_Cmd(W5100S_DMA_CHANNEL_RX, ENABLE);
	DMA_Cmd(W5100S_DMA_CHANNEL_TX, ENABLE);

	// Waiting for the end of Data Transfer 
	while(DMA_GetFlagStatus(DMA_TX_FLAG) == RESET);
	while(DMA_GetFlagStatus(DMA_RX_FLAG) == RESET);

	DMA_ClearFlag(DMA_TX_FLAG | DMA_RX_FLAG);

	DMA_Cmd(W5100S_DMA_CHANNEL_TX, DISABLE);
	DMA_Cmd(W5100S_DMA_CHANNEL_RX, DISABLE);

}
*/


