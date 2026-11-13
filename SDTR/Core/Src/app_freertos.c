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
#include "main.h"
//#include "usart.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for RealTimeTask */
osThreadId_t RealTimeTaskHandle;
const osThreadAttr_t RealTimeTask_attributes = {
  .name = "RealTimeTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 128 * 4
};
/* Definitions for CanCommTask */
osThreadId_t CanCommTaskHandle;
const osThreadAttr_t CanCommTask_attributes = {
  .name = "CanCommTask",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

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

  /* creation of CanCommTask */
  CanCommTaskHandle = osThreadNew(StartCanCommTask, NULL, &CanCommTask_attributes);

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

    for (;;)
    {
        BSP_LED_Toggle(LED_GREEN);

        osDelay(osKernelGetTickFreq() / 2U);
    }
  /* USER CODE END RealTimeTask */
}

/* USER CODE BEGIN Header_StartCanCommTask */
/**
* @brief Function implementing the CanCommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCanCommTask */
void StartCanCommTask(void *argument)
{
  /* USER CODE BEGIN CanCommTask */
  /* Infinite loop */
//  for(;;)
//  {
//    osDelay(1);
//  }
	    const uint32_t delay = osKernelGetTickFreq();

	    for (;;)
	    {
	        printf("CanCommTask is running\r\n");

	        osDelay(delay);

	}
  /* USER CODE END CanCommTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

