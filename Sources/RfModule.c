#include "main.h"
#include "flashrom.h"
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "rfmodule.h"
#include "segment.h"
#include "repeater.h"
#include "buzzer.h"
#include "radio.h"
#include "Si446x_cmd.h"
#include "Si446x_api_lib.h"
#include "w5100s_proc.h"


//=====================================================================================
_Bool	flg_reponse = 0;
_Bool	Im_sending = 0;
_Bool	flg_card_batch = 0;
_Bool	flg_password_batch = 0;
_Bool	flg_cardread_batch = 0;
_Bool	flg_rf_excute = 0;
_Bool	flg_rf_first = 1;
_Bool 	flg_update_rssi = 0;
_Bool 	flg_rf_init_fail = 0;
_Bool 	flg_rf_datetime_set = 1;

_Bool 	flg_remote_close = 0;
_Bool 	flg_alarm_remote_release = 0;
_Bool 	flg_need_radio_init = 0;

_Bool	flg_rf_command = 0;
uint8_t rf_data_id = 0;
uint8_t rf_data_command = 0;
uint8_t rf_data_len = 0;
uint8_t rf_data_buffer[64] = {0,};

uint8_t buffer[64];
uint8_t send_str[64];

uint8_t ledoff_timeout = 0;
uint8_t emdoor_state = 0;
uint8_t emlock_out_state = 0;

uint8_t batch_num = 0;
uint8_t pass_num = 0;
uint8_t read_num = 0;
uint8_t rf_retry_count = 0;
uint8_t do_timeout = 0;

uint16_t card_down_id = 0;
uint16_t pass_down_id = 0;
uint16_t card_read_id = 0;
uint16_t card_rcv_index = 0;
uint16_t prev_send_index = 0;
uint16_t bat_volt_int = 0;


uint8_t comm_stat[MAX_SLAVE_COUNT] = {0,};			// 통신 상태  
uint8_t retry_stat[MAX_SLAVE_COUNT] = {0,};			// 통신 상태  
uint8_t target_id = 0;
uint8_t send_timeout = 0;
uint8_t datetime_timeout = 60;

uint16_t tx_id = 1;
uint16_t tx_error_count = 0;


uint8_t cur_rssi = 0;
uint8_t max_rssi = 0;
uint8_t rcv_rssi = 0;

uint32_t fired_timeout = 0;
//=====================================================================================
extern TE2PDataRec Flashdatarec;
extern TCardlistrec	Flashcardrec;
extern TCarddata_rec	card_data[512];
extern TMy_TimeStruct 	DateTimeStruct;
extern uint8_t 	key_continued[KEY_COUNT];
extern uint8_t 	flg_1sec;
extern union 	si446x_cmd_reply_union  Si446xCmd;
extern uint8_t 	flg_toggle_500ms;
extern uint8_t	ethernet_command;
extern uint8_t	ethernet_id;
extern uint8_t	oper_mode;
extern uint8_t	target_slave_number;
extern uint8_t	rs485_retry_count;

extern _Bool	flg_ethernet_command;
extern _Bool 	flg_siren_out;
extern _Bool 	flg_emlock_out;
extern _Bool 	flg_nfc_tag;

extern _Bool 	flg_datetime_command;
extern _Bool 	flg_password_command;
extern _Bool 	flg_card_batch_command;
extern _Bool 	flg_warnoff_command;
extern _Bool	flg_rmt_door_access;

extern uint8_t	cb_num;
extern uint8_t	slave_status[MAX_SLAVE_COUNT];
extern uint8_t	emlock_status[MAX_SLAVE_COUNT];
extern uint8_t	ethernet_data[ETH_MAX_BUF_SIZE];
extern uint16_t battery_status[MAX_SLAVE_COUNT];
extern uint8_t	slave_status2[MAX_SLAVE_COUNT];

	

extern uint16_t	card_data_count;
extern uint16_t	card_recv_count;
extern uint16_t	e2p_passwd;
extern uint16_t	rs485_card_data_count;

extern uint16_t 	card_send_index;
extern uint8_t		nfc_uid[10];
extern float		adc_bat_volt;

