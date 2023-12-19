#include "cmsis_os.h"                   /* CMSIS RTOS definitions             */
#include "stm32f0xx.h"
#include "si446x.h"
#include "radio_config.h"
#include "spi.h"
#include "debug.h"


//================================================================================================
#define SI446X_SEL_GPIO_PIN		GPIO_Pin_12
#define SI446X_SEL_GPIO_PORT	GPIOA
#define SI446X_SEL_GPIO_CLK		RCC_AHBPeriph_GPIOA
#define SI446X_SEL_HIGH()	 	GPIO_SetBits(SI446X_SEL_GPIO_PORT, SI446X_SEL_GPIO_PIN)
#define SI446X_SEL_LOW()	  	GPIO_ResetBits(SI446X_SEL_GPIO_PORT, SI446X_SEL_GPIO_PIN)
					
#define SI446X_SDN_GPIO_PIN		GPIO_Pin_7
#define SI446X_SDN_GPIO_PORT	GPIOA
#define SI446X_SDN_GPIO_CLK		RCC_AHBPeriph_GPIOA
#define SI446X_SDN_HIGH()	 	GPIO_SetBits(SI446X_SDN_GPIO_PORT, SI446X_SDN_GPIO_PIN)
#define SI446X_SDN_LOW()	  	GPIO_ResetBits(SI446X_SDN_GPIO_PORT, SI446X_SDN_GPIO_PIN)
	
#define SI446X_IRQ_GPIO_PIN		GPIO_Pin_11
#define SI446X_IRQ_GPIO_PORT	GPIOA
#define SI446X_IRQ_GPIO_CLK		RCC_AHBPeriph_GPIOA
#define SI446X_READ_IRQ()    	GPIO_ReadInputDataBit(SI446X_IRQ_GPIO_PORT, SI446X_IRQ_GPIO_PIN)

#define SI446X_CTS_GPIO_PIN		GPIO_Pin_12
#define SI446X_CTS_GPIO_PORT	GPIOB
#define SI446X_CTS_GPIO_CLK		RCC_AHBPeriph_GPIOB
#define SI446X_READ_CTS()    	GPIO_ReadInputDataBit(SI446X_CTS_GPIO_PORT, SI446X_CTS_GPIO_PIN)	
//================================================================================================
const uint8_t Radio_Configuration_Data_Array[] =  RADIO_CONFIGURATION_DATA_ARRAY;
tRadioConfiguration RadioConfiguration_TRX = RADIO_CONFIGURATION_DATA;

union CMD_REPLY_UNION SI446X_CMD_REPLY;

//uint8_t TxErrCnt = 0,RxErrCnt = 0;
//================================================================================================
uint8_t bSpi_ReadDataByte(void)
{
	return (SPI1_ReadDataByte());
}

void bSpi_WriteDataByte(uint8_t data)
{
	SPI1_WriteDataByte(data);
}

void bSpi_ReadDataNByte(uint8_t bDataOutLength, uint8_t *pbDataOut)
{
	while(bDataOutLength --)
		*pbDataOut++ = bSpi_ReadDataByte();
}

void bSpi_WriteDataNByte(uint8_t bDataInLength, uint8_t *pbDataIn)
{
	while(bDataInLength --)
		bSpi_WriteDataByte(*pbDataIn++);
}

void bApi_SendCommand(uint8_t bCmdLength, uint8_t *pbCmdData) 
{   
    SI446X_SEL_LOW();
    bSpi_WriteDataNByte(bCmdLength, pbCmdData);
    SI446X_SEL_HIGH();
}

uint8_t bApi_WaitForCTS(void)
{
	uint8_t bCtsValue = 0;
	uint16_t bErrCnt  = 0; 
	
	while (bCtsValue != 0xFF)
	{
		SI446X_SEL_LOW();
		bSpi_WriteDataByte(READ_CMD_BUFF);
		bCtsValue = bSpi_ReadDataByte();
		if(bCtsValue != 0xFF)
		{
			SI446X_SEL_HIGH();
		}
		
		if (++bErrCnt > MAX_CTS_RETRY)
		{
			return 1;
		}
	}
	return 0;
}

