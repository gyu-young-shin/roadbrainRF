#ifndef _RADIO_COMM_H_
#define _RADIO_COMM_H_
#include "main.h"
#include <stdbool.h>

//#define RADIO_CTS_TIMEOUT 255
#define RADIO_CTS_TIMEOUT 10000

extern uint8_t radioCmd[16];

uint8_t radio_comm_GetResp(uint8_t byteCount, uint8_t* pData);
void radio_comm_SendCmd(uint8_t byteCount, uint8_t* pData);
void radio_comm_ReadData(uint8_t cmd, bool pollCts, uint8_t byteCount, uint8_t* pData);
void radio_comm_WriteData(uint8_t cmd, bool pollCts, uint8_t byteCount, uint8_t* pData);

uint8_t radio_comm_PollCTS(void);
uint8_t radio_comm_SendCmdGetResp(uint8_t cmdByteCount, uint8_t* pCmdData, \
                             uint8_t respByteCount, uint8_t* pRespData);
void radio_comm_ClearCTS(void);

#endif //_RADIO_COMM_H_
