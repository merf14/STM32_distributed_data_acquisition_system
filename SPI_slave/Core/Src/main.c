/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define BUFFER_SIZE 3
#define DHT11_CODE 0
#define HCSR04_CODE 1
#define RESPONSE_CODE 2
#include "task.h"
#include "queue.h"
#include "semphr.h"
uint8_t transmitBuffer[BUFFER_SIZE];
uint8_t receiveBuffer[BUFFER_SIZE];


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
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for SPI_Transaction */
osThreadId_t SPI_TransactionHandle;
const osThreadAttr_t SPI_Transaction_attributes = {
  .name = "SPI_Transaction",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DHT11 */
osThreadId_t DHT11Handle;
const osThreadAttr_t DHT11_attributes = {
  .name = "DHT11",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for HCSR04 */
osThreadId_t HCSR04Handle;
const osThreadAttr_t HCSR04_attributes = {
  .name = "HCSR04",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for QueueTransmit */
osMessageQueueId_t QueueTransmitHandle;
const osMessageQueueAttr_t QueueTransmit_attributes = {
  .name = "QueueTransmit"
};
/* Definitions for BinarySemDHT11 */
osSemaphoreId_t BinarySemDHT11Handle;
const osSemaphoreAttr_t BinarySemDHT11_attributes = {
  .name = "BinarySemDHT11"
};
/* Definitions for BinarySemHCSR04 */
osSemaphoreId_t BinarySemHCSR04Handle;
const osSemaphoreAttr_t BinarySemHCSR04_attributes = {
  .name = "BinarySemHCSR04"
};
/* Definitions for BinarySemRESP */
osSemaphoreId_t BinarySemRESPHandle;
const osSemaphoreAttr_t BinarySemRESP_attributes = {
  .name = "BinarySemRESP"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
void StartDefaultTask(void *argument);
void StartTaskSPI_Transaction(void *argument);
void StartTaskDHT11(void *argument);
void StartTaskHCSR04(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

sr04_t sr04 = {
    .trig_port = HCSR04_Trig_GPIO_Port,
    .trig_pin = HCSR04_Trig_Pin,
	.echo_port = HCSR04_Echo_GPIO_Port,
	.echo_pin = HCSR04_Echo_Pin,
    .echo_htim = & htim3,
    .capture_flag = 0,
    .time = 0,
    .last_time = 20,
    .counter = 0,
};

uint32_t task1Cnt = 0;
	uint32_t task2Cnt = 0;

	void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
	{
	    if (hspi->Instance == SPI1)
	    {
	    	BaseType_t xHigherPriorityTaskWoken = pdTRUE;

	        switch (receiveBuffer[0]) {
	        	case DHT11_CODE:
	        		xSemaphoreGiveFromISR(BinarySemDHT11Handle, &xHigherPriorityTaskWoken);
	        		break;
	        	case HCSR04_CODE:
	        		xSemaphoreGiveFromISR(BinarySemHCSR04Handle, &xHigherPriorityTaskWoken);
	        		break;
	        	case RESPONSE_CODE:
	        		xSemaphoreGiveFromISR(BinarySemRESPHandle, &xHigherPriorityTaskWoken);
	        		break;
	        	default:
	        		break;
	        }
	        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	    }
	}

	void sr04_trigger(sr04_t * sr04) {
	    if (!(sr04 -> capture_flag)) {
	        HAL_GPIO_WritePin(sr04 -> trig_port, sr04 -> trig_pin, GPIO_PIN_SET);
	        HAL_Delay(1);
	        HAL_GPIO_WritePin(sr04 -> trig_port, sr04 -> trig_pin, GPIO_PIN_RESET);
	    } else {
	        (sr04 -> counter) ++;
	        if ((sr04 -> counter) == 5) {
	            sr04 -> counter = 0;
	            sr04 -> capture_flag = 0;
	            sr04 -> echo_htim -> State = HAL_TIM_STATE_READY;
	        }
	    }
	}

	void sr04_measure(sr04_t * sr04) {
		uint32_t signal_time;
	    if (sr04 -> echo_htim -> State == HAL_TIM_STATE_READY) //start timer if stopped
	    {
	        sr04 -> capture_flag = 1;
	        __HAL_TIM_SET_COUNTER(sr04 -> echo_htim, 0x0000); // обнуление счётчика
	        HAL_TIM_Base_Start_IT(sr04 -> echo_htim);

	    } else if (sr04 -> echo_htim -> State == HAL_TIM_STATE_BUSY) //stop timer if started
	    {
	        HAL_TIM_Base_Stop_IT(sr04 -> echo_htim);
	        signal_time = __HAL_TIM_GET_COUNTER(sr04 -> echo_htim);
	        sr04 -> time = signal_time / 58;

	        sr04 -> last_time = sr04 -> time;
	        sr04 -> capture_flag = 0;
	    }
	}
	void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	    if (GPIO_Pin == sr04.echo_pin) {
	        sr04_measure( & sr04);
	    }
	}

	void DelayUS(uint32_t us) {
	 uint32_t start = TIM2->CNT;
	 while (TIM2->CNT - start < us);
	}

	void DHT11_Start (void)
	{
		 GPIO_InitTypeDef GPIO_InitStruct = {0};

		  GPIO_InitStruct.Pin = DHT11_Pin;
		  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		  GPIO_InitStruct.Pull = GPIO_PULLUP;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);

		HAL_GPIO_WritePin (DHT11_GPIO_Port, DHT11_Pin, 0);
		HAL_Delay (18);   // wait for 18ms

		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
	}

	uint8_t DHT11_Check_Response (void)
	{
		uint8_t Response = 0;
		DelayUS(40);
		if (!(HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)))
		{
			DelayUS(80);
			if ((HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin))) Response = 1;
			else Response = -1;
		}
		while ((HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)));   // wait for the pin to go low

		return Response;
	}

	uint8_t DHT11_Read (void)
	{
		uint8_t i,j;
		i=0;
		for (j=0;j<8;j++)
		{
			while (!(HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)));   // wait for the pin to go high
			DelayUS (40);   // wait for 40 us
			if (!(HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)))   // if the pin is low
			{
				i&= ~(1<<(7-j));   // write 0
			}
			else i|= (1<<(7-j));  // if the pin is high, write 1
			while ((HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)));  // wait for the pin to go low
		}
		return i;
	}
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
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_IC_Start_IT( & htim3, TIM_CHANNEL_1);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of BinarySemDHT11 */
  BinarySemDHT11Handle = osSemaphoreNew(1, 0, &BinarySemDHT11_attributes);

  /* creation of BinarySemHCSR04 */
  BinarySemHCSR04Handle = osSemaphoreNew(1, 0, &BinarySemHCSR04_attributes);

  /* creation of BinarySemRESP */
  BinarySemRESPHandle = osSemaphoreNew(1, 0, &BinarySemRESP_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of QueueTransmit */
  QueueTransmitHandle = osMessageQueueNew (16, sizeof(uint32_t), &QueueTransmit_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of SPI_Transaction */
  SPI_TransactionHandle = osThreadNew(StartTaskSPI_Transaction, NULL, &SPI_Transaction_attributes);

  /* creation of DHT11 */
  DHT11Handle = osThreadNew(StartTaskDHT11, NULL, &DHT11_attributes);

  /* creation of HCSR04 */
  HCSR04Handle = osThreadNew(StartTaskHCSR04, NULL, &HCSR04_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 31;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 31;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(HCSR04_Trig_GPIO_Port, HCSR04_Trig_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : Led_Pin */
  GPIO_InitStruct.Pin = Led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HCSR04_Echo_Pin */
  GPIO_InitStruct.Pin = HCSR04_Echo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(HCSR04_Echo_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HCSR04_Trig_Pin */
  GPIO_InitStruct.Pin = HCSR04_Trig_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HCSR04_Trig_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT11_Pin */
  GPIO_InitStruct.Pin = DHT11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTaskSPI_Transaction */
/**
* @brief Function implementing the SPI_Transaction thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskSPI_Transaction */
void StartTaskSPI_Transaction(void *argument)
{
  /* USER CODE BEGIN StartTaskSPI_Transaction */
	uint32_t Received;
	HAL_SPI_TransmitReceive_IT(&hspi1, transmitBuffer, receiveBuffer, BUFFER_SIZE);
  /* Infinite loop */
  for(;;)
  {

	  	xQueueReceive( QueueTransmitHandle, &( Received ), portMAX_DELAY );
	  	transmitBuffer[0] = Received;
	  	transmitBuffer[1] = Received<<8;
	  	transmitBuffer[2] = Received<<16;
	  	HAL_SPI_TransmitReceive_IT(&hspi1, transmitBuffer, receiveBuffer, BUFFER_SIZE);
	  	xSemaphoreTake(BinarySemRESPHandle, portMAX_DELAY);
	  	HAL_SPI_TransmitReceive_IT(&hspi1, transmitBuffer, receiveBuffer, BUFFER_SIZE);
  }
  /* USER CODE END StartTaskSPI_Transaction */
}

/* USER CODE BEGIN Header_StartTaskDHT11 */
/**
* @brief Function implementing the TaskDHT11 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskDHT11 */
void StartTaskDHT11(void *argument)
{
  /* USER CODE BEGIN StartTaskDHT11 */
	uint8_t Presence,Rh_byte1,Rh_byte2,Temp_byte1,Temp_byte2,SUM;
  /* Infinite loop */
  for(;;)
  {
	 xSemaphoreTake(BinarySemDHT11Handle, portMAX_DELAY);
	 HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);

	 DHT11_Start();
	 Presence = DHT11_Check_Response();
	 Rh_byte1 = DHT11_Read ();
	 Rh_byte2 = DHT11_Read ();
	 Temp_byte1 = DHT11_Read ();
	 Temp_byte2 = DHT11_Read ();
	 SUM = DHT11_Read();

	 uint32_t toSend = (DHT11_CODE<<16)|(Rh_byte1<<8)|Temp_byte1;
	 HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
	 xQueueSend( QueueTransmitHandle, ( void * ) &toSend, portMAX_DELAY  );
  }
  /* USER CODE END StartTaskDHT11 */
}

/* USER CODE BEGIN Header_StartTaskHCSR04 */
/**
* @brief Function implementing the HCSR04 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskHCSR04 */
void StartTaskHCSR04(void *argument)
{
  /* USER CODE BEGIN StartTaskHCSR04 */
  /* Infinite loop */
  for(;;)
  {
	  xSemaphoreTake(BinarySemHCSR04Handle, portMAX_DELAY);
	  sr04_trigger( & sr04);
	  osDelay(10);

	  uint32_t toSend = (HCSR04_CODE<<16)|sr04.time/58;
	  xQueueSend( QueueTransmitHandle, ( void * ) &toSend, portMAX_DELAY  );
  }
  /* USER CODE END StartTaskHCSR04 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
