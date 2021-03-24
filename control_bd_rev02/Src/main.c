/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "usbd_cdc_if.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define EOT		0x0d   // '\r'  0x0d
#define DEBUGPORT	huart2		// default huart1
#define COMMUART	USART2		// default USART2
#define COMMPORT	huart2    // default huart2
#define COMMDATA	rx2_data	// default rx2_data
#define	ALIVE_PRIOD	180
#define REPORT_DOOR 1
#define	REPORT_CMD	0

//#define SOL_LOCK	//2020.07.28 psh

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t COMMDATA;
uint8_t buf_index = 0;
uint8_t Rxcplt_flag = 0;
uint8_t CDC_flag = 0;
uint8_t PAD_boot_flag = 0;
uint8_t door_status = 1;
char CDCbuffer[100] = "";
char Rxbuffer[100] = "";
char Txbuffer[12] = "";	//to add checksum psh 2020.07.28 Again...!!
//char Txbuffer[11] = "";
int time_second = 0;
//int	alive_call_fail = 0;
volatile int alive_call_fail = 0;
int close_check = 0;
int open_check = 0;
int door_check = 0;
int door_check_enable = 0;

struct Board_state {
	char cmd[5];
	char door[5];
	char lock[5];
	char scan[5];
	char end[5];
};
struct Board_state Bstate = { "BS,", "1,", "1,", "0,", "\r" };   // EOT == \r

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&DEBUGPORT, (uint8_t*) ptr, (uint16_t) len, 0xFFFFFFFF);
	return len;
}

void Report_to_Server(int report_path);
void Rfid_Scanning(int speed);		// unit: seconds
void check_tablet_alive(int max_trying_count);
void check_door(void);
void command_parsing(char *str_ptr);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM7_Init();
  MX_USB_DEVICE_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&COMMPORT, &COMMDATA, 1);
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_TIM_Base_Start_IT(&htim7);

	//2020.07.28 psh
	#ifndef SOL_LOCK
	// 2020.07.28 psh - EM lock
		HAL_GPIO_WritePin(SOL_LOCK1_GPIO_Port, SOL_LOCK1_Pin, GPIO_PIN_SET); // unlock SOL_LOCK1 --> LOCK of EM Lock
		HAL_GPIO_WritePin(SOL_LOCK2_GPIO_Port, SOL_LOCK2_Pin, GPIO_PIN_SET); // unlock SOL_LOCK2 --> LOCK of EM Lock
	#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	char *str_ptr = NULL;

	printf(" hello? \r\n");     // ǥ�� ���? printf�� ����Ͽ�?...

	while (1) {

		if (CDC_flag == 1) {
			printf(" Received CDC data: %s\r\n", CDCbuffer);
			if (CDCbuffer[0] != '\0') {
				str_ptr = strtok(CDCbuffer, ",");
				printf(" command: %s\r\n", str_ptr);
				command_parsing(str_ptr);
			}
			CDC_flag = 0;
		}

		if (Rxcplt_flag == 1) {
			printf(" Received data: %s\r\n", Rxbuffer);

			if (Rxbuffer[0] != '\0') {
				str_ptr = strtok(Rxbuffer, ",");
				printf(" command: %s\r\n", str_ptr);
				command_parsing(str_ptr);
			}   // end if(Rxbuffer[0] != '\0')
			buf_index = 0;
			Rxcplt_flag = 0;
		}   // end if(Rxcplt_flag == 1)

		if (time_second >= ALIVE_PRIOD)
			check_tablet_alive(3);

		if (door_check_enable == 1)
			check_door();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* USART1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* TIM6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM6_IRQn);
  /* USART2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/* USER CODE BEGIN 4 */