extern uint16_t MakeCCITT16(uint8_t *data, uint32_t len);
extern void FLASH_PageErase(uint32_t PageAddress);
//=====================================================================================
void RFModule_proc(void) 
{
	if(flg_rf_init_fail)
		return;
	
	if(flg_need_radio_init)
	{
		flg_need_radio_init = 0;
		LORAERRLED_ON;
		vRadio_Init();
		flg_rf_first = 1;
		tx_error_count = 0;
	}
	
	
	Get_Rssi();
	
	if(flg_rf_first)
	{
		flg_rf_first = 0;
		pRadioConfiguration->Radio_ChannelNumber = Flashdatarec.si_ch;
		vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, 0);
	}
	
	switch(Flashdatarec.model)
	{
		case 0:				// 
			SlaveMode_Proc();
			
			if(flg_rf_excute)		// 100ms
			{
				flg_rf_excute = 0;
				tx_error_count++;
				if(tx_error_count > 20)
					flg_need_radio_init = 1;
			}
			break;
		case 1:				// 
			MasterMode_Proc();
		
			if(flg_rf_excute)
			{
				flg_rf_excute = 0;
				if(Flashdatarec.e2p_slave_count > 0)
				{
					tx_error_count++;
					if(tx_error_count > 20)
					{
						tx_error_count = 0;
						flg_need_radio_init = 1;
					}
				}
			}
			Disp_LoraLed();
			break;
		default:
			break;
	}
}

void Disp_LoraLed(void)
{
	uint8_t	i;
	
	for(i=1; i<MAX_SLAVE_COUNT; i++)
	{
		if(comm_stat[i])
		{
			LORAERRLED_ON;
			break;
		}
	}
	
	if(i == MAX_SLAVE_COUNT)
		LORAERRLED_OFF;
}

