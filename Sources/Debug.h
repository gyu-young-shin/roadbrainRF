#ifndef	__DEBUG_H
#define	__DEBUG_H

#define	URX2_LEN		256
#define	UTX2_LEN		512


#define BACKSPACE			0x08			// Backspace
#define CRETURN				0x0D			// Carriage Return
#define LF					0x0A			// Line Feed
#define ESC					0x1B			// Escape
#define DEL					0x7F			// Del

typedef struct scmd
{
	char val[10];
	void (*func)(char* par);
}SCMD;

void USART2_IRQ_Function(void);
void Debug_proc(void);

char* get_entry(char *cp, char **pnext);
void GetLine_Proc(void);									// Line 입력을 받는다.
void UserCommand_Proc(void);						// 사용자 명령어 처리
void SendChar(unsigned char send_c);
void mprintf(const char *format, ...);					// For RS232 Debug
char* hex2Str(unsigned char * data, size_t dataLen);

void cmd_help(char *par);
void cmd_wifidebug(char *par);
void cmd_serialnumber(char *par);
void cmd_serverip(char *par);
void cmd_serverport(char *par);
void Update_SerialNSSid(void);

#endif
