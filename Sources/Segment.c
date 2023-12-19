#include "main.h"
#include "segment.h"



const uint8_t digit[] =
{
    FND_0,   FND_1,  FND_2, FND_3, FND_4, FND_5, FND_6, FND_7,         // 0~7
    FND_8,   FND_9,  FND_A, FND_b, FND_C, FND_d, FND_E, FND_F,         // 8~15
    FND_BAR, FND_n,  BLANK, FND_L, FND_o, FND_H, FND_i, FND_h,		   // 16-23
	FND_t,   FND_r,  FND_P, FND_N, FND_c							   // 24	
};

uint8_t seg_idx = 0;
uint8_t segdata[4] = {0xFF, 0xFF, 0xFF, 0xFF};
//======================================================
void Seg_data_set(uint8_t segnum, uint8_t seg_data, uint8_t dot)
{
    segdata[segnum] = digit[seg_data];
    if(dot)
        segdata[segnum] &= FND_DOT;
}

void Led_data_set(uint8_t segnum, uint8_t led_data, uint8_t onoff)
{
    if(segnum > 3) return;
    
    if(onoff)
        segdata[segnum] &= ~led_data;
    else
        segdata[segnum] |= led_data;
}

void Segment_Disp(void)
{
	SEG_SEL1_OFF;
	SEG_SEL2_OFF;
	SEG_SEL3_OFF;
	SEG_SEL4_OFF;

	(segdata[seg_idx] & 0x01) ? HAL_GPIO_WritePin(SEG_A_GPIO_Port, SEG_A_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_A_GPIO_Port, SEG_A_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x02) ? HAL_GPIO_WritePin(SEG_B_GPIO_Port, SEG_B_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_B_GPIO_Port, SEG_B_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x04) ? HAL_GPIO_WritePin(SEG_C_GPIO_Port, SEG_C_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_C_GPIO_Port, SEG_C_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x08) ? HAL_GPIO_WritePin(SEG_D_GPIO_Port, SEG_D_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_D_GPIO_Port, SEG_D_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x10) ? HAL_GPIO_WritePin(SEG_E_GPIO_Port, SEG_E_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_E_GPIO_Port, SEG_E_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x20) ? HAL_GPIO_WritePin(SEG_F_GPIO_Port, SEG_F_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_F_GPIO_Port, SEG_F_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x40) ? HAL_GPIO_WritePin(SEG_G_GPIO_Port, SEG_G_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_G_GPIO_Port, SEG_G_Pin, GPIO_PIN_RESET);
	(segdata[seg_idx] & 0x80) ? HAL_GPIO_WritePin(SEG_DP_GPIO_Port, SEG_DP_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(SEG_DP_GPIO_Port, SEG_DP_Pin, GPIO_PIN_RESET);

	
    switch(seg_idx)
    {
        case 0:
            SEG_SEL1_ON;
            break;
        case 1:
            SEG_SEL2_ON;
            break;
        case 2:
            SEG_SEL3_ON;
            break;
        case 3:
            SEG_SEL4_ON;
            break;
        default:
            break;
    }

    seg_idx++;
    if(seg_idx > 3)
        seg_idx = 0;
}