void command_parsing(char *str_ptr) {

	if (!strcmp(str_ptr, "OK")) {
		printf(" OK signal received\r\n");
		str_ptr = strtok(NULL, ",");
		PAD_boot_flag = 1;
		alive_call_fail = 0;

	} else if (!strcmp(str_ptr, "BR")) {
		printf(" Board Read\r\n");
		str_ptr = strtok(NULL, ",");
		Report_to_Server(REPORT_CMD);

	} else if (!strcmp(str_ptr, "UD")) {
		printf(" Unlock Door\r\n");
		str_ptr = strtok(NULL, ",");

#ifdef SOL_LOCK
	  HAL_GPIO_WritePin(SOL_LOCK1_GPIO_Port, SOL_LOCK1_Pin, GPIO_PIN_SET);  // unlock SOL_LOCK1
	  HAL_GPIO_WritePin(SOL_LOCK2_GPIO_Port, SOL_LOCK2_Pin, GPIO_PIN_SET);  // unlock SOL_LOCK2
#else
		// 2020.07.28 psh - EM lock
		HAL_GPIO_WritePin(SOL_LOCK1_GPIO_Port, SOL_LOCK1_Pin, GPIO_PIN_RESET); // lock SOL_LOCK1 ---> Unlock of EM Lock
		HAL_GPIO_WritePin(SOL_LOCK2_GPIO_Port, SOL_LOCK2_Pin, GPIO_PIN_RESET); // lock SOL_LOCK2 ---> Unlock of EM Lock
#endif
		HAL_Delay(1000);	// 1s delay for unlock operation
		strcpy(Bstate.lock, "0,");  // ���� ����
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

		Report_to_Server(REPORT_CMD);

	} else if (!strcmp(str_ptr, "LD")) {
		printf(" Lock Door\r\n");
		str_ptr = strtok(NULL, ",");

#ifdef SOL_LOCK
	  HAL_GPIO_WritePin(SOL_LOCK1_GPIO_Port, SOL_LOCK1_Pin, GPIO_PIN_RESET);  // lock SOL_LOCK1
	  HAL_GPIO_WritePin(SOL_LOCK2_GPIO_Port, SOL_LOCK2_Pin, GPIO_PIN_RESET);  // lock SOL_LOCK2
#else
// 2020.07.28 psh - EM lock
		HAL_GPIO_WritePin(SOL_LOCK1_GPIO_Port, SOL_LOCK1_Pin, GPIO_PIN_SET); // unlock SOL_LOCK1  ---> Lock of EM Lock
		HAL_GPIO_WritePin(SOL_LOCK2_GPIO_Port, SOL_LOCK2_Pin, GPIO_PIN_SET); // unlock SOL_LOCK2  ---> Lock of EM Lock
#endif

		HAL_Delay(1000);	// 1s delay for lock operation
		strcpy(Bstate.lock, "1,");  // ���� ����
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

		Report_to_Server(REPORT_CMD);

	} else if (!strcmp(str_ptr, "RS")) {
		printf(" RFID Scanning\r\n");
		str_ptr = strtok(NULL, ",");

		if (str_ptr == '\0') {
			printf(" need more parameter!\r\n");
		} else {
			printf(" speed:%s\r\n", str_ptr);
			// HAL_Delay(1000);
			Rfid_Scanning(atoi(str_ptr));

			Report_to_Server(REPORT_CMD);
		}
	}
}

void check_door(void) {
	door_check_enable = 0;

	if (HAL_GPIO_ReadPin(DOOR_GPIO_Port, DOOR_Pin) == GPIO_PIN_SET)
		open_check++;   // open
	else
		close_check++;  // close

	if (door_check > 3) {  // 5 times when door_check is 0,1,2,3,4
		if (open_check > 3) { // open
			HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
			if (!strcmp(Bstate.door, "1,")) {    // "1," == closed
				strcpy(Bstate.door, "0,");
				Report_to_Server(REPORT_DOOR);
				printf(" Door is opened!\r\n");
			}
		}
		if (close_check > 3) {  //close
			HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
			if (!strcmp(Bstate.door, "0,")) {	// "0," == opened
				strcpy(Bstate.door, "1,");
				Report_to_Server(REPORT_DOOR);
				printf(" Door is closed!\r\n");
			}
		}
		door_check = 0;
		open_check = 0;
		close_check = 0;

	} else
		door_check++;

}

void check_tablet_alive(int max_trying_count) {
	CDC_Transmit_FS((uint8_t*) "callme\r", (uint16_t) strlen("callme\r"));
	HAL_UART_Transmit(&COMMPORT, (uint8_t*) "callme\r",
			(uint16_t) strlen("callme\r"), 0xFFFFFFFF);
	printf(" alive calling!\r\n");
	if (PAD_boot_flag == 1) {
		alive_call_fail++;
	}
	time_second = 0;

	if (alive_call_fail >= max_trying_count) { // check alive for 'max_trying_count' times
		HAL_GPIO_WritePin(CUT_12VOUT_GPIO_Port, CUT_12VOUT_Pin, GPIO_PIN_SET);
		printf(" Tablet died, rebooted!\r\n"); // control relay for powering tablet
		HAL_Delay(2000);
		HAL_GPIO_WritePin(CUT_12VOUT_GPIO_Port, CUT_12VOUT_Pin, GPIO_PIN_RESET);
		alive_call_fail = 0;
		PAD_boot_flag = 0;
	}
}

