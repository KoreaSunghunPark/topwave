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
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_13
#define LED1_GPIO_Port GPIOC
#define EN2_Pin GPIO_PIN_5
#define EN2_GPIO_Port GPIOA
#define DIR2_Pin GPIO_PIN_6
#define DIR2_GPIO_Port GPIOA
#define PUL2_Pin GPIO_PIN_7
#define PUL2_GPIO_Port GPIOA
#define DIR1_Pin GPIO_PIN_10
#define DIR1_GPIO_Port GPIOB
#define CUT_12VOUT_Pin GPIO_PIN_7
#define CUT_12VOUT_GPIO_Port GPIOC
#define EN1_Pin GPIO_PIN_8
#define EN1_GPIO_Port GPIOA
#define USBDM_Pin GPIO_PIN_11
#define USBDM_GPIO_Port GPIOA
#define USBDP_Pin GPIO_PIN_12
#define USBDP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define LED3_Pin GPIO_PIN_11
#define LED3_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_12
#define LED2_GPIO_Port GPIOC
#define EI3_BLIMIT_Pin GPIO_PIN_3
#define EI3_BLIMIT_GPIO_Port GPIOB
#define EI3_BLIMIT_EXTI_IRQn EXTI3_IRQn
#define PUL1_Pin GPIO_PIN_4
#define PUL1_GPIO_Port GPIOB
#define EI5_TLIMIT_Pin GPIO_PIN_5
#define EI5_TLIMIT_GPIO_Port GPIOB
#define EI5_TLIMIT_EXTI_IRQn EXTI9_5_IRQn
#define DOOR_Pin GPIO_PIN_6
#define DOOR_GPIO_Port GPIOB
#define SOL_LOCK1_Pin GPIO_PIN_8
#define SOL_LOCK1_GPIO_Port GPIOB
#define SOL_LOCK2_Pin GPIO_PIN_9
#define SOL_LOCK2_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/