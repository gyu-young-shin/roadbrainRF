#ifndef	__RFMODULE_H
#define	__RFMODULE_H
#include "main.h"

void RFModule_Init(void);
void RFModule_proc(void);
void SlaveMode_Proc(void);
void MasterMode_Proc(void);
void Check_RF_Tx_Complete(void);

void Slave_work(uint16_t Data_len);
uint16_t Get_Cardlist_Count(void);
void Get_Rssi(void);
void Disp_LoraLed(void);
void Card_Data_down_proc(void);
void Pass_Data_down_proc(void);
void Card_Data_read_proc(void);
void Slave_Answer_Proc(uint16_t dest_id, uint8_t command, uint8_t *data_str, uint8_t data_len);

#endif
