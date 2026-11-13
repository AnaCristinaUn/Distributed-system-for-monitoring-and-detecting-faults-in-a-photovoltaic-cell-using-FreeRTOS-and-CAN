/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_RX_READY_FLAG    (1U << 0) //1 binary; leaving space for other flags
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint8_t uart_rx_byte; //recieve buffer

static volatile uint32_t uart_rx_errors = 0;
static volatile uint32_t uart_tx_errors = 0;
/* USER CODE END Variables */
/* Definitions for RealTimeTask */
osThreadId_t RealTimeTaskHandle;
const osThreadAttr_t RealTimeTask_attributes = {
  .name = "RealTimeTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 128 * 4
};
/* Definitions for ModbusCommTask */
osThreadId_t ModbusCommTaskHandle;
const osThreadAttr_t ModbusCommTask_attributes = {
  .name = "ModbusCommTask",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartRealTimeTask(void *argument);
void StartModbusCommTask(void *argument);

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of RealTimeTask */
  RealTimeTaskHandle = osThreadNew(StartRealTimeTask, NULL, &RealTimeTask_attributes);

  /* creation of ModbusCommTask */
  ModbusCommTaskHandle = osThreadNew(StartModbusCommTask, NULL, &ModbusCommTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartRealTimeTask */
/**
* @brief Function implementing the RealTimeTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRealTimeTask */
void StartRealTimeTask(void *argument)
{
  /* USER CODE BEGIN RealTimeTask */
  /* Infinite loop */
//  for(;;)
//  {
//    osDelay(1);
//  }
	(void)argument;
    for (;;)
    {
        BSP_LED_Toggle(LED_GREEN);

        osDelay(osKernelGetTickFreq() / 2U);
    }
  /* USER CODE END RealTimeTask */
}

/* USER CODE BEGIN Header_StartModbusCommTask */
/**
* @brief Function implementing the ModbusCommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartModbusCommTask */
void StartModbusCommTask(void *argument)
{
  /* USER CODE BEGIN ModbusCommTask */
  /* Infinite loop */
  (void)argument;

  HAL_StatusTypeDef status; //enum defined by STM32
  	  	  	  	  	  	  	// common values: HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT
  uint32_t flags;           //stores result by osThreadFlagsWait()
  for(;;)
  {
	  	  	  	  	  	  	  	  	//Using interrupt based recepetion
	  status = HAL_UART_Receive_IT( //Get the message byte from rx
			  &huart2,              //Pointer to UART handle to get its config
			  &uart_rx_byte,       //Address of recieve buffer
			  1U
			  );

	  if (status != HAL_OK)         //Check correct recepetion start
	  {
		  uart_rx_errors++;
		  osDelay(10U);
		  continue;
	  }

	  do{
		  flags = osThreadFlagsWait(
				  UART_RX_READY_FLAG, //Flag we wait for
				  osFlagsWaitAny,     //Wakes on any flags set, theres only one right now so when we get the RX flag
				  osWaitForever       //Waits indefently
				  );
	  }

	  while ((flags & osFlagsError) !=0U); //Repeat if an error is returned

	  if (uart_rx_byte >= 'a' && uart_rx_byte <= 'z')
	  {
	      uart_rx_byte = (uint8_t)(uart_rx_byte - ('a' - 'A')); //For easier testing
	  }

	  status = HAL_UART_Transmit( //Sends back the recieved byte, for testing
			  &huart2,
			  &uart_rx_byte,      //Finds the byte in the buffer
			  1U,
			  100U //Timeout of 100 milisecs
			  );
	  if (status != HAL_OK)
	         {
	             uart_tx_errors++;
	         }
  }
  /* USER CODE END ModbusCommTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        (void)osThreadFlagsSet(
            ModbusCommTaskHandle,
            UART_RX_READY_FLAG
        );
    }
}
/* USER CODE END Application */

