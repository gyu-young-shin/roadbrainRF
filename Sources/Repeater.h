#ifndef	__REPEATER_H
#define	__REPEATER_H
#include "main.h"

#define KEYBUF_SIZE	10


enum {DISP_MENU = 0, DISP_EDIT};

void Repeater_Proc(void);
void InputKey_Proc(void);
void Disp_Segment(void);
void Output_Proc(void);
void Disp_Version(void);
void Convert_Int_Temp(void);

	
#endif