void Check_RF_Tx_Complete(void)
{
	uint8_t Main_IT_Status;

	Main_IT_Status = bRadio_Check_Tx_RX();

	switch(Main_IT_Status)
	{
		case SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT:	// Send Packet Complete
			LORATXLED_OFF;
			vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, 0);
			break;
	}
}
//==============================================================================
void MasterMode_Proc(void)
{
	uint8_t i, packet_len, Main_IT_Status;
	uint16_t  src_id, dest_id;
	uint16_t rcv_crc, cmp_crc, rcv_count;
	uint16_t make_crc, send_check_count;
	
	rcv_count = 0;
	Main_IT_Status = bRadio_Check_Tx_RX();

	switch(Main_IT_Status)
	{
		case SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT:	// Send Packet Complete
			LORATXLED_OFF;
			vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, 0);
			break;
		case SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT:		// Receive Packet
			if(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT > 64)
				memcpy(buffer, customRadioPacket, 64);
			else
				memcpy(buffer, customRadioPacket, Si446xCmd.FIFO_INFO.RX_FIFO_COUNT);
			
			rcv_count = Si446xCmd.FIFO_INFO.RX_FIFO_COUNT;
			tx_error_count = 0;
			//mprintf("Master Received Count: %d\n", rcv_count);
			break;
		default:
			break;
	}
	
	if(Flashdatarec.e2p_slave_count > 0)
	{
		if(rcv_count > 7)			// RF Packet 수신시
		{
			packet_len = buffer[5];
			if(packet_len > 56)
				return;
			// 0,1: Source Id, 2,3: Dest Id, 4: Command, 5:Length
			cmp_crc = MakeCCITT16(buffer, packet_len + 6);
			rcv_crc = (uint16_t)(buffer[packet_len + 6] << 8);
			rcv_crc |= buffer[packet_len + 7];
			
			
			src_id = (uint16_t)(buffer[0] << 8) + buffer[1];
			dest_id = (uint16_t)(buffer[2] << 8) + buffer[3];
			
			// Dest ID 가 같고 CRC 이상 없으면
			if((cmp_crc == rcv_crc) && (dest_id == 0x00))
			{
				LORARXLED_ON;
				ledoff_timeout = 10;		// RX LED Off after 150ms
				
				if(tx_id == src_id)			// 송신된 ID 와 수신된 ID 비교
				{
					flg_reponse = 1;
					retry_stat[tx_id] = 0;
					comm_stat[tx_id] = 0;
					
					switch(buffer[4])						// Command
					{
						case 0xA2:							// Card Tag
							for(i=0; i<4; i++)
								rf_data_buffer[i] = buffer[6 + i];
							slave_status[tx_id] = buffer[10];
							slave_status[tx_id] &= ~0x80;		// 통신 불량 해제
							battery_status[tx_id] = (uint16_t)(buffer[11] << 8) + buffer[12];
							emlock_status[tx_id] = buffer[13];
							slave_status2[tx_id] = buffer[14];
							
							rf_data_id = src_id;
							rf_data_len = 4;
							rf_data_command = 0x02;
							flg_rf_command = 1;
							break;
						case 0xB1:							// Device Status
							slave_status[tx_id] = buffer[6];
							slave_status[tx_id] &= ~0x80;		// 통신 불량 해제
							battery_status[tx_id] = (uint16_t)(buffer[7] << 8) + buffer[8];
							emlock_status[tx_id] = buffer[9];
							slave_status2[tx_id] = buffer[10];
							break;
						case 0xB2:							// Door 제어
							rf_data_buffer[0] = buffer[6];
							rf_data_id = src_id;
							rf_data_len = 1;
							rf_data_command = 0x12;
							flg_rf_command = 1;
							break;
						case 0xB3:							// Emergency Mode 해제
							rf_data_buffer[0] = buffer[6];
							rf_data_id = src_id;
							rf_data_len = 1;
							rf_data_command = 0x13;
							flg_rf_command = 1;
							break;
						default:
							break;
					}
				}
			}
		}
		
		if(flg_card_batch)
		{
			Card_Data_down_proc();
		}
		else if(flg_password_batch)
		{
			Pass_Data_down_proc();
		}
		else if(flg_ethernet_command && (send_timeout == 0))
		{
			// Ethernet으로 부터 온 명령어를 RF으로 전송 한다.
			flg_ethernet_command = 0;
			
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = (uint8_t)((ethernet_id & 0xFF00) >> 8);		// Dest   ID
			buffer[3] = (uint8_t)(ethernet_id & 0x00FF);
		
			
			buffer[4] = ethernet_command;							// Command
			
			buffer[5] = 0x01;										// Data Length
			buffer[6] = ethernet_data[0];

			make_crc = MakeCCITT16(buffer, 7);
			buffer[7] = (uint8_t)(make_crc >> 8);
			buffer[8] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, buffer, pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					send_timeout = 20;			// 200ms
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
		}
		else if(flg_rf_datetime_set && (send_timeout == 0))			// DateTime Set
		{
			flg_rf_datetime_set = 0;
			
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = 0x00;	// Dest   ID
			buffer[3] = 0x99;
		
			
			buffer[4] = 0x88;							// Command
			buffer[5] = 0x06;							// Data Length
			
			buffer[6] = DateTimeStruct.years;
			buffer[7] = DateTimeStruct.month;
			buffer[8] = DateTimeStruct.date;
			buffer[9] = DateTimeStruct.hours;
			buffer[10] = DateTimeStruct.minutes;
			buffer[11] = DateTimeStruct.sec;
			
			make_crc = MakeCCITT16(buffer, 12);
			buffer[12] = (uint8_t)(make_crc >> 8);
			buffer[13] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, buffer, pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					send_timeout = 20;			// 200ms
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
		}			
		else if(send_timeout == 0)
		{
			if(flg_reponse == 0)
			{
				if(retry_stat[tx_id] < 3)
					retry_stat[tx_id]++;
				else
				{
					comm_stat[tx_id] = 1;
					slave_status[tx_id] |= 0x80;		// 통신 불량
				}
			}
			
			tx_id++;
			if(tx_id > Flashdatarec.e2p_slave_count)
				tx_id = 1;
			
			memset(buffer, 0, sizeof(buffer));
			// 상태 조회 데이터를 RF 로 전송한다.
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = (uint8_t)((tx_id & 0xFF00) >> 8);				// Dest   ID
			buffer[3] = (uint8_t)(tx_id & 0x00FF);
			
			buffer[4] = 0xB1;					// Command
			buffer[5] = 0;						// Data Length

			make_crc = MakeCCITT16(buffer, 6);
			buffer[6] = (uint8_t)(make_crc >> 8);
			buffer[7] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, buffer, pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					send_timeout = 20;			// 200ms
					target_id = tx_id;
					flg_reponse = 0;
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
		}
	}
}
//==============================================================================
// Src id(2), Dest id(2), Cmd(1), Data_Len(1), Data(N), CRC16(2)
void SlaveMode_Proc(void)
{
	uint8_t Main_IT_Status;
	
	Main_IT_Status = bRadio_Check_Tx_RX();

	switch(Main_IT_Status)
	{
		case SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT:	// Send Packet Complete
			LORATXLED_OFF;
			vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, 0);
			break;
		case SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT:		// Receive Packet
			Im_sending = 0;					// 송신하고 있는지 표시
			LORARXLED_ON;
			ledoff_timeout = 10;			// RX LED Off after 150ms
			
			memset(buffer, 0, sizeof(buffer));
		
			if(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT > 64)
				memcpy(buffer, customRadioPacket, 64);
			else
				memcpy(buffer, customRadioPacket, Si446xCmd.FIFO_INFO.RX_FIFO_COUNT);
			
			Slave_work(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT);
		
			if(Im_sending == 0)
				vRadio_StartRX(pRadioConfiguration->Radio_ChannelNumber, 0);
			
			tx_error_count = 0;
			break;
		default:
			break;
	}
}
//=====================================================================================================
void Slave_work(uint16_t Data_len)
{
	uint8_t  i, rcv_count, packet_len;
	uint16_t rcv_crc, cmp_crc;
	uint16_t src_id, dest_id;
	
	if(Data_len >= 9)
	{
		packet_len = buffer[5];
		if(packet_len > 56)
			return;
		
		cmp_crc = MakeCCITT16(buffer, packet_len + 6);
		rcv_crc = (uint16_t)(buffer[packet_len + 6] << 8);
		rcv_crc |= buffer[packet_len + 7];
		
		//mprintf("cmp_crc: %x, rcv_crc: %x,\n", cmp_crc, rcv_crc);
		//mprintf("data_len: %d, rcv_data: %x,%x,%x,%x,%x,%x\n", Data_len, buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
		
		if(cmp_crc == rcv_crc)					// CRC Check
		{
			src_id = (uint16_t)(buffer[0] << 8) + buffer[1];
			dest_id = (uint16_t)(buffer[2] << 8) + buffer[3];
			
			if((dest_id == Flashdatarec.e2p_id) || (dest_id == 0x99))		// 수신된 ID가 자기꺼 이거나 전체 발송
			{
				LORARXLED_ON;
				ledoff_timeout = 10;			// RX LED Off after 150ms
				
				switch(buffer[4])	// command
				{
					case 0xB1:		// 비상문 통신 상태 (0bit: 0: Open, 1:Close, 1bit: 0:Emlock Nomal, 1: Emlock Cut
						// 2bit: 0:AC Momal, 1: AC Off, 3bit: 0: Battery Nomal, 1: battery discharge,
						// 4bit: 0:Cover Open, 1:Cover Close, 5bit: 0:Fire Signal Nomal, 1bit: Fire signal In
						// 6bit: 0:Emg Button, 1:Emg Button, 7bit: 1: Comm Error, 0: Comm Normal (for master)
					
						//bat_volt_int = (uint16_t)(adc_bat_volt * 10);
					
						if(flg_nfc_tag)
						{
							for(i=0; i<4; i++)
								send_str[i] = nfc_uid[i];
							
							send_str[4] = emdoor_state;
							send_str[5] = (bat_volt_int & 0xFF00) >> 8;
							send_str[6] = (bat_volt_int & 0x00FF);
							send_str[7] = emlock_out_state;
							send_str[8] = slave_status2[0];					// 485 비상문으로 부터 받은 비번 카드 문열림 상태
							
							Slave_Answer_Proc(src_id, 0xA2, send_str, 9);
							
							flg_nfc_tag = 0;
						}
						else
						{
							send_str[0] = emdoor_state;			// OK
							send_str[1] = (bat_volt_int & 0xFF00) >> 8;
							send_str[2] = (bat_volt_int & 0x00FF);
							send_str[3] = emlock_out_state;
							send_str[4] = slave_status2[0];					// 485 비상문으로 부터 받은 비번 카드 문열림 상태
							
							Slave_Answer_Proc(src_id, buffer[4], send_str, 5);
						}
						//mprintf("Sending %s\r\n", send_buffer);
						break;
					case 0xB2:							// 비상문 도어 제어
						if(buffer[6] == 1)				// 문 열림
						{
							if((emdoor_state & 0x01) == 0)		// 닫혀 있다면
							{
								do_timeout = 10;		// 1Sec
								DO_ON;
								
								flg_rmt_door_access = 1;
							}
							flg_emlock_out = 0;
							send_str[0] = 0x01;
						}
						else							// 문 닫힘
						{
							if(flg_remote_close)
							{
								if(emdoor_state & 0x01)	// 열려있다면
								{
									do_timeout = 10;		// 1Sec
									DO_ON;
								}
								flg_emlock_out = 1;
								send_str[0] = 0x01;
							}
							else
								send_str[0] = 0x02;
						}
						Slave_Answer_Proc(src_id, buffer[4], send_str, 1);
						break;
					case 0xB3:							// 비상모드 해제 
						if(flg_alarm_remote_release)
						{
							send_str[0] = 0x01;			// OK
							flg_warnoff_command = 1;
						}
						else
							send_str[0] = 0x02;			
						Slave_Answer_Proc(src_id, buffer[4], send_str, 1);
						break;
					case 0xC2:							// 비밀번호 변경
						e2p_passwd = (buffer[6] * 1000) + (buffer[7] * 100) + (buffer[8] * 10) + buffer[9];
						FlashRom_WriteData();
						send_str[0] = 0x01;			// OK
						Slave_Answer_Proc(src_id, buffer[4], send_str, 1);
						rs485_retry_count = 0;
						flg_password_command = 1;				// RS485 Password Command
						break;
					case 0xE1:										// 카드정보
						card_data_count = (uint16_t)(buffer[6] << 8) + buffer[7];
						send_str[0] = 0x01;			// OK
						Slave_Answer_Proc(src_id, buffer[4], send_str, 1);
						card_recv_count = 0;
						break;
					case 0xE2:		// 카드 데이터
						rcv_count = buffer[5] / sizeof(TCarddata_rec);
						if((card_recv_count + rcv_count) <= card_data_count)
						{
							memcpy(&card_data[card_recv_count], &buffer[6], buffer[5]);
							card_recv_count += rcv_count;
							send_str[0] = 0x01;			// OK
						}
						else
							send_str[0] = 0x02;			// FAIL

						Slave_Answer_Proc(src_id, buffer[4], send_str, 1);
						break;
					case 0xE3:		// 카드 정보 Down 종료
						send_str[0] = 0x01;			// OK
						Slave_Answer_Proc(src_id, buffer[4], send_str, 1);
						//Master_Card_Write();
						rs485_card_data_count = card_data_count;
						flg_card_batch_command = 1;			// RS485 Card batch Command
						cb_num = 0;
						break;
					case 0x88:			// DateTime Set	응답 없음
						DateTimeStruct.years = buffer[6];
						DateTimeStruct.month = buffer[7];
						DateTimeStruct.date = buffer[8];
						DateTimeStruct.hours = buffer[9];
						DateTimeStruct.minutes = buffer[10];
						DateTimeStruct.sec = buffer[11];
						flg_datetime_command = 1;			// RS485 Date Time Set
						break;
					default:
						break;
				}
			}
		}
	}
}