uint8_t bApi_GetResponse(uint8_t bRespLength, uint8_t *pbRespData)
{
	if(bApi_WaitForCTS() != 0)
	{
		return 1;
	}
	bSpi_ReadDataNByte(bRespLength, pbRespData);
	SI446X_SEL_HIGH();	
	return 0;
}

uint8_t bApi_SendCmdGetResp(uint8_t cmdByteCount, uint8_t *pCmdData, uint8_t respByteCount, uint8_t *pRespData)
{
	uint8_t result = 1;
	
	bApi_SendCommand(cmdByteCount, pCmdData);
	result = bApi_GetResponse(respByteCount, pRespData);
	return result; 

}

void si446x_hard_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//SI446X SDN
	RCC_AHBPeriphClockCmd(SI446X_SDN_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = SI446X_SDN_GPIO_PIN;                  
	
	GPIO_Init(SI446X_SDN_GPIO_PORT, &GPIO_InitStructure);

	//SI446X SEL
	RCC_AHBPeriphClockCmd(SI446X_SEL_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = SI446X_SEL_GPIO_PIN;      
	
	GPIO_Init(SI446X_SEL_GPIO_PORT, &GPIO_InitStructure);
	
	//SI446X IRQ
	RCC_AHBPeriphClockCmd(SI446X_IRQ_GPIO_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = SI446X_IRQ_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SI446X_IRQ_GPIO_PORT, &GPIO_InitStructure);
	
	SI446X_SEL_HIGH();
	SI446X_SDN_HIGH();		
}

void si446x_reset(void)
{
	SI446X_SDN_HIGH();
	osDelay(5);
	SI446X_SDN_LOW();
	osDelay(10);
}

uint8_t si446x_configuration_init(const uint8_t *pSetPropCmd)
{
	uint8_t col, numOfBytes;
	uint8_t cmd[16];
	
	/* While cycle as far as the pointer points to a command */
	while (*pSetPropCmd != 0x00)
	{
		numOfBytes = *pSetPropCmd++;
		
		if (numOfBytes > 16)
		{
			return SI446X_COMMAND_ERROR;
		}
		
		for (col=0; col<numOfBytes; col++)
		{
			cmd[col] = *pSetPropCmd++;
		}
		
		if (bApi_SendCmdGetResp(numOfBytes, cmd, 0, 0) != 0)
		{
			return SI446X_CTS_TIMEOUT;
		}        
	}
	return SI446X_SUCCESS;
}

uint8_t si446x_power_up(uint8_t BOOT_OPTIONS, uint8_t XTAL_OPTIONS, uint32_t XO_FREQ)
{    
	uint8_t result = 1;
	uint8_t cmd[7];
	
	cmd[0] = POWER_UP;
	cmd[1] = BOOT_OPTIONS;
	cmd[2] = XTAL_OPTIONS;
	cmd[3] = (uint8_t)(XO_FREQ >> 24);
	cmd[4] = (uint8_t)(XO_FREQ >> 16);
	cmd[5] = (uint8_t)(XO_FREQ >> 8);
	cmd[6] = (uint8_t)(XO_FREQ);	
	result = bApi_SendCmdGetResp(7, cmd, 0, 0);
	return result;  
}

uint8_t si446x_nop(void)
{
	uint8_t result = 1;
	uint8_t cmd;
	
	cmd = NOP;	
	result = bApi_SendCmdGetResp(1, &cmd, 0, 0);
	return result;    
}

uint8_t si446x_part_info(void)
{    
	uint8_t result = 1;
	uint8_t buffer[8];
	uint8_t cmd;
	
	cmd = PART_INFO;
	result = bApi_SendCmdGetResp(1, &cmd, 8, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.PART_INFO_REPLY.CHIPREV  = buffer[0];
		SI446X_CMD_REPLY.PART_INFO_REPLY.PART     = (buffer[1] << 8) + buffer[2];
		SI446X_CMD_REPLY.PART_INFO_REPLY.PBUILD   = buffer[3];
		SI446X_CMD_REPLY.PART_INFO_REPLY.ID       = (buffer[4] << 8) + buffer[5];
		SI446X_CMD_REPLY.PART_INFO_REPLY.CUSTOMER = buffer[6];
		SI446X_CMD_REPLY.PART_INFO_REPLY.ROMID    = buffer[7];
	}
	return result;  
}

uint8_t si446x_set_property_x(SI446X_PROPERTY START_PROP, uint8_t NUM_PROPS, uint8_t *PAR_BUFF)
{
	uint8_t result = 1;
	uint8_t cmd[16], i = 0;
	
	if(NUM_PROPS > 12)
	{ 
		return 1; 
	}
	cmd[i++] = SET_PROPERTY;
	cmd[i++] = (uint8_t)(START_PROP >> 8);
	cmd[i++] = NUM_PROPS;
	cmd[i++] = (uint8_t)(START_PROP);
	while(NUM_PROPS--)
	{
		cmd[i++] = *PAR_BUFF++;
	}
	result = bApi_SendCmdGetResp(i, cmd, 0, 0);
	return result;
    
}

uint8_t si446x_get_property_x(SI446X_PROPERTY START_PROP, uint8_t NUM_PROPS, uint8_t *PAR_BUFF)
{
	uint8_t result = 1;
	uint8_t cmd[4];
	if(NUM_PROPS > 16)
	{ 
		return 1; 
	}
	cmd[0] = GET_PROPERTY;
	cmd[1] = (uint8_t)(START_PROP>>8);
	cmd[2] = NUM_PROPS;
	cmd[3] = (uint8_t)(START_PROP);
	result = bApi_SendCmdGetResp(4, cmd, NUM_PROPS, PAR_BUFF);
	return result;
}

uint8_t si446x_set_property_1(SI446X_PROPERTY START_PROP, uint8_t PAR_VALUE)
{
	uint8_t result = 1;
	uint8_t cmd[5];
	
	cmd[0] = SET_PROPERTY;
	cmd[1] = (uint8_t)(START_PROP >> 8);
	cmd[2] = 1;
	cmd[3] = (uint8_t)(START_PROP);
	cmd[4] = PAR_VALUE;
	result = bApi_SendCmdGetResp(5, cmd, 0, 0);
	return result;  
}

uint8_t si446x_get_property_1(SI446X_PROPERTY START_PROP, uint8_t *pPAR_VALUE)
{
	uint8_t result = 1;
	uint8_t cmd[4];
	
	cmd[0] = GET_PROPERTY;
	cmd[1] = (uint8_t)(START_PROP>>8);
	cmd[2] = 1;
	cmd[3] = (uint8_t)(START_PROP);
	result = bApi_SendCmdGetResp(4, cmd, 1, pPAR_VALUE);
	return result;
}

uint8_t si446x_gpio_pin_cfg(uint8_t G0, uint8_t G1, uint8_t G2, uint8_t G3, uint8_t NIRQ, uint8_t SDO, uint8_t GEN_CONFIG)
{
	uint8_t result = 1;
	uint8_t buffer[7];
	uint8_t cmd[8];
	
	cmd[0] = GPIO_PIN_CFG;
	cmd[1] = G0;
	cmd[2] = G1;
	cmd[3] = G2;
	cmd[4] = G3;
	cmd[5] = NIRQ;
	cmd[6] = SDO;
	cmd[7] = GEN_CONFIG;
	result = bApi_SendCmdGetResp(8, cmd, 7, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.GPIO0      = buffer[0];
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.GPIO1      = buffer[1];
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.GPIO2      = buffer[2];
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.GPIO3      = buffer[3];
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.NIRQ       = buffer[4];
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.SDO        = buffer[5];
		SI446X_CMD_REPLY.GPIO_PIN_CFG_REPLY.GEN_CONFIG = buffer[6];
	}
	return result;
}

uint8_t si446x_fifo_info(uint8_t FIFO)
{
	uint8_t result = 1;
	uint8_t buffer[2];
	uint8_t cmd[2];
	
	cmd[0] = FIFO_INFO;
	cmd[1] = FIFO;
	result = bApi_SendCmdGetResp(2, cmd, 2, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.FIFO_INFO_REPLY.RX_FIFO_COUNT = buffer[0];
		SI446X_CMD_REPLY.FIFO_INFO_REPLY.TX_FIFO_SPACE = buffer[1];
	}
	return result;
}

uint8_t si446x_ircal(uint8_t SEARCHING_STEP_SIZE, uint8_t SEARCHING_RSSI_AVG, uint8_t RX_CHAIN_SETTING1, uint8_t RX_CHAIN_SETTING2)
{
	uint8_t result = 1;
	uint8_t buffer[2];
	uint8_t cmd[5];
	
	cmd[0] = IRCAL;
	cmd[1] = SEARCHING_STEP_SIZE;
	cmd[2] = SEARCHING_RSSI_AVG;
	cmd[3] = RX_CHAIN_SETTING1;
	cmd[4] = RX_CHAIN_SETTING2;
	result = bApi_SendCmdGetResp(5, cmd, 2, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.IRCAL_REPLY.IRCAL_AMP = buffer[0];
		SI446X_CMD_REPLY.IRCAL_REPLY.IRCAL_PH  = buffer[1];
	}
	return result;
}

uint8_t si446x_get_int_status(uint8_t PH_CLR_PEND, uint8_t MODEM_CLR_PEND, uint8_t CHIP_CLR_PEND)
{
	uint8_t result = 1;
	uint8_t buffer[8];
	uint8_t cmd[4];
	
	cmd[0] = GET_INT_STATUS;
	cmd[1] = PH_CLR_PEND;
	cmd[2] = MODEM_CLR_PEND;
	cmd[3] = CHIP_CLR_PEND;
	result = bApi_SendCmdGetResp(4, cmd, 8, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.INT_PEND     = buffer[0];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.INT_STATUS   = buffer[1];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.PH_PEND      = buffer[2];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.PH_STATUS    = buffer[3];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.MODEM_PEND   = buffer[4];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.MODEM_STATUS = buffer[5];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.CHIP_PEND    = buffer[6];
		SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.CHIP_STATUS  = buffer[7];
	}
	return result;    
}

uint8_t si446x_get_ph_status(uint8_t PH_CLR_PEND)
{
	uint8_t result = 1;
	uint8_t buffer[2];
	uint8_t cmd[2];
	
	cmd[0] = GET_PH_STATUS;
	cmd[1] = PH_CLR_PEND;
	result = bApi_SendCmdGetResp(2, cmd, 2, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.GET_PH_STATUS_REPLY.PH_PEND   = buffer[0];
		SI446X_CMD_REPLY.GET_PH_STATUS_REPLY.PH_STATUS = buffer[1];
	}
	return result;    
}

uint8_t si446x_get_modem_status(uint8_t MODEM_CLR_PEND)
{  
	uint8_t result = 1;
	uint8_t buffer[8];
	uint8_t cmd[2];
	
	cmd[0] = GET_MODEM_STATUS;
	cmd[1] = MODEM_CLR_PEND;	
	result = bApi_SendCmdGetResp(2, cmd, 8, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.MODEM_PEND      = buffer[0];
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.MODEM_STATUS    = buffer[1];
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.CURR_RSSI       = buffer[2];
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.LATCH_RSSI      = buffer[3];
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.ANT1_RSSI       = buffer[4];
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.ANT2_RSSI       = buffer[5];
		SI446X_CMD_REPLY.GET_MODEM_STATUS_REPLY.AFC_FREQ_OFFSET = (buffer[6] << 8) + buffer[7];
	}
	return result;  
}

uint8_t si446x_get_chip_status(uint8_t CHIP_CLR_PEND)
{
	uint8_t result = 1;
	uint8_t buffer[4];
	uint8_t cmd[2];
	
	cmd[0] = GET_CHIP_STATUS;
	cmd[1] = CHIP_CLR_PEND;
	result = bApi_SendCmdGetResp(2, cmd, 4, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.GET_CHIP_STATUS_REPLY.CHIP_PEND      = buffer[0];
		SI446X_CMD_REPLY.GET_CHIP_STATUS_REPLY.CHIP_STATUS    = buffer[1];
		SI446X_CMD_REPLY.GET_CHIP_STATUS_REPLY.CMD_ERR_STATUS = buffer[2];
		SI446X_CMD_REPLY.GET_CHIP_STATUS_REPLY.CMD_ERR_CMD_ID = buffer[3];		
	}
	return result;    
}

uint8_t si446x_get_packet_info(uint8_t FIELD_NUMBER, uint16_t LEN, uint16_t LEN_DIFF)
{
    uint8_t result = 1;
    uint8_t buffer[2];
	uint8_t cmd[6];
	
    cmd[0] = PACKET_INFO;
    cmd[1] = FIELD_NUMBER;
    cmd[2] = (uint8_t)(LEN >> 8);
    cmd[3] = (uint8_t)(LEN);
    cmd[4] = (uint8_t)(LEN_DIFF >> 8);
    cmd[5] = (uint8_t)(LEN_DIFF);
    result = bApi_SendCmdGetResp(6, cmd, 2, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.PACKET_INFO_REPLY.LENGTH_15_8 = buffer[0];
		SI446X_CMD_REPLY.PACKET_INFO_REPLY.LENGTH_7_0  = buffer[1];
	}
	return result;
}

void si446x_frr_read(void)
{
	uint8_t buffer[4];
	uint8_t cmd;
	
	cmd = FRR_A_READ;
	SI446X_SEL_LOW();
	bSpi_WriteDataByte(cmd);
	bSpi_ReadDataNByte(4, buffer);
	SI446X_SEL_HIGH();
	
	SI446X_CMD_REPLY.FRR_READ_REPLY.FRR_A_VALUE = buffer[0];
	SI446X_CMD_REPLY.FRR_READ_REPLY.FRR_B_VALUE = buffer[1];
	SI446X_CMD_REPLY.FRR_READ_REPLY.FRR_C_VALUE = buffer[2];
	SI446X_CMD_REPLY.FRR_READ_REPLY.FRR_D_VALUE = buffer[3];
}

uint8_t si446x_frr_read_1(SI446X_CMD FRR_X_READ)
{
	uint8_t result;
	uint8_t cmd;

	cmd = FRR_X_READ;	
	SI446X_SEL_LOW();
	bSpi_WriteDataByte(cmd);
	result = bSpi_ReadDataByte();
	SI446X_SEL_HIGH();
	return result;
}


uint8_t si446x_start_tx(uint8_t CHANNEL, uint8_t CONDITION, uint16_t TX_LEN)
{
	uint8_t result = 1;
	uint8_t cmd[5];
	
	cmd[0] = START_TX;
	cmd[1] = CHANNEL;
	cmd[2] = CONDITION;
	cmd[3] = (uint8_t)(TX_LEN >> 8);
	cmd[4] = (uint8_t)(TX_LEN);
	result = bApi_SendCmdGetResp(5, cmd, 0, 0);
	return result;  
}

uint8_t si446x_start_rx(uint8_t CHANNEL, uint8_t CONDITION, uint16_t RX_LEN, SI446X_STATE NEXT_STATE1,
                                                           SI446X_STATE NEXT_STATE2, 
                                                           SI446X_STATE NEXT_STATE3)
{
	uint8_t result = 1;
	uint8_t cmd[8];
	
	cmd[0] = START_RX;
	cmd[1] = CHANNEL;
	cmd[2] = CONDITION;
	cmd[3] = (uint8_t)(RX_LEN >> 8);
	cmd[4] = (uint8_t)(RX_LEN);
	cmd[5] = NEXT_STATE1;
	cmd[6] = NEXT_STATE2;
	cmd[7] = NEXT_STATE3;
	result = bApi_SendCmdGetResp(8, cmd, 0, 0);
	return result;  
}

uint8_t si446x_change_state(SI446X_STATE NEXT_STATE1)
{
	uint8_t result = 1;
	uint8_t cmd[2];
	
	cmd[0] = CHANGE_STATE;
	cmd[1] = NEXT_STATE1;
	result = bApi_SendCmdGetResp(2, cmd, 0, 0);
	return result;
}

void si446x_write_tx_fifo(uint8_t numBytes, uint8_t *pTxData)
{
	uint8_t cmd;
	
	cmd = WRITE_TX_FIFO;
	SI446X_SEL_LOW();
	bSpi_WriteDataByte(cmd);
	bSpi_WriteDataNByte(numBytes, pTxData);
	SI446X_SEL_HIGH();
}

void si446x_read_rx_fifo(uint8_t numBytes, uint8_t *pRxData)
{
	uint8_t cmd;
	
	cmd = READ_RX_FIFO;
	SI446X_SEL_LOW();
	bSpi_WriteDataByte(cmd);
	bSpi_ReadDataNByte(numBytes, pRxData);
	SI446X_SEL_HIGH();
}


/*
uint8_t si446x_apply_patch(void)
{
;
}
*/

uint8_t si446x_func_info(void)
{
	uint8_t result = 1;
	uint8_t buffer[6];
	uint8_t cmd;
	
	cmd = FUNC_INFO;
	result = bApi_SendCmdGetResp(1, &cmd, 6, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.FUNC_INFO_REPLY.REVEXT    = buffer[0];
		SI446X_CMD_REPLY.FUNC_INFO_REPLY.REVBRANCH = buffer[1];
		SI446X_CMD_REPLY.FUNC_INFO_REPLY.REVINT    = buffer[2];
		SI446X_CMD_REPLY.FUNC_INFO_REPLY.PATCH     = (buffer[3] << 8) + buffer[4];
		SI446X_CMD_REPLY.FUNC_INFO_REPLY.FUNC      = buffer[5];
	}
	return result;
}

uint8_t si446x_get_adc_reading(uint8_t ADC_EN, uint8_t ADC_CFG)
{
	uint8_t result = 1;
	uint8_t buffer[6];
	uint8_t cmd[3];
	
	cmd[0] = GET_ADC_READING;
	cmd[1] = ADC_EN;
	cmd[2] = ADC_CFG;
	result = bApi_SendCmdGetResp(3, cmd, 6, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.GET_ADC_READING_REPLY.GPIO_ADC    = (buffer[0] << 8) + buffer[1];
		SI446X_CMD_REPLY.GET_ADC_READING_REPLY.BATTERY_ADC = (buffer[2] << 8) + buffer[3];
		SI446X_CMD_REPLY.GET_ADC_READING_REPLY.TEMP_ADC    = (buffer[4] << 8) + buffer[5];
	}
	return result;
}

/*
uint8_t si446x_protocol_cfg(uint8_t PROTOCOL)
{
;
}
*/

uint8_t si446x_request_device_state(void)
{
	uint8_t result = 1;
	uint8_t buffer[2];
	uint8_t cmd;
	
	cmd= REQUEST_DEVICE_STATE;
	result = bApi_SendCmdGetResp(1, &cmd, 2, buffer);
	if(result == 0)
	{
		SI446X_CMD_REPLY.REQUEST_DEVICE_STATE_REPLY.CURR_STATE      = buffer[0];
		SI446X_CMD_REPLY.REQUEST_DEVICE_STATE_REPLY.CURRENT_CHANNEL = buffer[1];
	}
	return result;
}

uint8_t si446x_rx_hop(uint8_t INTE, uint32_t FRAC, uint16_t VCO_CNT)
{
	uint8_t result = 1;
	uint8_t cmd[7];
	
	cmd[0]= RX_HOP;
	cmd[1]= INTE;
	cmd[2]= (uint8_t)(FRAC >> 16);
	cmd[3]= (uint8_t)(FRAC >> 8);
	cmd[4]= (uint8_t)(FRAC);
	cmd[5]= (uint8_t)(VCO_CNT >> 8);
	cmd[6]= (uint8_t)(VCO_CNT);
	result = bApi_SendCmdGetResp( 7, cmd, 0, 0);
	return result;
}

/*
uint8_t si446x_agc_override(uint8_t AGC_OVERRIDE)
{
;
}
*/


void gRadio_monitor_OK(uint8_t channel)
{
	uint8_t result = 1;

	result = si446x_change_state(READY);
	result = si446x_request_device_state();
	if(result == 0)
	{
		if((SI446X_CMD_REPLY.REQUEST_DEVICE_STATE_REPLY.CURR_STATE      != READY) | \
		   (SI446X_CMD_REPLY.REQUEST_DEVICE_STATE_REPLY.CURRENT_CHANNEL != channel))
		{
			gRadio_init_RX_TX();
		}
	}
	else
	{
		gRadio_init_RX_TX();
	}
}

uint8_t gRadio_check_CCA(uint8_t channel)
{    
	uint8_t retry;

	retry = 5;
	while(retry)
	{
		si446x_fifo_info(0x02);
		si446x_start_rx(channel, 0, 0, NOCHANGE, READY, READY);
		osDelay(50);
		si446x_get_int_status(0, 0, 0);
		if(SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.MODEM_PEND & (1 << 1))
		{
			retry --;
			si446x_change_state(READY);
		}
		else
		{
			return 0;
		}
	}
	return 1;
}

uint8_t gRadio_send_packet(uint8_t channel, uint8_t size, uint8_t *txbuffer)
{
	uint8_t tx_len = size;
	uint8_t retry_counter;
	
	si446x_fifo_info(0x01);				
	si446x_write_tx_fifo(1, &tx_len);
	si446x_write_tx_fifo(tx_len, txbuffer);
	tx_len ++;

	if(gRadio_check_CCA(channel) == 0)
	{
		si446x_start_tx(channel, READY << 4, tx_len);
		retry_counter = 200;
		while(retry_counter--)
		{			 
			si446x_get_int_status(0, 0, 0);	
			if((SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.PH_PEND & (1 << 5)) && \
			   (SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.PH_PEND != 0xFF))
			{
				return SI446X_SUCCESS;
			}
			osDelay(1);		
		}
		si446x_change_state(READY);
		return SI446X_FAILURE;
	}
	else
	{
		return SI446X_AIR_BUSSY;
	}
}

uint8_t gRadio_goto_receive(uint8_t channel)
{
	uint8_t retry_counter;
	
	si446x_fifo_info(0x02);
	si446x_start_rx(channel, 0, 0, NOCHANGE, READY, READY);

	retry_counter = 200;
	while(retry_counter--)
	{
		si446x_get_int_status(0, 0, 0);
		if((SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.PH_PEND & (1 << 4)) && \
		   (SI446X_CMD_REPLY.GET_INT_STATUS_REPLY.PH_PEND != 0xFF))
		{
			return SI446X_SUCCESS;
		}
		osDelay(5);
	}
	si446x_change_state(READY);
	return SI446X_FAILURE;
}

uint8_t gRadio_read_packet(uint8_t *buffer)
{
	uint8_t rx_len;

	si446x_read_rx_fifo(1, &rx_len);
	si446x_read_rx_fifo(rx_len, buffer);
	si446x_fifo_info(0x02);
	
    return rx_len;
}

void gRadio_init_RX_TX(void)
{        
    si446x_reset();
        
    while (SI446X_SUCCESS != si446x_configuration_init(Radio_Configuration_Data_Array))
    {   
        osDelay(10);
        si446x_reset();
    }
    si446x_get_int_status(0, 0, 0);
    si446x_part_info();
    si446x_fifo_info(0x03);    
}