void Report_to_Server(int report_path) {
	int total = 0, sum = 0;

	for (int i = 0; i < 3; i++)
		sum += Bstate.cmd[i];

	for (int i = 0; i < 2; i++) {
		sum += Bstate.door[i];
		sum += Bstate.lock[i];
		sum += Bstate.scan[i];
	}
	sum += Bstate.end[0];
	total = sum;
	total = total & 0xff;
	total = ~total + 1;

	sprintf(Txbuffer, "%s%s%s%s%s", Bstate.cmd, Bstate.door, Bstate.lock, Bstate.scan, Bstate.end);
	printf(" Txbuffer: %s\r\n", Txbuffer);

	Txbuffer[10] = total;	//checksum

	if (report_path == REPORT_CMD) {
		if (Rxcplt_flag == 1) {
			HAL_UART_Transmit(&COMMPORT, (uint8_t*) Txbuffer,
					(uint16_t) strlen(Txbuffer), 0xFFFFFFFF);
		} else if (CDC_flag == 1) {
			CDC_Transmit_FS((uint8_t*) Txbuffer, (uint16_t) strlen(Txbuffer));
		}
	} else if (report_path == REPORT_DOOR) {
		CDC_Transmit_FS((uint8_t*) Txbuffer, (uint16_t) strlen(Txbuffer));
		HAL_UART_Transmit(&COMMPORT, (uint8_t*) Txbuffer,
				(uint16_t) strlen(Txbuffer), 0xFFFFFFFF);
	}

	/*
	 if(report_path == REPORT_CMD) {
	 if(Rxcplt_flag == 1) {
	 HAL_UART_Transmit(&COMMPORT, (uint8_t *)Txbuffer, (uint16_t)strlen(Txbuffer), 0xFFFFFFFF);
	 } else if(CDC_flag == 1) {
	 CDC_Transmit_FS((uint8_t*) Txbuffer, (uint16_t)strlen(Txbuffer));
	 }
	 } else if(report_path == REPORT_DOOR) {
	 CDC_Transmit_FS((uint8_t*) Txbuffer, (uint16_t)strlen(Txbuffer));
	 HAL_UART_Transmit(&COMMPORT, (uint8_t *)Txbuffer, (uint16_t)strlen(Txbuffer), 0xFFFFFFFF);
	 }
	 */
}

void Rfid_Scanning(int speed) {
	uint8_t i = 0;

	printf(" scanning...... \r\n");
	strcpy(Bstate.scan, "1,");  // ���� ����, ��ĵ��...
	Report_to_Server(REPORT_CMD);

	for (i = 0; i < (speed * 2); ++i) {
		HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
		HAL_Delay(500);
		HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
		HAL_Delay(500);
	}
	strcpy(Bstate.scan, "0,");  // ���� ����, ��ĵ ��...
	printf(" scan is completed!!\r\n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	// printf("RxINT_Routine");
	if (huart->Instance == COMMUART) {
		if (COMMDATA == EOT) {
			Rxbuffer[buf_index] = '\0';
			Rxcplt_flag = 1;
		} else if (COMMDATA != 0x00) {			// except NULL(0x00)
			Rxbuffer[buf_index++] = COMMDATA;
		}

		HAL_UART_Receive_IT(&COMMPORT, &COMMDATA, 1);

	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		time_second++;
		// HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
		// printf(" alive_call_fail: %d,  time: %d\r\n", alive_call_fail, time_second);
		// CDC_Transmit_FS("CDC_TX test...\r\n", sizeof("CDC_TX test...\r\n"));
	}
	if (htim->Instance == TIM7) {
		door_check_enable = 1;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	/* if(GPIO_Pin == EI5_TLIMIT_Pin) {
	 printf("TLIMIT click!!!\r\n");
	 } */

	if(GPIO_Pin == EI3_BLIMIT_Pin) {
	 printf("BLIMIT click!!!\r\n");
	 }

}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
