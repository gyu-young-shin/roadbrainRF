#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include "flashrom.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "dhcp.h"
#include "dns.h"
#include "W5100SRelFunctions.h"
#include "w5100s_proc.h"
#include "debug.h"
#include "repeater.h"
#include "wifi.h"
#include "rs485.h"


wiz_NetInfo gWIZNETINFO = { .mac = {0x00,0x0A,0xDC,0x57,0xf7,0x88},
							.ip = {192,168,0,100},
							.sn = {255, 255, 255, 0},
							.gw = {192, 168, 0, 1},
							.dns = {168, 126, 63, 1},
							//.dns = {8, 8, 8, 8},
							.dhcp = NETINFO_STATIC};
//                            .dhcp = NETINFO_DHCP};

_Bool	flg_test232 = 0;
_Bool	flg_test485 = 0;
_Bool	flg_testwifi = 0;
_Bool	flg_testin = 0;
_Bool	flg_testout = 0;
							
							
_Bool	flg_w5100s_init_ok = 0;
_Bool	flg_ethernet_connected = 0;
_Bool	flg_mod_tcp_connected = 0;
_Bool	flg_mrs_enable = 0;
_Bool	flg_mdi_enable = 0;
_Bool	flg_mrtu_enable = 0;
_Bool	flg_mrtu_485_enable = 0;
_Bool	flg_mtcp_enable = 0;
_Bool	flg_reset_enable = 0;
_Bool	flg_next_client = 1;
			
uint8_t	ethBuf0[ETH_MAX_BUF_SIZE];
uint8_t	ethBuf1[ETH_MAX_BUF_SIZE];
							
char 	eth_sendbuf[ETH_MAX_BUF_SIZE];
int8_t	eth_line[512];

uint16_t	ether_timeout = 0;
uint16_t	ethmod_timeout = 0;
uint8_t		mod_tcp_indx = 0;
uint8_t		eth_retry_count	= 0;
uint8_t		mod_retry_count	= 0;

uint8_t		modbus_send_buf[64];

uint16_t	eth_req_int_timeout = 0;
uint16_t	send_232int_timeout = 0;
uint16_t	send_485int_timeout = 0;

uint8_t		packet_data_num	= 0;

const	char eth_help[] =
	"\n\r\n\r\n\r"
	"+-----------------------------------------------------------------------+\n\r"
	"|                         Road Brain Command Help                       |\n\r"
	"|-----------------------------------------------------------------------|\n\r"
	"|  help            This infomation                                      |\n\r"
	"|  ip              Set local  ip address                                |\n\r"
	"|  gw              Set gateway ip address                               |\n\r"
	"|  sip             Set server ip address                                |\n\r"
	"|  sport           Set server port number                               |\n\r"
	"|  ssid            Set name string of wifi device                       |\n\r"
	"|  spwd            Set password string of wifi device                   |\n\r"
	"|  baud            Set baud rate of rs485                               |\n\r"
	"|  mrs             Monitoring RS485 input string                        |\n\r"
	"|  mdi             Monitoring DI input count                            |\n\r"
	"|  mrtu            Monitoring Modbus RTU                                |\n\r"
	"|  mtcp            Monitoring Modbus TCP                                |\n\r"
	"|  ESC+ENT         Stop Monitoring of mrs, mdi                          |\n\r"
	"|  info            Total parameter value listing                        |\n\r"
	"|  reset           Device Reset                                         |\n\r"
	"|  ccnt            Clear DI count value                                 |\n\r"
	"|  id              Set Device Identification                            |\n\r"
	"|  wmac            Set wifi MAC Address                                 |\n\r"
	"|  rtutcp          Select RTU or TCP                                    |\n\r"
	"|  selnum          Select packet data number                            |\n\r"
	"|  rtubaud         Set RTU baud rate                                    |\n\r"
	"|  tcpsip          Set Tcp destnation IP packet address                 |\n\r"
	"|  tcpport         Set Tcp destnation port number                       |\n\r"
	"|  rtuid           Set id of RTU                                        |\n\r"
	"|  fcode           Set Function Code                                    |\n\r"
	"|  addr            Set packet data address                              |\n\r"
	"|  length          Set packet data length                               |\n\r"
	"|  enable          Set packet data enable                               |\n\r"
	"|  rtuinfo         View RTU packet data infomation                      |\n\r"
	"|  tcpinfo         View TCP packet data infomation                      |\n\r"
	"|  rtuint          RTU request interval time                            |\n\r"
	"|  tcpint          TCP request interval time                            |\n\r"
	"|  inttime         RS485 or RS232 send interval time                    |\n\r"
	"+-----------------------------------------------------------------------+\n\r"
	"\n\r\n\r\n\r\0";

const SCMD eth_cmd[] = 
{
	"help",		eth_cmd_help,
	"ip",		eth_cmd_ip,
	"gw",		eth_cmd_gw,
	"sip",		eth_cmd_sip,
	"sport",	eth_cmd_sport,
	"ssid",		eth_cmd_ssid,
	"spwd",		eth_cmd_spwd,
	"baud",		eth_cmd_baud,
	"mrs",		eth_cmd_mrs,
	"mdi",		eth_cmd_mdi,
	"mrtu",		eth_cmd_mrtu,
	"mtcp",		eth_cmd_mtcp,
	"info",		eth_cmd_info,
	"reset",	eth_cmd_reset,
	"ccnt",		eth_cmd_ccnt,
	"id",		eth_cmd_id,
	"wmac",		eth_cmd_wifimac,
	"rtutcp",	eth_cmd_rtutcp,	
	"selnum",	eth_cmd_selnum,	
	"rtubaud",	eth_cmd_rtubaud,		
	"tcpsip",	eth_cmd_tcpsip,
	"tcpport",	eth_cmd_tcpport,
	"rtuid",	eth_cmd_rtuid,
	"fcode",	eth_cmd_fcode,
	"addr",		eth_cmd_addr,
	"length",	eth_cmd_length,
	"enable",	eth_cmd_enable,
	"rtuinfo",	eth_cmd_rtuinfo,
	"tcpinfo",	eth_cmd_tcpinfo,
	"rtuint",	eth_cmd_rtuint,
	"tcpint",	eth_cmd_tcpint,
	"inttime",	eth_cmd_inttime,
};

#define ETH_CMD_COUNT (sizeof(eth_cmd) / sizeof(eth_cmd[0]))

//========================================================
extern	TE2PDataRec 	Flashdatarec;
extern	__IO uint32_t 	RegularConvData_Tab[4];
extern	CRC_HandleTypeDef hcrc;
extern	uint32_t 		pulse_in1_count;
extern	_Bool			flg_di1_event;

extern	uint8_t		wifi_send_buf[WBUF_COUNT][WBUF_LEN];
extern	uint16_t	wifi_send_len[WBUF_COUNT];
extern	uint16_t	wifi_rd_indx;
extern	uint16_t	wifi_wr_indx;

