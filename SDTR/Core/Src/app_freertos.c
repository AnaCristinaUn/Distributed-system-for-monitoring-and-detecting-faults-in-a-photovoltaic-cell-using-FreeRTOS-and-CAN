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
#include "measurement_data.h" // To be able to use the shared struct
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_RX_QUEUE_LENGTH 256U
#define TELEMETRY_PERIOD_MS 1000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

static MeasurementSnapshot_t measurementSnapshot; // Shared item between the two tasks for measurements
static osMutexId_t measurementMutexHandle;        // Mutex declaration

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
const osThreadAttr_t RealTimeTask_attributes = {//
  .name = "RealTimeTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 1024
};
/* Definitions for ModbusCommTask */
osThreadId_t ModbusCommTaskHandle;
const osThreadAttr_t ModbusCommTask_attributes = {
  .name = "ModbusCommTask",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 1024
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartRealTimeTask(void *argument);
void StartModbusCommTask(void *argument);
static void PublishMeasurementSnapshot(
    const MeasurementSnapshot_t *newSnapshot
);
static uint8_t ReadMeasurementSnapshot(
    MeasurementSnapshot_t *snapshotCopy
);

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) { //Where we intialise RTOS objects before starting the scheduler
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	measurementMutexHandle = osMutexNew(NULL);

	if (measurementMutexHandle == NULL)
	{
	    Error_Handler();
	}
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
	    sizeof(uint8_t), //
	    NULL
	);

	if (uartRxQueueHandle == NULL) //Prevents application from starting without a valid queue handle
	{
	    Error_Handler();
	}
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
    uint32_t sequence = 0U;
    for (;;) //Forever
    {
        MeasurementSnapshot_t newSnapshot = {0};

        sequence++;//Increment ID

        newSnapshot.sequence_number = sequence; //Give number to ID
        newSnapshot.timestamp_ms = osKernelGetTickCount(); //Get time to add timestamp

        /* Temporary test values */
        newSnapshot.voltage_mV = 5000U;
        newSnapshot.current_mA = 128U;
        newSnapshot.power_mW = 640U;

        newSnapshot.fault_state = PV_STATE_NORMAL;
        newSnapshot.data_valid = 1U;

        PublishMeasurementSnapshot(&newSnapshot); //Using the task to publish values through MUTEX

        BSP_LED_Toggle(LED_GREEN);

        osDelay(500U);
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

    uint8_t received_byte; //Data from pc
    HAL_StatusTypeDef status; //For debug
    uint32_t last_telemetry_tick = osKernelGetTickCount(); //Get time

    // Start interrupt-based UART reception
    do
    {
        status = HAL_UART_Receive_IT(//Using interrupt
            &huart2,
            &uart_rx_byte,
            1U
        );

        if (status != HAL_OK)
        {
            uart_rx_start_errors++;
            osDelay(10U); //Keep trying
        }

    } while (status != HAL_OK);

    for (;;)
    {
        // Check for received UART bytes without blocking
        while (osMessageQueueGet(
                   uartRxQueueHandle,
                   &received_byte,
                   NULL,
                   0U
               ) == osOK)
        {
            //   Command processing will be implemented later.
        }

        // Check if the telemetry period has elapsed
        uint32_t current_tick = osKernelGetTickCount();

        uint32_t telemetry_ticks =
            (TELEMETRY_PERIOD_MS * osKernelGetTickFreq()) / 1000U;

        if ((current_tick - last_telemetry_tick) >= telemetry_ticks)
        {
            last_telemetry_tick = current_tick;

            MeasurementSnapshot_t localSnapshot;
            char tx_buffer[128];
            int tx_length;

            if (ReadMeasurementSnapshot(&localSnapshot) != 0U)
            {
                tx_length = snprintf(
                    tx_buffer,
                    sizeof(tx_buffer),
                    "SEQ=%lu TIME=%lu V=%lu mV I=%lu mA P=%lu mW STATE=%u VALID=%u\r\n",
                    (unsigned long)localSnapshot.sequence_number,
                    (unsigned long)localSnapshot.timestamp_ms,
                    (unsigned long)localSnapshot.voltage_mV,
                    (unsigned long)localSnapshot.current_mA,
                    (unsigned long)localSnapshot.power_mW,
                    (unsigned int)localSnapshot.fault_state,
                    (unsigned int)localSnapshot.data_valid
                );

                if (tx_length > 0 &&
                    tx_length < (int)sizeof(tx_buffer))
                {
                    status = HAL_UART_Transmit(
                        &huart2,
                        (uint8_t *)tx_buffer,
                        (uint16_t)tx_length,
                        100U
                    );

                    if (status != HAL_OK)
                    {
                        uart_tx_errors++;
                    }
                }
            }
        }

        osDelay(1U);
    }
}



/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void PublishMeasurementSnapshot( // Used by the measuring task to publish message
    const MeasurementSnapshot_t *newSnapshot // New snapshot
)
{
    if (newSnapshot == NULL)
    {
        return;
    }

    if (osMutexAcquire(measurementMutexHandle, osWaitForever) == osOK)//Get control of the Mutex
    {
        measurementSnapshot = *newSnapshot; //Set the measurementSnapshot as the newly constructed values.

        osMutexRelease(measurementMutexHandle);//Release the mutex
    }
}

static uint8_t ReadMeasurementSnapshot(
    MeasurementSnapshot_t *snapshotCopy
)
{
    if (snapshotCopy == NULL)
    {
        return 0U;
    }

    if (osMutexAcquire(measurementMutexHandle, osWaitForever) == osOK) //Get control of the Mutex
    {
        *snapshotCopy = measurementSnapshot; //Copy the values in the local snapshotCopy

        osMutexRelease(measurementMutexHandle); //Release the Mutex

        return 1U;
    }

    return 0U;
}

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

