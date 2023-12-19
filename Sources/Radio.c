#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "debug.h"
#include "radio_config.h"
#include "radio.h"
#include "Si446x_api_lib.h"
#include "Si446x_cmd.h"
#include "radio_hal.h"
#include "radio_comm.h"
#include "Flashrom.h"

uint8_t Radio_Configuration_Data_Array[] = RADIO_CONFIGURATION_DATA_ARRAY;
tRadioConfiguration RadioConfiguration = RADIO_CONFIGURATION_DATA;
tRadioConfiguration *pRadioConfiguration = &RadioConfiguration;


uint8_t customRadioPacket[RADIO_MAX_PACKET_LENGTH];
//============================================================================
extern TE2PDataRec Flashdatarec;
extern _Bool 	flg_rf_init_fail;
//============================================================================
/*****************************************************************************
 *  Local Function Declarations
 *****************************************************************************/
void vRadio_PowerUp(void)
{
	uint16_t wDelay = 0;

	si446x_reset();
	for (; wDelay < pRadioConfiguration->Radio_Delay_Cnt_After_Reset; wDelay++)
	{
		__nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop();
	}
}
/*
void Radio_test(void)
{
	uint8_t i;
	
	vRadio_PowerUp();
	//#define RF_POWER_UP 0x02, 0x81, 0x00, 0x01, 0xC9, 0xC3, 0x80
	memset(Pro2Cmd, 0, 16);
	Pro2Cmd[0] = 0x02;
	Pro2Cmd[1] = 0x81;
	Pro2Cmd[2] = 0x00;
	Pro2Cmd[3] = 0x01;
	Pro2Cmd[4] = 0xC9;
	Pro2Cmd[5] = 0xC3;
	Pro2Cmd[6] = 0x80;
	
    radio_comm_SendCmd(7, Pro2Cmd);
    i =radio_comm_GetResp(0, 0);
	
	//#define RF_GPIO_PIN_CFG 0x13, 0x00, 0x00, 0x00, 0x00, 0x67, 0x4B, 0x00
	memset(Pro2Cmd, 0, 16);
	Pro2Cmd[0] = 0x13;
	Pro2Cmd[1] = 0x00;
	Pro2Cmd[2] = 0x00;
	Pro2Cmd[3] = 0x00;
	Pro2Cmd[4] = 0x00;
	Pro2Cmd[5] = 0x67;
	Pro2Cmd[6] = 0x4B;
	Pro2Cmd[7] = 0x00;
	
    radio_comm_SendCmd(8, Pro2Cmd);
	memset(Pro2Cmd, 0, 16);
    i =radio_comm_GetResp(8, Pro2Cmd);
	i = i;
}
*/

void vRadio_Init(void)
{
	uint16_t wDelay;

	/* Power Up the radio chip */
	vRadio_PowerUp();

	pRadioConfiguration->Radio_ChannelNumber = Flashdatarec.si_ch;
	
	/* Load radio configuration */
	while (SI446X_SUCCESS != si446x_configuration_init(pRadioConfiguration->Radio_ConfigurationArray))
	{
		/* Error hook */
		//LED4 = !LED4;
		//GPIOB->ODR ^= GPIO_Pin_7;	// Lora Error LED
		HAL_GPIO_TogglePin(RFERR_LED_GPIO_Port, RFERR_LED_Pin);
		for(wDelay = 0x7FFF; wDelay--;)
		{}
		/* Power Up the radio chip */
		vRadio_PowerUp();
		
		if(flg_rf_init_fail)
		{
			LORAERRLED_ON;
			return;
		}
	}

	
	// Read ITs, clear pending ones
	si446x_get_int_status(0u, 0u, 0u);
	
	// PA Power Level ¼³Á¤
	si446x_set_property(0x22, 1, 0x01, Flashdatarec.si_db);
	LORAERRLED_OFF;
}

/*!
 *  Check if Packet sent IT flag or Packet Received IT is pending.
 *
 *  @return   SI4455_CMD_GET_INT_STATUS_REP_PACKET_SENT_PEND_BIT / SI4455_CMD_GET_INT_STATUS_REP_PACKET_RX_PEND_BIT
 *
 *  @note
 *
 */
uint8_t bRadio_Check_Tx_RX(void)
{
	//mprintf("NIRQ=%d\n", SPI_NIRQ);
	if(SPI_NIRQ == 0)
	{
		/* Read ITs, clear pending ones */
		si446x_get_int_status(0u, 0u, 0u);

		if (Si446xCmd.GET_INT_STATUS.CHIP_PEND & SI446X_CMD_GET_CHIP_STATUS_REP_CHIP_PEND_CMD_ERROR_PEND_BIT)
		{
			/* State change to */
			si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);

			/* Reset FIFO */
			si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);

			/* State change to */
			si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_RX);
		}

		if(Si446xCmd.GET_INT_STATUS.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT)
		{
			return SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT;
		}

		if(Si446xCmd.GET_INT_STATUS.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT)
		{
			/* Packet RX */

			/* Get payload length */
			si446x_fifo_info(0x00);

			si446x_read_rx_fifo(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT, &customRadioPacket[0]);

			return SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT;
		}

		if (Si446xCmd.GET_INT_STATUS.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_STATUS_CRC_ERROR_BIT)
		{
			/* Reset FIFO */
			si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);
		}
	}
	return 0;
}

/*!
 *  Set Radio to RX mode. .
 *
 *  @param channel Freq. Channel,  packetLength : 0 Packet handler fields are used , nonzero: only Field1 is used
 *
 *  @note
 *
 */
void vRadio_StartRX(uint8_t channel, uint8_t packetLenght )
{
	// Read ITs, clear pending ones
	si446x_get_int_status(0u, 0u, 0u);

	// Reset the Rx Fifo
	si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);

	/* Start Receiving packet, channel 0, START immediately, Packet length used or not according to packetLength */
	si446x_start_rx(channel, 0u, packetLenght,
				  SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
				  SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_READY,
				  SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_RX );
}

/*!
 *  Set Radio to TX mode, variable packet length.
 *
 *  @param channel Freq. Channel, Packet to be sent length of of the packet sent to TXFIFO
 *
 *  @note
 *
 */
void vRadio_StartTx_Variable_Packet(uint8_t channel, uint8_t *pioRadioPacket, uint8_t length)
{
	/* Leave RX state */
	si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);

	/* Read ITs, clear pending ones */
	si446x_get_int_status(0u, 0u, 0u);

	/* Reset the Tx Fifo */
	si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT);

	/* Fill the TX fifo with datas */
	si446x_write_tx_fifo(length, pioRadioPacket);

	/* Start sending packet, channel 0, START immediately */
	si446x_start_tx(channel, 0x80, length);
}

