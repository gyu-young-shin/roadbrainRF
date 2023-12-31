#ifndef RADIO_H_
#define RADIO_H_
#include "main.h"
//#include "radio_config.h"

/*****************************************************************************
 *  Global Macros & Definitions
 *****************************************************************************/
/*! Maximal packet length definition (FIFO size) */
#define RADIO_MAX_PACKET_LENGTH     64u

/*****************************************************************************
 *  Global Typedefs & Enums
 *****************************************************************************/
typedef struct
{
    uint8_t   *Radio_ConfigurationArray;

    uint8_t   Radio_ChannelNumber;
    uint8_t   Radio_PacketLength;
    uint8_t   Radio_State_After_Power_Up;

    uint16_t  Radio_Delay_Cnt_After_Reset;

    uint8_t   Radio_CustomPayload[RADIO_MAX_PACKET_LENGTH];
} tRadioConfiguration;

/*****************************************************************************
 *  Global Variable Declarations
 *****************************************************************************/
extern tRadioConfiguration *pRadioConfiguration;
extern uint8_t customRadioPacket[RADIO_MAX_PACKET_LENGTH];

/*! Si446x configuration array */
extern uint8_t Radio_Configuration_Data_Array[];

/*****************************************************************************
 *  Global Function Declarations
 *****************************************************************************/
void  vRadio_Init(void);
uint8_t    bRadio_Check_Tx_RX(void);
void  vRadio_StartRX(uint8_t,uint8_t);
uint8_t    bRadio_Check_Ezconfig(uint16_t);
void  vRadio_StartTx_Variable_Packet(uint8_t, uint8_t*, uint8_t);
void Radio_test(void);

#endif /* RADIO_H_ */
