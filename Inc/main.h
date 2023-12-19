/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OPER_LED0_Pin GPIO_PIN_2
#define OPER_LED0_GPIO_Port GPIOE
#define W5100_RST_Pin GPIO_PIN_4
#define W5100_RST_GPIO_Port GPIOE
#define W5100_INT_Pin GPIO_PIN_5
#define W5100_INT_GPIO_Port GPIOE
#define SW_MENU_Pin GPIO_PIN_0
#define SW_MENU_GPIO_Port GPIOC
#define SW_UP_Pin GPIO_PIN_1
#define SW_UP_GPIO_Port GPIOC
#define SW_DOWN_Pin GPIO_PIN_2
#define SW_DOWN_GPIO_Port GPIOC
#define SW_ENT_Pin GPIO_PIN_3
#define SW_ENT_GPIO_Port GPIOC
#define EXIT_DI1_Pin GPIO_PIN_4
#define EXIT_DI1_GPIO_Port GPIOA
#define EXIT_DI1_EXTI_IRQn EXTI4_IRQn
#define DIO_IN2_Pin GPIO_PIN_5
#define DIO_IN2_GPIO_Port GPIOA
#define DIO_OUT_Pin GPIO_PIN_6
#define DIO_OUT_GPIO_Port GPIOA
#define DIO_OUT2_Pin GPIO_PIN_7
#define DIO_OUT2_GPIO_Port GPIOA
#define DIO_OUT1_Pin GPIO_PIN_4
#define DIO_OUT1_GPIO_Port GPIOC
#define OPER_LED_Pin GPIO_PIN_0
#define OPER_LED_GPIO_Port GPIOB
#define ETH_LED_Pin GPIO_PIN_1
#define ETH_LED_GPIO_Port GPIOB
#define RFTX_LED_Pin GPIO_PIN_12
#define RFTX_LED_GPIO_Port GPIOE
#define RFRX_LED_Pin GPIO_PIN_13
#define RFRX_LED_GPIO_Port GPIOE
#define RFERR_LED_Pin GPIO_PIN_14
#define RFERR_LED_GPIO_Port GPIOE
#define WIFI_EN_Pin GPIO_PIN_15
#define WIFI_EN_GPIO_Port GPIOE
#define RF_NSS_Pin GPIO_PIN_12
#define RF_NSS_GPIO_Port GPIOB
#define RF_TXRAMP_Pin GPIO_PIN_8
#define RF_TXRAMP_GPIO_Port GPIOD
#define LORA_RESET_Pin GPIO_PIN_9
#define LORA_RESET_GPIO_Port GPIOD
#define RF_NIRQ_Pin GPIO_PIN_10
#define RF_NIRQ_GPIO_Port GPIOD
#define LORA_DIO3_Pin GPIO_PIN_11
#define LORA_DIO3_GPIO_Port GPIOD
#define LORA_DIO2_Pin GPIO_PIN_12
#define LORA_DIO2_GPIO_Port GPIOD
#define LORA_DIO1_Pin GPIO_PIN_13
#define LORA_DIO1_GPIO_Port GPIOD
#define LORA_DIO0_Pin GPIO_PIN_6
#define LORA_DIO0_GPIO_Port GPIOC
#define FND_COM1_Pin GPIO_PIN_7
#define FND_COM1_GPIO_Port GPIOC
#define FND_COM2_Pin GPIO_PIN_8
#define FND_COM2_GPIO_Port GPIOC
#define FND_COM3_Pin GPIO_PIN_9
#define FND_COM3_GPIO_Port GPIOC
#define FND_COM4_Pin GPIO_PIN_8
#define FND_COM4_GPIO_Port GPIOA
#define RS485_DE1_Pin GPIO_PIN_11
#define RS485_DE1_GPIO_Port GPIOA
#define ETHERNET_LINK_Pin GPIO_PIN_12
#define ETHERNET_LINK_GPIO_Port GPIOA
#define SEG_A_Pin GPIO_PIN_15
#define SEG_A_GPIO_Port GPIOA
#define SEG_D_Pin GPIO_PIN_12
#define SEG_D_GPIO_Port GPIOC
#define SEG_E_Pin GPIO_PIN_2
#define SEG_E_GPIO_Port GPIOD
#define SEG_B_Pin GPIO_PIN_3
#define SEG_B_GPIO_Port GPIOD
#define SEG_C_Pin GPIO_PIN_6
#define SEG_C_GPIO_Port GPIOD
#define SEG_F_Pin GPIO_PIN_3
#define SEG_F_GPIO_Port GPIOB
#define SEG_G_Pin GPIO_PIN_4
#define SEG_G_GPIO_Port GPIOB
#define SEG_DP_Pin GPIO_PIN_5
#define SEG_DP_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define	SENSOR_COUNT	7
#define KEY_COUNT  		4
#define	MAX_SLAVE_COUNT	32
#define	VERSION_HIGH	3
#define	VERSION_LOW		3