uint16_t Get_Cardlist_Count(void)
{
	uint16_t i, j, ret_var;
	uint32_t mask, read_value;
	
	ret_var = 0;
	
	for(i=0; i<16; i++)
	{
		mask = 1;
		for(j=0; j<32; j++)
		{
			if(Flashcardrec.e2p_card_used[i] & mask)
			{
				if(Flashcardrec.e2p_bank_num == 0)
					read_value = *(volatile uint32_t *)(FLASH_CARD_BANK1_ADDR + (((i * 32) + j) * 4));
				else
					read_value = *(volatile uint32_t *)(FLASH_CARD_BANK2_ADDR + (((i * 32) + j) * 4));
				
				card_data[ret_var].card_num = (i * 32) + j + 1;
				card_data[ret_var].card_data = read_value;

				ret_var++;
			}
			mask <<= 1;
		}
	}
	return ret_var;
}
//===========================================================================================
void Slave_Answer_Proc(uint16_t dest_id, uint8_t command, uint8_t *data_str, uint8_t data_len)
{
	uint8_t i;
	uint16_t make_crc, send_check_count;
	
	memset(buffer, 0, sizeof(buffer));
	
	buffer[0] = (uint8_t)((Flashdatarec.e2p_id & 0xFF00) >> 8);	// Srouce ID
	buffer[1] = (uint8_t)(Flashdatarec.e2p_id & 0x00FF);
	
	buffer[2] = (uint8_t)((dest_id & 0xFF00) >> 8);				// Dest   ID
	buffer[3] = (uint8_t)(dest_id & 0x00FF);
	
	buffer[4] = command;				// Command
	buffer[5] = data_len;
	
	for(i=0; i<data_len; i++)
		buffer[6+i] = *data_str++;		// Command
	
	i = data_len + 6;

	make_crc = MakeCCITT16(buffer, i);
	buffer[i++] = (uint8_t)(make_crc >> 8);
	buffer[i++] = (uint8_t)(make_crc & 0x00FF);

	//HAL_Delay(10);
	send_check_count = 10;
	
	while(1)
	{
		HAL_Delay(50);
		Get_Rssi();
		
		if(rcv_rssi < Flashdatarec.e2p_rssi)
		{
			vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, buffer, pRadioConfiguration->Radio_PacketLength);
			LORATXLED_ON;
			Im_sending = 1;
			break;
		}
		
		send_check_count--;
		if(send_check_count == 0)
			break;
	}
}

