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
#define UART_RX_QUEUE_LENGTH 256U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint8_t uart_rx_byte; //recieve buffer

static osMessageQueueId_t uartRxQueueHandle; //Identifies RTOS message queue

static volatile uint32_t uart_rx_queue_overflows = 0U; //Counts bytes lost because the queue was full
static volatile uint32_t uart_rx_start_errors = 0U; //Counts failures when initially starting reception
static volatile uint32_t uart_rx_rearm_errors = 0U; //Counts failures when restarting reception in the callback
static volatile uint32_t uart_tx_errors = 0U; //Counts transmission failures
static volatile uint32_t uart_rx_error_count = 0U;
static volatile uint32_t uart_rx_last_error = 0U;
static volatile uint32_t uart_rx_error_flags = 0U;
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
	uartRxQueueHandle = osMessageQueueNew(//Creates queue of 256 elements, each one byte in size
	    UART_RX_QUEUE_LENGTH, //256
	    sizeof(uint8_t),
	    NULL
	);
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
    (void)argument;

    uint8_t received_byte;
    HAL_StatusTypeDef status;

    /* Start interrupt-based UART reception */
    do
    {
        status = HAL_UART_Receive_IT(
            &huart2,
            &uart_rx_byte,
            1U
        );

        if (status != HAL_OK)
        {
            uart_rx_start_errors++;
            osDelay(10U);
        }

    } while (status != HAL_OK);

    /* Process received bytes */
    for (;;)
    {
        if (osMessageQueueGet(
                uartRxQueueHandle,
                &received_byte,
                NULL,
                osWaitForever
            ) == osOK)
        {
            /* Convert lowercase ASCII characters to uppercase */
            if (received_byte >= 'a' && received_byte <= 'z')
            {
                received_byte = (uint8_t)(
                    received_byte - ('a' - 'A')
                );
            }

            /* Echo the processed byte to the PC */
            status = HAL_UART_Transmit(
                &huart2,
                &received_byte,
                1U,
                100U
            );

            if (status != HAL_OK)
            {
                uart_tx_errors++;
            }
        }
    }
}



/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
  void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  {
      if (huart->Instance == USART2)
      {
          uint8_t received_byte = uart_rx_byte;

          // Put received byte into the RTOS queue
          if (osMessageQueuePut(
                  uartRxQueueHandle,
                  &received_byte,
                  0U,
                  0U //An interrupt msut never wait for queue space
              ) != osOK)
          {
              uart_rx_queue_overflows++;
          }

          // Immediately prepare reception of the next byte
          if (HAL_UART_Receive_IT(
                  &huart2,
                  &uart_rx_byte,
                  1U
              ) != HAL_OK)
          {
              uart_rx_rearm_errors++;
          }
      }
  }

  void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
  {
      if (huart->Instance == USART2)
      {
          uint32_t error = huart->ErrorCode;

          uart_rx_error_count++;
          uart_rx_last_error = error;
          uart_rx_error_flags |= error;
      }
  }
/* USER CODE END Application */

