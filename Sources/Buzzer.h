#ifndef __BUZZER_H
#define __BUZZER_H
#include "main.h"

#define	BASETIME	10      //4msec        //10

#define O7_DO       2093
#define O7_RE       2349
#define O7_MI       2637
#define O7_FA       2793
#define O7_SO       3135
#define O7_RA       3520
#define O7_CI       3951

#define O8_DO       4186
#define O8_RE       4698
#define O8_MI       5274
#define O8_FA       5587
#define O8_SO       6271
#define O8_RA       7040
#define O8_CI       7902

#define	TEMPO1		0x22
#define	TEMPO2		0x29

#define	Tempo_0		0x00
#define Tempo_h8    0x01        //1
#define Tempo_h4    0x02        //2
#define Tempo_h2    0x03        //4
#define Tempo_1     0x04        //8
#define Tempo_1h2   0x05        //12
#define Tempo_2     0x06        //16
#define	Tempo_2h2   0x07        //20
#define Tempo_3     0x08        //24
#define	Tempo_3h2   0x09        //28
#define Tempo_4     0x0a        //32
#define Tempo_4h2   0x0b        //36
#define	Tempo_5     0x0c        //40
#define Tempo_5h2   0x0e        //44
#define Tempo_6     0x0f        //48

//****     쉬는 박자    ***********//
#define	CTempo_h8	0x10        //1
#define CTempo_h4   0x20        //2
#define CTempo_h2	0x30        //4
#define CTempo_1    0x40        //8
#define CTempo_1h2  0x50        //12
#define CTempo_2    0x60        //16
#define CTempo_2h2  0x70        //20
#define CTempo_3    0x80        //24
#define CTempo_3h2  0x90        //28
#define CTempo_4    0xa0        //32
#define CTempo_4h2  0xb0        //36
#define CTempo_5    0xc0        //40
#define CTempo_5h2	0xe0        //44
#define CTempo_6    0xf0        //48

void Buzzer_Sequence(void);
void Play_Buzzer(uint8_t play_num);
void Buzzer_Stop(void);
void PWM_Config(uint16_t cur_period);

#endif
