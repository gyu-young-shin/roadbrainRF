#ifndef	__W5100S_H
#define	__W5100S_H
#include "main.h"

#define ETH_MAX_BUF_SIZE	512
#define TCP_SOCKET_PORT_NUM	10000
#define MODBUS_DATA_SIZE	8192

#define	_TCPSERVER_DEBUG_

/* DATA_BUF_SIZE define for Loopback example */
#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE	256
#endif


void Wiznet_W5100S_Init(void);
void Set_MAC_Address(void);
void Tcp_Proc(void);
void Tcp_Client_Proc(uint8_t sn);
void print_network_information(void);

void Ethernet_Proc(int32_t recv_len);
void Send_Ethernet_Packet(const char *str_buf);

uint16_t Swap_int16(uint16_t swap_var);
uint32_t Swap_int32(uint32_t swap_var);

void Ether_UserCommand_Proc(int32_t recv_len);					// 사용자 명령어 처리
void eth_cmd_help(char *par);
void eth_cmd_ip(char *par);
void eth_cmd_gw(char *par);

void eth_cmd_sip(char *par);
void eth_cmd_sport(char *par);
void eth_cmd_ssid(char *par);
void eth_cmd_spwd(char *par);
void eth_cmd_baud(char *par);
void eth_cmd_mrs(char *par);
void eth_cmd_mdi(char *par);
void eth_cmd_mrtu(char *par);
void eth_cmd_mtcp(char *par);
void eth_cmd_info(char *par);
void eth_cmd_reset(char *par);
void eth_cmd_ccnt(char *par);
void eth_cmd_wifimac(char *par);
void eth_cmd_id(char *par);

void eth_cmd_rtutcp(char *par);
void eth_cmd_selnum(char *par);
void eth_cmd_rtubaud(char *par);
void eth_cmd_tcpsip(char *par);
void eth_cmd_tcpport(char *par);
void eth_cmd_rtuid(char *par);
void eth_cmd_fcode(char *par);
void eth_cmd_addr(char *par);
void eth_cmd_length(char *par);
void eth_cmd_enable(char *par);
void eth_cmd_rtuinfo(char *par);
void eth_cmd_tcpinfo(char *par);
void eth_cmd_rtuint(char *par);
void eth_cmd_tcpint(char *par);
void eth_cmd_inttime(char *par);

void View_Packet_Data(uint8_t rtutcp_flag, uint8_t view_num);

void Modbus_Tcp_Proc(uint8_t sn);
void Modbus_Tcp_Recv_Proc(uint32_t recv_len);
void Tcp_Request_Packet(uint8_t sn);
uint8_t Select_Client_index(void);
uint8_t Compare_Next_IP(void);

#endif