void Get_Rssi(void)
{
	uint8_t rssi;
	
	si446x_get_modem_status(0xFF);
	rssi = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;
	rcv_rssi = rssi;
	
	if(max_rssi < rssi)
		max_rssi = rssi;
	
	if(flg_update_rssi)
	{
		flg_update_rssi = 0;
		cur_rssi = max_rssi;
		max_rssi = 0;
	}
}
//=====================================================================================
// Master 측에서 실행
void Card_Data_down_proc(void)
{
	uint16_t i, make_crc, send_size;
	uint16_t send_check_count;
	
	switch(batch_num)
	{
		case 0:
			if(Flashdatarec.e2p_slave_count > 0)
				card_down_id = 1;
			else
			{
				flg_card_batch = 0;
				return;
			}
			
			target_id = card_down_id;
			batch_num = 1;
			break;
		case 1:
			// Send ready for card data  카드정보가 전송되니까 수신 준비 해라
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = (uint8_t)((card_down_id & 0xFF00) >> 8);		// Dest   ID
			buffer[3] = (uint8_t)(card_down_id & 0x00FF);
		
			
			buffer[4] = 0xE1;					// Command
			buffer[5] = 2;						// Data Length
			buffer[6] = (uint8_t)((card_data_count & 0xFF00) >> 8);
			buffer[7] = (uint8_t)(card_data_count & 0x00FF);

			make_crc = MakeCCITT16(buffer, 8);
			buffer[8] = (uint8_t)(make_crc >> 8);
			buffer[9] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					card_send_index = 0;
					send_timeout = 20;			// 200ms
					batch_num = 2;
					flg_reponse = 0;
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
			break;
		case 2:
			if(flg_reponse && (buffer[4] == 0xE1))
			{
				flg_reponse = 0;
				retry_stat[card_down_id] = 0;
				batch_num = 3;
			}
			else if(send_timeout == 0)
			{
				if(retry_stat[card_down_id] < 3)
				{
					retry_stat[card_down_id]++;
					batch_num = 1;
				}
				else
				{
					comm_stat[card_down_id] = 1;
					slave_status[card_down_id] |= 0x80;		// 통신 불량
					batch_num = 7;
				}
			}
			break;
		case 3:
			flg_reponse = 0;
			
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = (uint8_t)((card_down_id & 0xFF00) >> 8);		// Dest   ID
			buffer[3] = (uint8_t)(card_down_id & 0x00FF);
			
			buffer[4] = 0xE2;					// Command
			
			if((card_data_count - card_send_index) > 9)
				send_size = 9;
			else
				send_size = card_data_count - card_send_index;
			
			
			buffer[5] = send_size * sizeof(TCarddata_rec);						// Data Length
			
			memcpy(&buffer[6], &card_data[card_send_index], buffer[5]);
			prev_send_index = card_send_index;
			card_send_index += send_size;

			make_crc = MakeCCITT16(buffer, buffer[5] + 6);
			i= buffer[5] + 6;
			buffer[i++] = (uint8_t)(make_crc >> 8);
			buffer[i++] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					send_timeout = 20;			// 200ms
					batch_num = 4;
					flg_reponse = 0;
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
			break;
		case 4:
			if(flg_reponse && (buffer[4] == 0xE2))
			{
				flg_reponse = 0;
				retry_stat[card_down_id] = 0;
				if(card_data_count == card_send_index)
					batch_num = 5;
				else
					batch_num = 3;
			}
			else if(send_timeout == 0)
			{
				if(retry_stat[card_down_id] < 3)
				{
					retry_stat[card_down_id]++;
					card_send_index = prev_send_index;
					batch_num = 3;
				}
				else
				{
					comm_stat[card_down_id] = 1;
					slave_status[card_down_id] |= 0x80;		// 통신 불량
					batch_num = 7;
				}
			}
			break;
		case 5:
			flg_reponse = 0;
			
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = (uint8_t)((card_down_id & 0xFF00) >> 8);		// Dest   ID
			buffer[3] = (uint8_t)(card_down_id & 0x00FF);
			
			buffer[4] = 0xE3;					// Command
			buffer[5] = 0x00;
			
			make_crc = MakeCCITT16(buffer, 6);
			buffer[6] = (uint8_t)(make_crc >> 8);
			buffer[7] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					send_timeout = 20;			// 200ms
					batch_num = 6;
					flg_reponse = 0;
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
			break;
		case 6:
			if(flg_reponse && (buffer[4] == 0xE3))
			{
				flg_reponse = 0;
				retry_stat[card_down_id] = 0;
				batch_num = 7;
			}
			else if(send_timeout == 0)
			{
				if(retry_stat[card_down_id] < 3)
				{
					retry_stat[card_down_id]++;
					batch_num = 5;
				}
				else
				{
					comm_stat[card_down_id] = 1;
					slave_status[card_down_id] |= 0x80;		// 통신 불량
					batch_num = 7;
				}
			}
			break;
		case 7:
			if(card_down_id < Flashdatarec.e2p_slave_count)
			{
				card_down_id++;
				target_id = card_down_id;
				batch_num = 1;
			}
			else
			{
				flg_card_batch = 0;
				batch_num = 99;
			}
			break;
		default:
			break;
	}
}
//=====================================================================================
// 비밀번호 업데이트
void Pass_Data_down_proc(void)
{
	uint16_t make_crc, send_check_count;
	
	switch(pass_num)
	{
		case 0:
			if(Flashdatarec.e2p_slave_count > 0)
				pass_down_id = 1;
			else
			{
				flg_password_batch = 0;
				return;
			}
			
			target_id = pass_down_id;
			pass_num = 1;
			break;
		case 1:
			buffer[0] = 0x00;	// Srouce ID
			buffer[1] = 0x00;
			
			buffer[2] = (uint8_t)((pass_down_id & 0xFF00) >> 8);		// Dest   ID
			buffer[3] = (uint8_t)(pass_down_id & 0x00FF);
		
			
			buffer[4] = 0xC2;					// Command
			buffer[5] = 4;						// Data Length
			buffer[6] = e2p_passwd / 1000;
			buffer[7] = (e2p_passwd % 1000) / 100;
			buffer[8] = (e2p_passwd % 100) / 10;
			buffer[9] = e2p_passwd % 10;

			make_crc = MakeCCITT16(buffer, 10);
			buffer[10] = (uint8_t)(make_crc >> 8);
			buffer[11] = (uint8_t)(make_crc & 0x00FF);

			send_check_count = 10;
			
			while(1)
			{
				HAL_Delay(50);
				Get_Rssi();
				
				if(rcv_rssi < Flashdatarec.e2p_rssi)
				{
					vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
					LORATXLED_ON;
					send_timeout = 20;			// 200ms
					pass_num = 2;
					flg_reponse = 0;
					break;
				}
				
				send_check_count--;
				if(send_check_count == 0)
					break;
			}
			break;
		case 2:
			if(flg_reponse && (buffer[4] == 0xC2))
			{
				flg_reponse = 0;
				retry_stat[pass_down_id] = 0;
				pass_num = 3;
			}
			else if(send_timeout == 0)
			{
				if(retry_stat[pass_down_id] < 3)
				{
					retry_stat[pass_down_id]++;
					pass_num = 1;
				}
				else
				{
					comm_stat[pass_down_id] = 1;
					slave_status[pass_down_id] |= 0x80;		// 통신 불량
					pass_num = 3;
				}
			}
			break;
		case 3:
			if(pass_down_id < Flashdatarec.e2p_slave_count)
			{
				pass_down_id++;
				target_id = pass_down_id;
				pass_num = 1;
			}
			else
			{
				flg_password_batch = 0;
				pass_num = 99;
			}
			break;
		default:
			break;
	}
}
//=====================================================================================
/*
void Card_Data_read_proc(void)
{
	uint16_t make_crc;
	uint8_t  card_receive_count;
	
	switch(read_num)
	{
		case 0:
			card_read_id = target_slave_number;
			
			target_id = card_read_id;
			read_num = 1;
			break;
		case 1:
			if(card_read_id == 0)			// Master 일 경우
			{
				Master_Card_Read();
				flg_reponse = 1;
				read_num = 4;
			}
			else
			{	// 카드데이터 읽기 준비
				buffer[0] = Flashdatarec.e2p_id;	// Srouce ID
				buffer[1] = card_read_id;			// Dest   ID
				
				buffer[2] = 0xE0;					// Command 카드정보 요청
				buffer[3] = 0;						// Data Length
				buffer[4] = 0;

				make_crc = MakeCCITT16(buffer, 5);
				buffer[5] = (uint8_t)(make_crc >> 8);
				buffer[6] = (uint8_t)(make_crc & 0x00FF);

				vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
				LORATXLED_ON;
				send_timeout = 20;			// 200ms
				read_num = 2;
				flg_reponse = 0;
			}
			break;
		case 2:
			if(flg_reponse)
			{
				flg_reponse = 0;
				
				if(buffer[2] == 0xE0)				// 카드정보
				{
					card_data_count = (uint16_t)(buffer[4] << 8) + buffer[5];
					
					if(card_data_count > 0)
					{
						buffer[0] = Flashdatarec.e2p_id;	// Srouce ID
						buffer[1] = card_read_id;			// Dest   ID
						buffer[2] = 0xE1;					// Card Data 요청
						buffer[3] = 0;
						buffer[4] = 0;

						make_crc = MakeCCITT16(buffer, 5);
						buffer[5] = (uint8_t)(make_crc >> 8);
						buffer[6] = (uint8_t)(make_crc & 0x00FF);

						vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
						LORATXLED_ON;
						send_timeout = 20;			// 200ms
						card_rcv_index = 0;
						read_num = 3;
					}
					else
					{
						flg_cardread_batch = 0;
						// 카드 정보만 전송
					}
				}
			}
			break;
		case 3:
			if(flg_reponse)
			{
				flg_reponse = 0;

				if(buffer[2] == 0xE1)				// 카드 Data
				{
					card_receive_count = buffer[3] / sizeof(TCarddata_rec);
					if((card_rcv_index + card_receive_count) <= card_data_count)
					{
						memcpy(&card_data[card_rcv_index], &buffer[4], buffer[3]);
						card_rcv_index += card_receive_count;
						
						buffer[0] = Flashdatarec.e2p_id;	// Srouce ID
						buffer[1] = card_read_id;			// Dest   ID
						buffer[2] = 0xE1;					// Card Data 요청
						buffer[3] = 0;
						buffer[4] = 0;

						make_crc = MakeCCITT16(buffer, 5);
						buffer[5] = (uint8_t)(make_crc >> 8);
						buffer[6] = (uint8_t)(make_crc & 0x00FF);

						vRadio_StartTx_Variable_Packet(pRadioConfiguration->Radio_ChannelNumber, &buffer[0], pRadioConfiguration->Radio_PacketLength);
						LORATXLED_ON;
						send_timeout = 20;			// 200ms
						card_rcv_index = 0;
					}
				}
				else if(buffer[2] == 0xE2)			// 카드 Data 종료
				{
					flg_cardread_batch = 0;
					read_num = 99;
					// card read 종료
				}
			}
			break;
		default:
			break;
	}
}
*/
//============================================================================================