#define	OPER_LED0_OFF	HAL_GPIO_WritePin(OPER_LED0_GPIO_Port, OPER_LED0_Pin, GPIO_PIN_SET)
#define	OPER_LED0_ON	HAL_GPIO_WritePin(OPER_LED0_GPIO_Port, OPER_LED0_Pin, GPIO_PIN_RESET)

#define	RS485_DE1_HIGH	HAL_GPIO_WritePin(RS485_DE1_GPIO_Port, RS485_DE1_Pin, GPIO_PIN_SET)
#define	RS485_DE1_LOW	HAL_GPIO_WritePin(RS485_DE1_GPIO_Port, RS485_DE1_Pin, GPIO_PIN_RESET)

#define RF_GPIO0		HAL_GPIO_ReadPin(LORA_DIO0_GPIO_Port, LORA_DIO0_Pin)
#define RF_GPIO1		HAL_GPIO_ReadPin(LORA_DIO1_GPIO_Port, LORA_DIO1_Pin)
#define RF_GPIO2		HAL_GPIO_ReadPin(LORA_DIO2_GPIO_Port, LORA_DIO2_Pin)
#define RF_GPIO3		HAL_GPIO_ReadPin(LORA_DIO3_GPIO_Port, LORA_DIO3_Pin)

#define MENU_BUTTON		HAL_GPIO_ReadPin(SW_MENU_GPIO_Port, SW_MENU_Pin)
#define UP_BUTTON		HAL_GPIO_ReadPin(SW_UP_GPIO_Port, SW_UP_Pin)
#define DOWN_BUTTON		HAL_GPIO_ReadPin(SW_DOWN_GPIO_Port, SW_DOWN_Pin)
#define ENT_BUTTON		HAL_GPIO_ReadPin(SW_ENT_GPIO_Port, SW_ENT_Pin)

#define TX_RAMP 		HAL_GPIO_ReadPin(RF_TXRAMP_GPIO_Port, RF_TXRAMP_Pin)
#define SPI_NIRQ		HAL_GPIO_ReadPin(RF_NIRQ_GPIO_Port, RF_NIRQ_Pin)

#define ETHERNET_LINK	HAL_GPIO_ReadPin(ETHERNET_LINK_GPIO_Port, ETHERNET_LINK_Pin)

#define DIO_IN1			HAL_GPIO_ReadPin(EXIT_DI1_GPIO_Port, EXIT_DI1_Pin)
#define DIO_IN2			HAL_GPIO_ReadPin(DIO_IN2_GPIO_Port, DIO_IN2_Pin)

#define PWRDN_ON		HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_SET)
#define PWRDN_OFF		HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_RESET)

#define RF_NSEL_RESET	HAL_GPIO_WritePin(RF_NSS_GPIO_Port, RF_NSS_Pin, GPIO_PIN_RESET)
#define RF_NSEL_SET		HAL_GPIO_WritePin(RF_NSS_GPIO_Port, RF_NSS_Pin, GPIO_PIN_SET)