//========================================================
void Wiznet_W5100S_Init(void)
{
	W5100SReset();
	/* Indirect bus method callback registration */
	reg_wizchip_bus_cbfunc(busReadByte, busWriteByte);
	
	W5100SInitialze();
	mprintf("\nCHIP Version: %02x\n", getVER());
	if(getVER() != 0x51)
	{
		mprintf("Ethernet Chip Init Error!!\n");
		return;
	}
	
	if(gWIZNETINFO.dhcp == NETINFO_STATIC)
	{
		wizchip_setnetinfo(&gWIZNETINFO);
		flg_w5100s_init_ok = 1;
		mprintf("flg_w5100s_init_ok = 1\n");
	}
	
	mprintf("Register value after W5100S initialize!\r\n");
	print_network_information();
}
//-----------------------------------------------------------------------------------
void Set_MAC_Address(void)
{
	uint8_t i;
	uint32_t ret_crc;
	
	for(i=0; i<32; i++)
	{
		HAL_Delay(1);
		ret_crc = HAL_CRC_Accumulate(&hcrc, (uint32_t *)&RegularConvData_Tab[0], 1);
	}
	gWIZNETINFO.mac[2] = (uint8_t)((ret_crc & 0xFF000000) >> 24);
	gWIZNETINFO.mac[3] = (uint8_t)((ret_crc & 0x00FF0000) >> 16);
	gWIZNETINFO.mac[4] = (uint8_t)((ret_crc & 0x0000FF00) >> 8);
	gWIZNETINFO.mac[5] = (uint8_t)(ret_crc & 0x000000FF);

	for(i=0; i<32; i++)
	{
		HAL_Delay(1);
		ret_crc = HAL_CRC_Accumulate(&hcrc, (uint32_t *)&RegularConvData_Tab[0], 1);
	}
	gWIZNETINFO.mac[1] = (uint8_t)(ret_crc & 0x000000FF);
}
//-----------------------------------------------------------------------------------
void Tcp_Proc(void)
{
	uint8_t	sel_ret;
	
	if(flg_w5100s_init_ok == 0)		// Master
	{
		if(Flashdatarec.e2p_set_mac == 0)
		{
			Set_MAC_Address();
			
			Flashdatarec.e2p_mac_addr[1] = gWIZNETINFO.mac[1];
			Flashdatarec.e2p_mac_addr[2] = gWIZNETINFO.mac[2];
			Flashdatarec.e2p_mac_addr[3] = gWIZNETINFO.mac[3];
			Flashdatarec.e2p_mac_addr[4] = gWIZNETINFO.mac[4];
			Flashdatarec.e2p_mac_addr[5] = gWIZNETINFO.mac[5];
			
			Flashdatarec.e2p_local_ip[0] = gWIZNETINFO.ip[0];
			Flashdatarec.e2p_local_ip[1] = gWIZNETINFO.ip[1];
			Flashdatarec.e2p_local_ip[2] = gWIZNETINFO.ip[2];
			Flashdatarec.e2p_local_ip[3] = gWIZNETINFO.ip[3];

			Flashdatarec.e2p_local_gw[0] = gWIZNETINFO.gw[0];
			Flashdatarec.e2p_local_gw[1] = gWIZNETINFO.gw[1];
			Flashdatarec.e2p_local_gw[2] = gWIZNETINFO.gw[2];
			Flashdatarec.e2p_local_gw[3] = gWIZNETINFO.gw[3];
			
			Flashdatarec.e2p_set_mac = 1;
			FlashRom_WriteData();
		}
		else
		{
			gWIZNETINFO.mac[1] = Flashdatarec.e2p_mac_addr[1];
			gWIZNETINFO.mac[2] = Flashdatarec.e2p_mac_addr[2];
			gWIZNETINFO.mac[3] = Flashdatarec.e2p_mac_addr[3];
			gWIZNETINFO.mac[4] = Flashdatarec.e2p_mac_addr[4];
			gWIZNETINFO.mac[5] = Flashdatarec.e2p_mac_addr[5];

			gWIZNETINFO.ip[0] = Flashdatarec.e2p_local_ip[0];
			gWIZNETINFO.ip[1] = Flashdatarec.e2p_local_ip[1];
			gWIZNETINFO.ip[2] = Flashdatarec.e2p_local_ip[2];
			gWIZNETINFO.ip[3] = Flashdatarec.e2p_local_ip[3];

			gWIZNETINFO.gw[0] = Flashdatarec.e2p_local_gw[0];
			gWIZNETINFO.gw[1] = Flashdatarec.e2p_local_gw[1];
			gWIZNETINFO.gw[2] = Flashdatarec.e2p_local_gw[2];
			gWIZNETINFO.gw[3] = Flashdatarec.e2p_local_gw[3];
		}
		
		Wiznet_W5100S_Init();
	}
	else
	{
		if((ETHERNET_LINK == 1) && (flg_ethernet_connected))	// No Link && Connected
		{
			mprintf("No Link.. Socket Close\n");
			flg_ethernet_connected = 0;
			close(0);
		}
		else
		{
			Tcp_Client_Proc(0);							// FOR Debugging
			
			if(Flashdatarec.e2p_modbus_sel == 0)		// TCP
			{
				if(flg_next_client)
				{
					flg_next_client = 0;
					sel_ret = Select_Client_index();
					eth_req_int_timeout = Flashdatarec.e2p_req_interval;				// 1 sec
				}
				
				if(sel_ret)
				{
					if(eth_req_int_timeout == 0)
						Modbus_Tcp_Proc(1);
				}
				else
					flg_next_client = 1;
			}
			else
			{
				if(flg_mod_tcp_connected)
				{
					flg_mod_tcp_connected = 0;
					disconnect(1);
				}
				
				flg_next_client = 1;
			}
		}
	}
}
//-----------------------------------------------------------------------------------
void Tcp_Client_Proc(uint8_t sn)
{
	int32_t		ret;
	uint16_t	size;

	switch(getSn_SR(sn))
	{
		case SOCK_ESTABLISHED:
			ETHLED_ON;
		
			if(flg_ethernet_connected == 0)
			{
				mprintf("Soket %d Established.. \n", sn);
				sprintf(eth_sendbuf, "\n\rRoad Brain Device Ver %d.%d\n\rBuild Date: %s %s\n\rRoadBrain>", \
						VERSION_HIGH, VERSION_LOW, __DATE__, __TIME__);
				Send_Ethernet_Packet(eth_sendbuf);
			}
			
			flg_ethernet_connected = 1;
		
			if(getSn_IR(sn) & Sn_IR_CON)
			{
				setSn_IR(sn,Sn_IR_CON);
			}

			if((size = getSn_RX_RSR(sn)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
			{
				if(size > ETH_MAX_BUF_SIZE) 
					size = ETH_MAX_BUF_SIZE;

				ret = recv(sn, ethBuf0, size);

				if(ret <= 0) 
					return;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.

				ethBuf0[ret] = NULL;
				Ethernet_Proc(ret);
			}
			
			if(flg_mdi_enable)
			{
				if(flg_di1_event)
				{
					flg_di1_event = 0;
					
					sprintf(eth_sendbuf, "DI Count=%d\n\r", pulse_in1_count); 
					Send_Ethernet_Packet(eth_sendbuf); 
				}
			}
			
			break;
		case SOCK_CLOSE_WAIT :
			flg_ethernet_connected = 0;
			if((ret = disconnect(sn)) != SOCK_OK) 
				return;
			#ifdef _TCPSERVER_DEBUG_
				mprintf("%d:Socket Closed\n", sn);
			#endif
			break;
		case SOCK_INIT :
			#ifdef _TCPSERVER_DEBUG_
				//mprintf("%d:Listen, TCP server, port [%d]\r\n", sn, TCP_SOCKET_PORT_NUM);
			#endif
			if(listen(sn) != SOCK_OK) 
			{
				mprintf("Listen Error [%d]\n", sn);
				return;
			}
			break;
		case SOCK_CLOSED:
			flg_ethernet_connected = 0;
			ETHLED_OFF;
		
			if(socket(sn, Sn_MR_TCP, TCP_SOCKET_PORT_NUM, SF_IO_NONBLOCK) == sn)
				mprintf("Socket %d Opend\n", sn);
			else
				mprintf("Socket Open Error %d\n", sn);
			break;
		default:
			break;
	}
}
//-----------------------------------------------------------------------------------
void Ethernet_Proc(int32_t recv_len)
{
//	uint16_t	i;

//	for(i=0; i<recv_len; i++)
//		mprintf("%02X ", ethBuf0[i]);
//	mprintf("\n");
//	
//	send(sn, ethBuf0, recv_len);
//	mprintf("Ethernet: %d\n", recv_len);
	if(recv_len >= 2)
	{
		if((recv_len == 3) && (ethBuf0[0] == 0x1B))			// ESC Key
		{
			if(flg_mdi_enable)
			{
				flg_mdi_enable = 0;
				Send_Ethernet_Packet("mdi disabled..\n\r"); 
			}
			
			if(flg_mrs_enable)
			{
				flg_mrs_enable = 0;
				Send_Ethernet_Packet("mrs disabled..\n\r"); 
			}
			
			if(flg_mrtu_enable)
			{
				flg_mrtu_enable = 0;
				Send_Ethernet_Packet("mrtu disabled..\n\r"); 
			}

			if(flg_mtcp_enable)
			{
				flg_mtcp_enable = 0;
				Send_Ethernet_Packet("mtcp disabled..\n\r"); 
			}
		}
		else
			Ether_UserCommand_Proc(recv_len);
	}
	
}
//-----------------------------------------------------------------------------------
uint16_t Swap_int16(uint16_t swap_var)
{
	return (uint16_t)((swap_var & 0x00FF) << 8) | (swap_var >> 8);
}
//-----------------------------------------------------------------------------------
uint32_t Swap_int32(uint32_t swap_var)
{
	return (uint32_t)((swap_var & 0x000000FF) << 24) | (uint32_t)((swap_var & 0x0000FF00) << 8) | \
			(uint32_t)((swap_var & 0x00FF0000) >> 8) | (uint32_t)((swap_var & 0xFF000000) >> 24);
}
//-----------------------------------------------------------------------------------
void print_network_information(void)
{
	wizchip_getnetinfo(&gWIZNETINFO);
	mprintf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	mprintf("IP address : %d.%d.%d.%d\n",gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	mprintf("SM Mask	   : %d.%d.%d.%d\n",gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	mprintf("Gate way   : %d.%d.%d.%d\n",gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	mprintf("DNS Server : %d.%d.%d.%d\n",gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
}
//-----------------------------------------------------------------------------------
void Send_Ethernet_Packet(const char *str_buf)
{
	uint16_t send_cnt, sended_count, str_len;
	
	sended_count = 0;
	str_len = strlen(str_buf);
	
	while(sended_count < str_len)
	{
		if((str_len - sended_count) > 1024)
			send_cnt = send(0, (uint8_t *)(str_buf + sended_count), 1024);
		else
			send_cnt = send(0, (uint8_t *)(str_buf + sended_count), str_len - sended_count);
		
		sended_count +=  send_cnt;
	}
}
//----------------------------------------------------------------------------------------
void Ether_UserCommand_Proc(int32_t recv_len)						// �����? ���ɾ� ó��
{
	uint32_t i;
	char *sp, *next;

	sp = 0;
	next = 0;

	if((ethBuf0[recv_len - 1] == 0x0D) || (ethBuf0[recv_len - 1] == 0x0A))
		ethBuf0[recv_len - 1] = 0x00;
	
	if((ethBuf0[recv_len - 2] == 0x0D) || (ethBuf0[recv_len - 2] == 0x0A))
		ethBuf0[recv_len - 2] = 0x00;
	
	strcpy((char *)eth_line, (char *)ethBuf0);
	
	sp = get_entry((char *)eth_line, &next);

	if(*sp != 0)
	{
		for(i = 0; i < ETH_CMD_COUNT; i++) 
		{
			if(strcmp(sp, (const char *)&eth_cmd[i].val)) 
				continue;
			eth_cmd[i].func(next);                  
			break;
		}
		
		if(i == ETH_CMD_COUNT)
			Send_Ethernet_Packet("\n\rCommand error\n\r");
	}
	
	Send_Ethernet_Packet("\n\rRoadBrain>");
}
//--------------------------------------------------------------------------------------------
void eth_cmd_help(char *par)
{
	Send_Ethernet_Packet(eth_help);
}
//--------------------------------------------------------------------------------------------
void eth_cmd_ip(char *par)
{
	char *data_var, *next;
	uint8_t ip_addr[4] = {0,};

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("ip [xxx xxx xxx xxx] : 0~255\n\r"); 
		sprintf(eth_sendbuf, "ip=%d.%d.%d.%d\n\r", Flashdatarec.e2p_local_ip[0],Flashdatarec.e2p_local_ip[1],
									Flashdatarec.e2p_local_ip[2],Flashdatarec.e2p_local_ip[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if(atoi(data_var) <= 255)
			ip_addr[0] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;
		
		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[1] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[2] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[3] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;
		
		Flashdatarec.e2p_local_ip[0] = ip_addr[0];
		Flashdatarec.e2p_local_ip[1] = ip_addr[1];
		Flashdatarec.e2p_local_ip[2] = ip_addr[2];
		Flashdatarec.e2p_local_ip[3] = ip_addr[3];
		FlashRom_WriteData();

		sprintf(eth_sendbuf, "ip=%d.%d.%d.%d\n\r", Flashdatarec.e2p_local_ip[0],Flashdatarec.e2p_local_ip[1],
									Flashdatarec.e2p_local_ip[2],Flashdatarec.e2p_local_ip[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
		return;
		
		ETH_ERR_PARAM:
			Send_Ethernet_Packet("Parameter Error!!\n\r");

	}
}
//--------------------------------------------------------------------------------------------
void eth_cmd_gw(char *par)
{
	char *data_var, *next;
	uint8_t ip_addr[4] = {0,};

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("gw [xxx xxx xxx xxx] : 0~255\n\r"); 
		sprintf(eth_sendbuf, "gw=%d.%d.%d.%d\n\r", Flashdatarec.e2p_local_gw[0],Flashdatarec.e2p_local_gw[1],
									Flashdatarec.e2p_local_gw[2],Flashdatarec.e2p_local_gw[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if(atoi(data_var) <= 255)
			ip_addr[0] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;
		
		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[1] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[2] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[3] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;
		
		Flashdatarec.e2p_local_gw[0] = ip_addr[0];
		Flashdatarec.e2p_local_gw[1] = ip_addr[1];
		Flashdatarec.e2p_local_gw[2] = ip_addr[2];
		Flashdatarec.e2p_local_gw[3] = ip_addr[3];
		FlashRom_WriteData();

		sprintf(eth_sendbuf, "gw=%d.%d.%d.%d\n\r", Flashdatarec.e2p_local_gw[0],Flashdatarec.e2p_local_gw[1],
									Flashdatarec.e2p_local_gw[2],Flashdatarec.e2p_local_gw[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
		return;
		
		ETH_ERR_PARAM:
			Send_Ethernet_Packet("Parameter Error!!\n\r");

	}
}
//--------------------------------------------------------------------------------------------
void eth_cmd_sip(char *par)
{
	char *data_var, *next;
	uint8_t ip_addr[4] = {0,};

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("sip [xxx xxx xxx xxx] : 0~255\n\r"); 
		sprintf(eth_sendbuf, "sip=%d.%d.%d.%d\n\r", Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
									Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if(atoi(data_var) <= 255)
			ip_addr[0] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;
		
		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[1] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[2] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM;
		
		if(atoi(data_var) <= 255)
			ip_addr[3] = atoi(data_var);
		else
			goto ETH_ERR_PARAM;
		
		Flashdatarec.e2p_server_ip[0] = ip_addr[0];
		Flashdatarec.e2p_server_ip[1] = ip_addr[1];
		Flashdatarec.e2p_server_ip[2] = ip_addr[2];
		Flashdatarec.e2p_server_ip[3] = ip_addr[3];
		FlashRom_WriteData();

		sprintf(eth_sendbuf, "sip=%d.%d.%d.%d\n\r", Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
									Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
		return;
		
		ETH_ERR_PARAM:
			Send_Ethernet_Packet("Parameter Error!!\n\r");

	}
}
//--------------------------------------------------------------------------------------------
void eth_cmd_sport(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("sport [xxxxx] : 1024~65535\n\r"); 
		sprintf(eth_sendbuf, "sport=%d\n\r", Flashdatarec.e2p_server_port); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if((atoi(data_var) >= 1024) && (atoi(data_var) <= 65535))
		{
			Flashdatarec.e2p_server_port = atoi(data_var);
			FlashRom_WriteData();
			sprintf(eth_sendbuf, "sport=%d\n\r", Flashdatarec.e2p_server_port); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
			Send_Ethernet_Packet("Parameter Error!!\n\r");
	}
}
//--------------------------------------------------------------------------------------------
void eth_cmd_ssid(char *par)
{
	if(*par == NULL)
	{
		Send_Ethernet_Packet("ssid [""string""]\n\r"); 
		sprintf(eth_sendbuf, "ssid=\"%s\"\n\r", Flashdatarec.e2p_station_ssid); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if(strlen(par) > 19)
		{
			strncpy((char *)Flashdatarec.e2p_station_ssid, par, 19);
			Flashdatarec.e2p_station_ssid[19] = 0;
		}
		else
		{
			strncpy((char *)Flashdatarec.e2p_station_ssid, par, strlen(par));
			Flashdatarec.e2p_station_ssid[strlen(par)] = 0;
		}
		
		FlashRom_WriteData();
		sprintf(eth_sendbuf, "ssid=\"%s\"\n\r", Flashdatarec.e2p_station_ssid); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
}
//--------------------------------------------------------------------------------------------
void eth_cmd_spwd(char *par)
{
	if(*par == NULL)
	{
		Send_Ethernet_Packet("spwd [""string""]\n\r"); 
		sprintf(eth_sendbuf, "spwd=\"%s\"\n\r", Flashdatarec.e2p_station_pawd); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if(strlen(par) > 19)
		{
			strncpy((char *)Flashdatarec.e2p_station_pawd, par, 19);
			Flashdatarec.e2p_station_pawd[19] = 0;
		}
		else
		{
			strncpy((char *)Flashdatarec.e2p_station_pawd, par, strlen(par));
			Flashdatarec.e2p_station_pawd[strlen(par)] = 0;
		}
		
		FlashRom_WriteData();
		sprintf(eth_sendbuf, "spwd=\"%s\"\n\r", Flashdatarec.e2p_station_pawd); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_baud(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("baud [xxxxxx] : 2400~115200\n\r"); 
		sprintf(eth_sendbuf, "baud=%d bps\n\r", Flashdatarec.e2p_baudrate); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if((atoi(data_var) >= 2400) && (atoi(data_var) <= 115200))
		{
			Flashdatarec.e2p_baudrate = atoi(data_var);
			FlashRom_WriteData();
			sprintf(eth_sendbuf, "baud=%d bps\n\r", Flashdatarec.e2p_baudrate); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
			Send_Ethernet_Packet("Parameter Error!!\n\r");
	}
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_mrs(char *par)
{
	flg_mrs_enable = 1;
	Send_Ethernet_Packet("Monitoring start RS485!!\n\r");
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_mdi(char *par)
{
	flg_mdi_enable = 1;
	Send_Ethernet_Packet("Monitoring start DI input count!!\n\r");
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_mrtu(char *par)
{
	flg_mrtu_enable = 1;
	Send_Ethernet_Packet("Monitoring start RTU!!\n\r");
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_mtcp(char *par)
{
	flg_mtcp_enable = 1;
	Send_Ethernet_Packet("Monitoring start TCP!!\n\r");
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_info(char *par)
{
	sprintf(eth_sendbuf, "\n\rRoad Brain Device Ver %d.%d\n\rBuild Date: %s %s\n\r\n\r", \
			VERSION_HIGH, VERSION_LOW, __DATE__, __TIME__);
	Send_Ethernet_Packet(eth_sendbuf);
	
	sprintf(eth_sendbuf, "sip=%d.%d.%d.%d\n\r", Flashdatarec.e2p_server_ip[0],Flashdatarec.e2p_server_ip[1],
								Flashdatarec.e2p_server_ip[2],Flashdatarec.e2p_server_ip[3]); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "sport=%d\n\r", Flashdatarec.e2p_server_port); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "ssid=\"%s\"\n\r", Flashdatarec.e2p_station_ssid); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "spwd=\"%s\"\n\r", Flashdatarec.e2p_station_pawd); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "baud=%d bps\n\r", Flashdatarec.e2p_baudrate); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "rtubaud=%d bps\n\r", Flashdatarec.e2p_232_baudrate); 
	Send_Ethernet_Packet(eth_sendbuf); 
	
	if(Flashdatarec.e2p_modbus_sel == 0)
		sprintf(eth_sendbuf, "rtutcp: tcp\n\r");
	else if(Flashdatarec.e2p_modbus_sel == 1)
		sprintf(eth_sendbuf, "rtutcp: rtu\n\r");
	else 
		sprintf(eth_sendbuf, "rtutcp: 232\n\r");
	
	Send_Ethernet_Packet(eth_sendbuf);
	
	sprintf(eth_sendbuf, "tcpsip=%d.%d.%d.%d\n\r",
			Flashdatarec.e2p_mb_sip[0],
			Flashdatarec.e2p_mb_sip[1],
			Flashdatarec.e2p_mb_sip[2],
			Flashdatarec.e2p_mb_sip[3]); 
	Send_Ethernet_Packet(eth_sendbuf); 

	sprintf(eth_sendbuf, "tcp port=%d\n\r", Flashdatarec.e2p_mb_portnum); 
	Send_Ethernet_Packet(eth_sendbuf); 

	sprintf(eth_sendbuf, "RS485 send interval time: %d sec\n\r", Flashdatarec.e2p_485_int);
	Send_Ethernet_Packet(eth_sendbuf);		
	sprintf(eth_sendbuf, "RS232 send interval time: %d sec\n\r", Flashdatarec.e2p_232_int);
	Send_Ethernet_Packet(eth_sendbuf);	
	
	sprintf(eth_sendbuf, "count=%d EA\n\r", pulse_in1_count); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "device id=%d\n\r", Flashdatarec.e2p_id); 
	Send_Ethernet_Packet(eth_sendbuf); 
	sprintf(eth_sendbuf, "WMAC=%02X:%02X:%02X:%02X:%02X:%02X\n\r", Flashdatarec.e2p_mac_wifi[0],Flashdatarec.e2p_mac_wifi[1],
									Flashdatarec.e2p_mac_wifi[2],Flashdatarec.e2p_mac_wifi[3], Flashdatarec.e2p_mac_wifi[4],
									Flashdatarec.e2p_mac_wifi[5]); 
	Send_Ethernet_Packet(eth_sendbuf); 
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_reset(char *par)
{
	flg_reset_enable = 1;
	Send_Ethernet_Packet("Device reset\n\r"); 
}
//--------------------------------------------------------------------------------------------	
void eth_cmd_ccnt(char *par)
{
	pulse_in1_count = 0;
	sprintf(eth_sendbuf, "count=%d EA\n\r", pulse_in1_count); 
	Send_Ethernet_Packet(eth_sendbuf); 
}
//--------------------------------------------------------------------------------------------
void eth_cmd_id(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("id [xxxxx] : 0~42,949,672,96\n\r"); 
		sprintf(eth_sendbuf, "id=%d\n\r", Flashdatarec.e2p_id); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		Flashdatarec.e2p_id = atoi(data_var);
		FlashRom_WriteData();
		sprintf(eth_sendbuf, "id=%d\n\r", Flashdatarec.e2p_id); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
}
//----------------------------------------------------------------------------------------------------------------
void eth_cmd_wifimac(char *par)
{
	char *data_var, *next;
	uint8_t wifimac_addr[6] = {0,};

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		sprintf(eth_sendbuf, "WMAC [xx xx xx xx xx xx] : 0x00~0xFF\n\r");
		Send_Ethernet_Packet(eth_sendbuf);		
		sprintf(eth_sendbuf, "WMAC=%02X:%02X:%02X:%02X:%02X:%02X\n\r", Flashdatarec.e2p_mac_wifi[0],Flashdatarec.e2p_mac_wifi[1],
									Flashdatarec.e2p_mac_wifi[2],Flashdatarec.e2p_mac_wifi[3], Flashdatarec.e2p_mac_wifi[4],
									Flashdatarec.e2p_mac_wifi[5]); 
		Send_Ethernet_Packet(eth_sendbuf);
	}
	else
	{
		wifimac_addr[0] = strtoul(data_var, NULL, 16);
		
		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) 
			goto ERR_WMAC;
		wifimac_addr[1] = strtoul(data_var, NULL, 16);

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) 
			goto ERR_WMAC;
		wifimac_addr[2] = strtoul(data_var, NULL, 16);

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) 
			goto ERR_WMAC;
		wifimac_addr[3] = strtoul(data_var, NULL, 16);

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) 
			goto ERR_WMAC;
		wifimac_addr[4] = strtoul(data_var, NULL, 16);

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) 
			goto ERR_WMAC;
		wifimac_addr[5] = strtoul(data_var, NULL, 16);
		
		Flashdatarec.e2p_mac_wifi[0] = wifimac_addr[0];
		Flashdatarec.e2p_mac_wifi[1] = wifimac_addr[1];
		Flashdatarec.e2p_mac_wifi[2] = wifimac_addr[2];
		Flashdatarec.e2p_mac_wifi[3] = wifimac_addr[3];
		Flashdatarec.e2p_mac_wifi[4] = wifimac_addr[4];
		Flashdatarec.e2p_mac_wifi[5] = wifimac_addr[5];
		Flashdatarec.e2p_set_wifimac = 0;
		FlashRom_WriteData();

		sprintf(eth_sendbuf, "WMAC=%02X:%02X:%02X:%02X:%02X:%02X\n\r", Flashdatarec.e2p_mac_wifi[0],Flashdatarec.e2p_mac_wifi[1],
									Flashdatarec.e2p_mac_wifi[2],Flashdatarec.e2p_mac_wifi[3], Flashdatarec.e2p_mac_wifi[4],
									Flashdatarec.e2p_mac_wifi[5]); 

		Send_Ethernet_Packet(eth_sendbuf);
		return;
		
ERR_WMAC:
		mprintf("Parameter Error!!\n");

	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_rtutcp(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		sprintf(eth_sendbuf, "rtutcp [rtu232 | rtu485 | tcp | 232 | 485 ]\n\r");
		Send_Ethernet_Packet(eth_sendbuf);		
		// TCP:0, RTU232:1, 232: 2 RTU485:3 485:4
		if(Flashdatarec.e2p_modbus_sel == 0)
			sprintf(eth_sendbuf, "rtutcp: tcp\n\r");
		else if(Flashdatarec.e2p_modbus_sel == 1)
			sprintf(eth_sendbuf, "rtutcp: rtu232\n\r");
		else if(Flashdatarec.e2p_modbus_sel == 2)
			sprintf(eth_sendbuf, "rtutcp: 232\n\r");
		else if(Flashdatarec.e2p_modbus_sel == 3)
			sprintf(eth_sendbuf, "rtutcp: rtu485\n\r");
		else 
			sprintf(eth_sendbuf, "rtutcp: 485\n\r");
		
		Send_Ethernet_Packet(eth_sendbuf);
	}
	else
	{
		if(strncmp((const char *)data_var, "tcp", 3) == 0)
		{
			Flashdatarec.e2p_modbus_sel = 0;
			sprintf(eth_sendbuf, "rtutcp: TCP\n\r");
			FlashRom_WriteData();
		}
		else if(strncmp((const char *)data_var, "rtu232", 6) == 0)
		{
			Flashdatarec.e2p_modbus_sel = 1;
			sprintf(eth_sendbuf, "rtutcp: RTU 232\n\r");
			FlashRom_WriteData();
		}
		else if(strncmp((const char *)data_var, "232", 3) == 0)
		{
			Flashdatarec.e2p_modbus_sel = 2;
			sprintf(eth_sendbuf, "rtutcp: 232\n\r");
			FlashRom_WriteData();
		}
		else if(strncmp((const char *)data_var, "rtu485", 6) == 0)
		{
			Flashdatarec.e2p_modbus_sel = 3;
			sprintf(eth_sendbuf, "rtutcp: RTU 485\n\r");
			FlashRom_WriteData();
		}
		else if(strncmp((const char *)data_var, "485", 3) == 0)
		{
			Flashdatarec.e2p_modbus_sel = 4;
			sprintf(eth_sendbuf, "rtutcp: 485\n\r");
			FlashRom_WriteData();
		}
		else
		{
			sprintf(eth_sendbuf, "Invalid string %s\n\r", data_var);
		}
		
		Send_Ethernet_Packet(eth_sendbuf);		
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_selnum(char *par)
{
	uint8_t data_num;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		sprintf(eth_sendbuf, "selnum [1-30]\n\r");
		Send_Ethernet_Packet(eth_sendbuf);		

		sprintf(eth_sendbuf, "selnum: %d\n\r", packet_data_num + 1);
		Send_Ethernet_Packet(eth_sendbuf);
	}
	else
	{
		data_num = atoi((const char *)data_var);
		if((data_num > 0) && (data_num <= 30))
		{
			packet_data_num = data_num - 1;
			sprintf(eth_sendbuf, "Packet data number: %d\n\r", data_num);
			Send_Ethernet_Packet(eth_sendbuf);		
		}
		else
		{
			sprintf(eth_sendbuf, "Invalid number %d\n\r", data_num);
			Send_Ethernet_Packet(eth_sendbuf);		
		}
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_rtubaud(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("rtubaud [xxxxxx] : 2400~115200\n\r"); 
		sprintf(eth_sendbuf, "rtubaud=%d bps\n\r", Flashdatarec.e2p_232_baudrate); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if((atoi(data_var) >= 2400) && (atoi(data_var) <= 115200))
		{
			Flashdatarec.e2p_232_baudrate = atoi(data_var);
			FlashRom_WriteData();
			sprintf(eth_sendbuf, "rtubaud=%d bps\n\r", Flashdatarec.e2p_232_baudrate); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
			Send_Ethernet_Packet("Parameter Error!!\n\r");
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_tcpsip(char *par)
{
	char *data_var, *next;
	uint8_t ip_addr[4] = {0,};

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("tcpsip [xxx xxx xxx xxx] : 0~255\n\r"); 
		sprintf(eth_sendbuf, "tcpsip=%d.%d.%d.%d\n\r",
				Flashdatarec.e2p_mb_sip[0],
				Flashdatarec.e2p_mb_sip[1],
				Flashdatarec.e2p_mb_sip[2],
				Flashdatarec.e2p_mb_sip[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if(atoi(data_var) <= 255)
			ip_addr[0] = atoi(data_var);
		else
			goto ETH_ERR_PARAM1;
		
		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM1;
		
		if(atoi(data_var) <= 255)
			ip_addr[1] = atoi(data_var);
		else
			goto ETH_ERR_PARAM1;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM1;
		
		if(atoi(data_var) <= 255)
			ip_addr[2] = atoi(data_var);
		else
			goto ETH_ERR_PARAM1;

		data_var = next;
		data_var = get_entry(data_var, &next);
		if(data_var == NULL) goto ETH_ERR_PARAM1;
		
		if(atoi(data_var) <= 255)
			ip_addr[3] = atoi(data_var);
		else
			goto ETH_ERR_PARAM1;
		
		Flashdatarec.e2p_mb_sip[0] = ip_addr[0];
		Flashdatarec.e2p_mb_sip[1] = ip_addr[1];
		Flashdatarec.e2p_mb_sip[2] = ip_addr[2];
		Flashdatarec.e2p_mb_sip[3] = ip_addr[3];
		FlashRom_WriteData();

		sprintf(eth_sendbuf, "tcpsip=%d.%d.%d.%d\n\r",
				Flashdatarec.e2p_mb_sip[0],
				Flashdatarec.e2p_mb_sip[1],
				Flashdatarec.e2p_mb_sip[2],
				Flashdatarec.e2p_mb_sip[3]); 
		Send_Ethernet_Packet(eth_sendbuf); 
		return;
		
		ETH_ERR_PARAM1:
			Send_Ethernet_Packet("Parameter Error!!\n\r");
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_tcpport(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("tcp port [xxxxx] : 1~65535\n\r"); 
		sprintf(eth_sendbuf, "tcp port=%d\n\r", Flashdatarec.e2p_mb_portnum); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		if((atoi(data_var) > 0) && (atoi(data_var) <= 65535))
		{
			Flashdatarec.e2p_mb_portnum = atoi(data_var);
			FlashRom_WriteData();
			sprintf(eth_sendbuf, "tcp port=%d\n\r", Flashdatarec.e2p_mb_portnum); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
			Send_Ethernet_Packet("Parameter Error!!\n\r");
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_rtuid(char *par)
{
	uint8_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("rtuid [xxxxx] : 1~254\n\r"); 
		sprintf(eth_sendbuf, "packet num: %d\n\rrtuid=%d\n\r", packet_data_num + 1, 
			Flashdatarec.e2p_mod_rtu[packet_data_num].mb_id); 
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var > 0) && (tmp_var < 255))
		{
			Flashdatarec.e2p_mod_rtu[packet_data_num].mb_id = tmp_var;
			FlashRom_WriteData();
			sprintf(eth_sendbuf, "packet num: %d\n\rrtuid=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_id); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("rtuid parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_fcode(char *par)
{
	uint8_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("fcode [1 | 2 | 3 | 4]\n\r"); 
		if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
			sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\rfcode=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_fcode); 
		else
			sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\rfcode=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_fcode); 
			
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var >= 1) || (tmp_var <= 4))
		{
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_fcode = tmp_var;
			else
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_fcode = tmp_var;
				
			FlashRom_WriteData();
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\rfcode=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_rtu[packet_data_num].mb_fcode); 
			else
				sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\rfcode=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_tcp[packet_data_num].mb_fcode); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("fcode parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_addr(char *par)
{
	uint16_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("addr [0-65535]\n\r"); 
		if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
			sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\raddr=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_address); 
		else
			sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\raddr=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_address); 
			
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if(tmp_var <= 65535)
		{
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_address = tmp_var;
			else
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_address = tmp_var;
				
			FlashRom_WriteData();
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\raddr=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_rtu[packet_data_num].mb_address); 
			else
				sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\raddr=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_tcp[packet_data_num].mb_address); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("address parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_length(char *par)
{
	uint16_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("length [0-65535]\n\r"); 
		if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
			sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\rlength=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_length); 
		else
			sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\rlength=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_length); 
			
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if(tmp_var <= 65535)
		{
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_length = tmp_var;
			else
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_length = tmp_var;
				
			FlashRom_WriteData();
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\rlength=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_rtu[packet_data_num].mb_length); 
			else
				sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\rlength=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_tcp[packet_data_num].mb_length); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("length parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_enable(char *par)
{
	uint8_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("enable [0 | 1]\n\r"); 
		if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
			sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\renable=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_enable); 
		else
			sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\renable=%d\n\r", packet_data_num + 1, 
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_enable); 
			
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var == 0) || (tmp_var == 1))
		{
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				Flashdatarec.e2p_mod_rtu[packet_data_num].mb_enable = tmp_var;
			else
				Flashdatarec.e2p_mod_tcp[packet_data_num].mb_enable = tmp_var;
				
			FlashRom_WriteData();
			if(Flashdatarec.e2p_modbus_sel == 1)		// RTU
				sprintf(eth_sendbuf, "RTU:\n\rpacket num: %d\n\renable=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_rtu[packet_data_num].mb_enable); 
			else
				sprintf(eth_sendbuf, "TCP:\n\rpacket num: %d\n\renable=%d\n\r", packet_data_num + 1, 
					Flashdatarec.e2p_mod_tcp[packet_data_num].mb_enable); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("enable parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_rtuinfo(char *par)
{
	uint8_t i, tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("RTU packet data information\n\r"); 
		Send_Ethernet_Packet("-----------------------------------------------------------------\n\r"); 
		for(i=0; i<CLIENT_COUNT; i++)
			View_Packet_Data(1, i); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var > 0) && (tmp_var <= CLIENT_COUNT))
		{
			View_Packet_Data(1, tmp_var - 1); 
		}
		else
			Send_Ethernet_Packet("rtuinfo number parameter error!!\n\r");			
	}
}
//-------------------------------------------------------------------------------------------
void eth_cmd_tcpinfo(char *par)
{
	uint8_t i, tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("TCP packet data information\n\r"); 
		Send_Ethernet_Packet("-----------------------------------------------------------------\n\r"); 
		for(i=0; i<CLIENT_COUNT; i++)
			View_Packet_Data(0, i); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var > 0) && (tmp_var <= CLIENT_COUNT))
		{
			View_Packet_Data(0, tmp_var - 1); 
		}
		else
			Send_Ethernet_Packet("tcpinfo number parameter error!!\n\r");			
	}
}
//-------------------------------------------------------------------------------------
void eth_cmd_rtuint(char *par)
{
	uint16_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("rtuint [5-100]\n\r"); 
		sprintf(eth_sendbuf, "rtu request interval: %d  x 100ms\r\n", Flashdatarec.e2p_mod_req_inter); 
			
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var >= 5) && (tmp_var <= 100))
		{
			Flashdatarec.e2p_mod_req_inter = tmp_var;
			FlashRom_WriteData();
			
			sprintf(eth_sendbuf, "rtu request interval: %d  x 100ms\r\n", Flashdatarec.e2p_mod_req_inter); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("rtuint parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------
void eth_cmd_tcpint(char *par)
{
	uint16_t tmp_var;
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		Send_Ethernet_Packet("tcpint [5-100]\n\r"); 
		sprintf(eth_sendbuf, "tcp request interval: %d  x 100ms\r\n", Flashdatarec.e2p_req_interval); 
			
		Send_Ethernet_Packet(eth_sendbuf); 
	}
	else
	{
		tmp_var = atoi(data_var);
		if((tmp_var >= 5) && (tmp_var <= 100))
		{
			Flashdatarec.e2p_req_interval = tmp_var;
			FlashRom_WriteData();
			
			sprintf(eth_sendbuf, "tcp request interval: %d  x 100ms\r\n", Flashdatarec.e2p_req_interval); 
			Send_Ethernet_Packet(eth_sendbuf); 
		}
		else
		{
			Send_Ethernet_Packet("tcpint parameter error!!\n\r");			
		}
	}
}
//-------------------------------------------------------------------------------------
void eth_cmd_inttime(char *par)
{
	char *data_var, *next;

	data_var = get_entry(par, &next);

	if(*data_var == NULL)
	{
		sprintf(eth_sendbuf, "inttime [485 | 232] [0-30]\n\r");
		Send_Ethernet_Packet(eth_sendbuf);		

		sprintf(eth_sendbuf, "RS485 send interval time: %d sec\n\r", Flashdatarec.e2p_485_int);
		Send_Ethernet_Packet(eth_sendbuf);		
		sprintf(eth_sendbuf, "RS232 send interval time: %d sec\n\r", Flashdatarec.e2p_232_int);
		Send_Ethernet_Packet(eth_sendbuf);	
	}
	else
	{
		if(strncmp((const char *)data_var, "485", 3) == 0)
		{
			data_var = next;
			data_var = get_entry(data_var, &next);
			if(data_var == NULL) 
				goto ETH_ERR_PARAM2;
			
			if(atoi(data_var) <= 30)
			{
				send_485int_timeout = atoi(data_var) * 10;
				Flashdatarec.e2p_485_int = atoi(data_var);
				FlashRom_WriteData();
				sprintf(eth_sendbuf, "RS485 send interval time: %d sec\n\r", Flashdatarec.e2p_485_int);
				Send_Ethernet_Packet(eth_sendbuf);		
				return;
			}
			else
				goto ETH_ERR_PARAM2;
		}
		else if(strncmp((const char *)data_var, "232", 3) == 0)
		{
			data_var = next;
			data_var = get_entry(data_var, &next);
			if(data_var == NULL) 
				goto ETH_ERR_PARAM2;
			
			if(atoi(data_var) <= 30)
			{
				send_232int_timeout = atoi(data_var) * 10;
				Flashdatarec.e2p_232_int = atoi(data_var);
				FlashRom_WriteData();
				sprintf(eth_sendbuf, "RS232 send interval time: %d sec\n\r", Flashdatarec.e2p_232_int);
				Send_Ethernet_Packet(eth_sendbuf);		
				return;
			}
			else
				goto ETH_ERR_PARAM2;
		}

		ETH_ERR_PARAM2:
			Send_Ethernet_Packet("Parameter Error!!\n\r");
	}
}
//-------------------------------------------------------------------------------------
void Modbus_Tcp_Proc(uint8_t sn)
{
	int32_t		ret;
	uint16_t	size;
	uint8_t 	*p_server_ip;
	
	switch(getSn_SR(sn))
	{
		case SOCK_ESTABLISHED:
			if(getSn_IR(sn) & Sn_IR_CON)
			{
				setSn_IR(sn,Sn_IR_CON);
			}

			if((size = getSn_RX_RSR(sn)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
			{
				if(size > ETH_MAX_BUF_SIZE) 
					size = ETH_MAX_BUF_SIZE;

				ret = recv(sn, ethBuf1, size);

				if(ret <= 0) 
					return;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.

				ethBuf1[ret] = NULL;
				Modbus_Tcp_Recv_Proc(ret);
//				if(Compare_Next_IP())		// ���� IP �� ���Ͽ� Ʋ���ٸ�
//				{
//					disconnect(sn);
//				}
				
				mod_retry_count = 0;
				flg_next_client = 1;
			}
			else
			{
				if(ethmod_timeout == 0)
				{
					if(mod_retry_count > 3)
					{
//						if(Compare_Next_IP())		// ���� IP �� ���Ͽ� Ʋ���ٸ�
//						{
//							disconnect(sn);
//							mprintf("Disconnect Retry 3 Over.. %d\n", sn);
//						}
						mod_retry_count = 0;
						flg_next_client = 1;
					}
					else
					{
						Tcp_Request_Packet(sn);
						mprintf("Request Packet.. %d\n", sn);
						ethmod_timeout = 10;
						mod_retry_count++;
					}
				}
			}
			break;
		case SOCK_CLOSE_WAIT :
			flg_mod_tcp_connected = 0;
			if((ret = disconnect(sn)) != SOCK_OK) 
				return;
			#ifdef _TCPSERVER_DEBUG_
				mprintf("%d:Socket Closed\n", sn);
			#endif
			break;
		case SOCK_INIT :
			if(ethmod_timeout == 0)
			{
				#ifdef _TCPSERVER_DEBUG_
					//mprintf("%d:Listen, TCP server, port [%d]\r\n", sn, TCP_SOCKET_PORT_NUM);
				#endif
				p_server_ip = Flashdatarec.e2p_mb_sip;
				
				ret = connect(sn, p_server_ip, Flashdatarec.e2p_mb_portnum);

				if(ret == SOCK_OK)
				{
					mprintf("%d:Socket connect OK..\n",sn);
					ethmod_timeout = 5;				// 3 sec
					mod_retry_count = 0;
				}
				else if(ret == SOCK_BUSY)
				{
					mprintf("%d:Socket Busy..\n",sn);
					ethmod_timeout = 10;				// 1 sec
					mod_retry_count++;
					if(mod_retry_count >= 3)
					{
						close(sn);
						mod_retry_count = 0;
						flg_next_client = 1;
					}
				}
			}
			break;
		case SOCK_CLOSED:
			flg_mod_tcp_connected = 0;
			mod_retry_count = 0;
			//ETHLED_OFF;
		
			if(socket(sn, Sn_MR_TCP, Flashdatarec.e2p_mb_portnum, SF_IO_NONBLOCK) == sn)
				mprintf("Socket %d Opend\n", sn);
			else
				mprintf("Socket Open Error %d\n", sn);
			break;
		default:
			break;
	}
}
//-----------------------------------------------------------------------
void Modbus_Tcp_Recv_Proc(uint32_t recv_len)
{
	char tmp_buffer[20]={0,};
	uint8_t i;
	uint16_t data_send_len, wifi_tmp_indx;
	
	data_send_len = recv_len + 9;
	wifi_tmp_indx = wifi_wr_indx + 1;
	wifi_tmp_indx %= WBUF_COUNT;
	
	if((wifi_rd_indx != wifi_tmp_indx) && (recv_len <= (WBUF_LEN - 18)))
	{
		wifi_send_buf[wifi_wr_indx][0] = 0x00;
		wifi_send_buf[wifi_wr_indx][1] = 0xBA;
		wifi_send_buf[wifi_wr_indx][2] = (Flashdatarec.e2p_id & 0xFF000000) >> 24;
		wifi_send_buf[wifi_wr_indx][3] = (Flashdatarec.e2p_id & 0x00FF0000) >> 16;
		wifi_send_buf[wifi_wr_indx][4] = (Flashdatarec.e2p_id & 0x0000FF00) >> 8;
		wifi_send_buf[wifi_wr_indx][5] = (Flashdatarec.e2p_id & 0x000000FF);
		wifi_send_buf[wifi_wr_indx][6] = 0x50;								// Command for RTU
		wifi_send_buf[wifi_wr_indx][7] = (data_send_len & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][8] = (data_send_len & 0x00FF);
		wifi_send_buf[wifi_wr_indx][9] = Flashdatarec.e2p_mb_sip[0];
		wifi_send_buf[wifi_wr_indx][10] = Flashdatarec.e2p_mb_sip[1];
		wifi_send_buf[wifi_wr_indx][11] = Flashdatarec.e2p_mb_sip[2];
		wifi_send_buf[wifi_wr_indx][12] = Flashdatarec.e2p_mb_sip[3];
		wifi_send_buf[wifi_wr_indx][13] = Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_fcode;
		wifi_send_buf[wifi_wr_indx][14] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_address & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][15] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_address & 0x00FF);
		wifi_send_buf[wifi_wr_indx][16] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_length & 0xFF00) >> 8;
		wifi_send_buf[wifi_wr_indx][17] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_length & 0x00FF);

		memcpy(&wifi_send_buf[wifi_wr_indx][18], ethBuf1, recv_len);			// Device id
		
		wifi_send_len[wifi_wr_indx] = recv_len + 18;
		
		if((flg_mtcp_enable) && (flg_ethernet_connected))
		{
			sprintf(tmp_buffer,"Recv %d Byte: ", recv_len);
			Send_Ethernet_Packet(tmp_buffer);
			
			for(i=0; i<wifi_send_len[wifi_wr_indx]; i++)
			{
				sprintf(tmp_buffer,"%02X ", wifi_send_buf[wifi_wr_indx][i]);
				Send_Ethernet_Packet(tmp_buffer);
			}
			Send_Ethernet_Packet("\r\n");
		}
		
		wifi_wr_indx++;
		wifi_wr_indx %= WBUF_COUNT;
		
		Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_transaction++;
		
		mprintf("Receve Len = %d, wifi_wr_indx = %d\n", recv_len, wifi_wr_indx);
	}
}
//-----------------------------------------------------------------------
void Tcp_Request_Packet(uint8_t sn)
{
	char tmp_buffer[20]={0,};
	uint8_t i;
	
	// MBAP Header
	modbus_send_buf[0] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_transaction & 0xFF00) >> 8;
	modbus_send_buf[1] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_transaction & 0x00FF);			// Transaction Identifier
	modbus_send_buf[2] = 0x0;
	modbus_send_buf[3] = 0x0;								// Modbus protocol
	modbus_send_buf[4] = 0x0;					
	modbus_send_buf[5] = 6;									// Length
	modbus_send_buf[6] = 0x01;								// Unit Identifier
	
	modbus_send_buf[7] = Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_fcode;							// Function Code
	modbus_send_buf[8] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_address & 0xFF00) >> 8;					
	modbus_send_buf[9] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_address & 0x00FF);				// Address
	modbus_send_buf[10] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_length & 0xFF00) >> 8;					
	modbus_send_buf[11] = (Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_length & 0x00FF);				// Length
	
	send(sn, modbus_send_buf, 12);
	
	if((flg_mtcp_enable) && (flg_ethernet_connected))
	{
		Send_Ethernet_Packet("Send: ");
		
		for(i=0; i<12; i++)
		{
			sprintf(tmp_buffer,"%02X ", modbus_send_buf[i]);
			Send_Ethernet_Packet(tmp_buffer);
		}
		Send_Ethernet_Packet("\r\n");
	}
}
//-----------------------------------------------------------------------
uint8_t Select_Client_index(void)
{
	uint8_t i, ret_var = 0, tmp_indx;
	
	
	tmp_indx = mod_tcp_indx;

	tmp_indx++;
	tmp_indx %= CLIENT_COUNT;

	for(i=0; i<CLIENT_COUNT; i++)
	{
		
		if(Flashdatarec.e2p_mod_tcp[tmp_indx].mb_enable)
		{
			mod_tcp_indx = tmp_indx;
			ret_var = 1;
			break;
		}
		
		tmp_indx++;
		tmp_indx %= CLIENT_COUNT;
	}
	
	return ret_var;
}

//-----------------------------------------------------------------------
//uint8_t Compare_Next_IP(void)
//{
//	uint8_t i, ret_var = 0, tmp_indx;
//	
//	tmp_indx = mod_tcp_indx;

//	tmp_indx++;
//	tmp_indx %= CLIENT_COUNT;

//	for(i=0; i<CLIENT_COUNT; i++)
//	{
//		
//		if(Flashdatarec.e2p_mod_tcp[tmp_indx].mb_enable)
//			break;
//		
//		tmp_indx++;
//		tmp_indx %= CLIENT_COUNT;
//	}
//	
//	if(strncmp((const char *)(Flashdatarec.e2p_mod_tcp[mod_tcp_indx].mb_server_ip), 
//		(const char *)(Flashdatarec.e2p_mod_tcp[tmp_indx].mb_server_ip), 4) == 0)
//			ret_var = 0;
//	else
//		ret_var = 1;
//	
//	return ret_var;
//}
//----------------------------------------------------------------------------------------
void View_Packet_Data(uint8_t rtutcp_flag, uint8_t view_num)
{
	if(rtutcp_flag == 1)				// RTU
	{
		sprintf(eth_sendbuf, "packet num:\t%d\n\r", view_num + 1);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "enable:\t\t%d\n\r", Flashdatarec.e2p_mod_rtu[view_num].mb_enable); 
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "id:\t\t%d\n\r", Flashdatarec.e2p_mod_rtu[view_num].mb_id);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "fcode:\t\t%d\n\r", Flashdatarec.e2p_mod_rtu[view_num].mb_fcode);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "address:\t%d\n\r", Flashdatarec.e2p_mod_rtu[view_num].mb_address);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "length:\t\t%d\n\r", Flashdatarec.e2p_mod_rtu[view_num].mb_length);
		Send_Ethernet_Packet(eth_sendbuf); 
		Send_Ethernet_Packet("-----------------------------------------------------------------\n\r"); 
	}
	else
	{
		sprintf(eth_sendbuf, "packet num:\t%d\n\r", view_num + 1);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "enable:\t\t%d\n\r", Flashdatarec.e2p_mod_tcp[view_num].mb_enable); 
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "tr num:\t\t%d\n\r", Flashdatarec.e2p_mod_tcp[view_num].mb_transaction);
		Send_Ethernet_Packet(eth_sendbuf); 
		
		sprintf(eth_sendbuf, "fcode:\t\t%d\n\r", Flashdatarec.e2p_mod_tcp[view_num].mb_fcode);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "address:\t%d\n\r", Flashdatarec.e2p_mod_tcp[view_num].mb_address);
		Send_Ethernet_Packet(eth_sendbuf); 
		sprintf(eth_sendbuf, "length:\t\t%d\n\r", Flashdatarec.e2p_mod_tcp[view_num].mb_length);
		Send_Ethernet_Packet(eth_sendbuf); 
		Send_Ethernet_Packet("-----------------------------------------------------------------\n\r"); 
	}
}
