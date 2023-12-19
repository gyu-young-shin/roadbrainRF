#ifndef __SEGMENT_H
#define __SEGMENT_H
#include "main.h"

#define FND_0   0xC0
#define FND_1   0xF9
#define FND_2   0xA4
#define FND_3   0xB0
#define FND_4   0x99
#define FND_5   0x92
#define FND_6   0x82
#define FND_7   0xD8
#define FND_8   0x80
#define FND_9   0x90
#define FND_A   0x88
#define FND_b   0x83
#define FND_C   0xC6
#define FND_c   0xA7
#define FND_u   0xE3
#define FND_d   0xA1
#define FND_E   0x86
#define FND_F   0x8E
#define FND_P   0x8C			// 10001100
#define FND_n   0xAB
#define FND_N   0xC8
#define FND_o   0xA3
#define FND_r   0xAF
#define FND_L   0xC7
#define FND_i   0xFB
#define FND_H   0x89
#define FND_h   0x8B
#define FND_t   0x87
#define FND_S   0x92
#define FND_BAR 0xBF
#define FND_UBAR 0xF7
#define BLANK   0xFF
#define FND_DOT 0x7F

#define LEDL_NOFIRED 0x01
#define LEDL_TILT    0x02
#define LEDL_LOCK    0x04
#define LEDL_THEMO   0x08
#define LEDL_LFULE   0x10
#define LEDL_MFULE   0x20
#define LEDL_SFULE   0x40
#define LEDL_EFULE   0x80

#define LEDT_OPER    0x01
#define LEDT_UP      0x02
#define LEDT_TCHLOCK 0x04
#define LEDT_TMPTIME 0x08
#define LEDT_DOWN    0x10
#define LEDT_OFFRSV  0x20
#define LEDT_BLOWFAN 0x40

#define LEDR_FIRED   0x01
#define LEDR_RESERV  0x02
#define LEDR_TEMP    0x04
#define LEDR_TIME    0x08

#define LEDF_LEVEL1  0x10
#define LEDF_LEVEL2  0x20
#define LEDF_LEVEL3  0x40
#define LEDF_AMTFULE 0x80




void Led_data_set(uint8_t segnum, uint8_t led_data, uint8_t onoff);
void Seg_data_set(uint8_t segnum, uint8_t seg_data, uint8_t dot);
void Segment_Disp(void);

#endif