#define ERRORLED_ON		HAL_GPIO_WritePin(RFTX_LED_GPIO_Port, RFTX_LED_Pin, GPIO_PIN_RESET)
#define ERRORLED_OFF	HAL_GPIO_WritePin(RFTX_LED_GPIO_Port, RFTX_LED_Pin, GPIO_PIN_SET)
#define ETHLED_ON		HAL_GPIO_WritePin(RFRX_LED_GPIO_Port, RFRX_LED_Pin, GPIO_PIN_RESET)
#define ETHLED_OFF		HAL_GPIO_WritePin(RFRX_LED_GPIO_Port, RFRX_LED_Pin, GPIO_PIN_SET)
#define WIFILED_ON		HAL_GPIO_WritePin(RFERR_LED_GPIO_Port, RFERR_LED_Pin, GPIO_PIN_RESET)
#define WIFILED_OFF		HAL_GPIO_WritePin(RFERR_LED_GPIO_Port, RFERR_LED_Pin, GPIO_PIN_SET)
#define CONNECTLED_ON	HAL_GPIO_WritePin(ETH_LED_GPIO_Port, ETH_LED_Pin, GPIO_PIN_RESET)
#define CONNECTLED_OFF	HAL_GPIO_WritePin(ETH_LED_GPIO_Port, ETH_LED_Pin, GPIO_PIN_SET)

#define SEG_SEL1_ON		HAL_GPIO_WritePin(FND_COM1_GPIO_Port, FND_COM1_Pin, GPIO_PIN_RESET)
#define SEG_SEL1_OFF	HAL_GPIO_WritePin(FND_COM1_GPIO_Port, FND_COM1_Pin, GPIO_PIN_SET)
#define SEG_SEL2_ON		HAL_GPIO_WritePin(FND_COM2_GPIO_Port, FND_COM2_Pin, GPIO_PIN_RESET)
#define SEG_SEL2_OFF	HAL_GPIO_WritePin(FND_COM2_GPIO_Port, FND_COM2_Pin, GPIO_PIN_SET)
#define SEG_SEL3_ON		HAL_GPIO_WritePin(FND_COM3_GPIO_Port, FND_COM3_Pin, GPIO_PIN_RESET)
#define SEG_SEL3_OFF	HAL_GPIO_WritePin(FND_COM3_GPIO_Port, FND_COM3_Pin, GPIO_PIN_SET)
#define SEG_SEL4_ON		HAL_GPIO_WritePin(FND_COM4_GPIO_Port, FND_COM4_Pin, GPIO_PIN_RESET)
#define SEG_SEL4_OFF	HAL_GPIO_WritePin(FND_COM4_GPIO_Port, FND_COM4_Pin, GPIO_PIN_SET)

#define DO_ON			HAL_GPIO_WritePin(DIO_OUT_GPIO_Port, DIO_OUT_Pin, GPIO_PIN_RESET)
#define DO_OFF			HAL_GPIO_WritePin(DIO_OUT_GPIO_Port, DIO_OUT_Pin, GPIO_PIN_SET)
#define DO1_ON			HAL_GPIO_WritePin(DIO_OUT1_GPIO_Port, DIO_OUT1_Pin, GPIO_PIN_RESET)
#define DO1_OFF			HAL_GPIO_WritePin(DIO_OUT1_GPIO_Port, DIO_OUT1_Pin, GPIO_PIN_SET)
#define DO2_ON			HAL_GPIO_WritePin(DIO_OUT2_GPIO_Port, DIO_OUT2_Pin, GPIO_PIN_RESET)
#define DO2_OFF			HAL_GPIO_WritePin(DIO_OUT2_GPIO_Port, DIO_OUT2_Pin, GPIO_PIN_SET)

#define WIZ_RESET_HIGH	HAL_GPIO_WritePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin, GPIO_PIN_SET)
#define WIZ_RESET_LOW	HAL_GPIO_WritePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin, GPIO_PIN_RESET)

enum {MENU_KEY = 0, UP_KEY, DOWN_KEY, ENTER_KEY};
enum {TCO = 0, TCI, TST, TSO, TL, TW, TR};	

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
