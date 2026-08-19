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
#define BUFFER_SIZE 2
#define DHT11_CODE 2
#define HCSR04_CODE 3
#include "task.h"
#include "queue.h"
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

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for SPI_Transmit */
osThreadId_t SPI_TransmitHandle;
const osThreadAttr_t SPI_Transmit_attributes = {
  .name = "SPI_Transmit",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SPI_Recieve */
osThreadId_t SPI_RecieveHandle;
const osThreadAttr_t SPI_Recieve_attributes = {
  .name = "SPI_Recieve",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for TaskDHT11 */
osThreadId_t TaskDHT11Handle;
const osThreadAttr_t TaskDHT11_attributes = {
  .name = "TaskDHT11",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for QueueTransmit */
osMessageQueueId_t QueueTransmitHandle;
const osMessageQueueAttr_t QueueTransmit_attributes = {
  .name = "QueueTransmit"
};
/* Definitions for QueueDHT11 */
osMessageQueueId_t QueueDHT11Handle;
const osMessageQueueAttr_t QueueDHT11_attributes = {
  .name = "QueueDHT11"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
void StartDefaultTask(void *argument);
void Start_SPI_Transmit(void *argument);
void Start_SPI_Recieve(void *argument);
void StartTaskDHT11(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t task1Cnt = 0;
	uint32_t task2Cnt = 0;

	void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
	{
	    if (hspi->Instance == SPI1)
	    {

	        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	        uint8_t toSend = receiveBuffer[0];

	        xQueueSendFromISR(
	        		QueueDHT11Handle,
	            &toSend,
	            &xHigherPriorityTaskWoken
	        );

	        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	    }
	}

	void DHT11_Start (void)
	{
		 GPIO_InitTypeDef GPIO_InitStruct = {0};

		  GPIO_InitStruct.Pin = DHT11_Pin;
		  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		  GPIO_InitStruct.Pull = GPIO_PULLUP;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);

		HAL_GPIO_WritePin (DHT11_GPIO_Port, DHT11_Pin, 0);   // pull the pin low
		HAL_Delay (18000);   // wait for 18ms

		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}

	uint8_t DHT11_Check_Response (void)
	{
		uint8_t Response = 0;
		HAL_Delay (40);
		if (!(HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)))
		{
			HAL_Delay (80);
			if ((HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin))) Response = 1;
			else Response = -1;
		}
		while ((HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)));   // wait for the pin to go low

		return Response;
	}

	uint8_t DHT11_Read (void)
	{
		uint8_t i,j;
		for (j=0;j<8;j++)
		{
			while (!(HAL_GPIO_ReadPin (DHT11_GPIO_Port, DHT11_Pin)));   // wait for the pin to go high
			HAL_Delay (40);   // wait for 40 us
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
  /* USER CODE BEGIN 2 */



  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of QueueTransmit */
  QueueTransmitHandle = osMessageQueueNew (16, sizeof(uint16_t), &QueueTransmit_attributes);

  /* creation of QueueDHT11 */
  QueueDHT11Handle = osMessageQueueNew (16, sizeof(uint16_t), &QueueDHT11_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of SPI_Transmit */
  SPI_TransmitHandle = osThreadNew(Start_SPI_Transmit, NULL, &SPI_Transmit_attributes);

  /* creation of SPI_Recieve */
  SPI_RecieveHandle = osThreadNew(Start_SPI_Recieve, NULL, &SPI_Recieve_attributes);

  /* creation of TaskDHT11 */
  TaskDHT11Handle = osThreadNew(StartTaskDHT11, NULL, &TaskDHT11_attributes);

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
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
  HAL_GPIO_WritePin(Led2_GPIO_Port, Led2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : Led2_Pin */
  GPIO_InitStruct.Pin = Led2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Led2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT11_Pin */
  GPIO_InitStruct.Pin = DHT11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);

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

/* USER CODE BEGIN Header_Start_SPI_Transmit */
/**
* @brief Function implementing the SPI_Transmit thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_SPI_Transmit */
void Start_SPI_Transmit(void *argument)
{
  /* USER CODE BEGIN Start_SPI_Transmit */
	uint16_t Recieved;
  /* Infinite loop */
  for(;;)
  {

	  if(uxQueueMessagesWaiting(QueueTransmitHandle)>1)
	  	  {
	  		  xQueueReceive( QueueTransmitHandle, &( Recieved ), portMAX_DELAY );
	  		transmitBuffer[0] = Recieved;
	  		transmitBuffer[1] = Recieved>>8;
	  		  HAL_SPI_Transmit_IT(&hspi1, transmitBuffer, BUFFER_SIZE);

	  	  }
    osDelay(30);
  }
  /* USER CODE END Start_SPI_Transmit */
}

/* USER CODE BEGIN Header_Start_SPI_Recieve */
/**
* @brief Function implementing the SPI_Recieve thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_SPI_Recieve */
void Start_SPI_Recieve(void *argument)
{
  /* USER CODE BEGIN Start_SPI_Recieve */
  /* Infinite loop */
  for(;;)
  {
	  HAL_SPI_Receive_IT(&hspi1, receiveBuffer, BUFFER_SIZE);
    osDelay(10);
  }
  /* USER CODE END Start_SPI_Recieve */
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
	uint16_t Recieved;
	uint8_t Presence,Rh_byte1,Rh_byte2,Temp_byte1,Temp_byte2,SUM;
	DHT11_Start();
  /* Infinite loop */
  for(;;)
  {
	  if(uxQueueMessagesWaiting(QueueDHT11Handle)>1)
	  	  	  {

	  	  		  xQueueReceive( QueueDHT11Handle, &( Recieved ), portMAX_DELAY );

	  	  		      Presence = DHT11_Check_Response();
	  	  		      Rh_byte1 = DHT11_Read ();
	  	  		      Rh_byte2 = DHT11_Read ();
	  	  		      Temp_byte1 = DHT11_Read ();
	  	  		      Temp_byte2 = DHT11_Read ();
	  	  		      SUM = DHT11_Read();
	  	  		      uint16_t toSend = (Temp_byte1<<8)|Rh_byte1;
	  	  		      xQueueSend( QueueTransmitHandle, ( void * ) &toSend, portMAX_DELAY  );
	  	  		  HAL_GPIO_WritePin(Led2_GPIO_Port, Led2_Pin, GPIO_PIN_RESET);
	  	  		  	        osDelay(50);
	  	  		  	        HAL_GPIO_WritePin(Led2_GPIO_Port, Led2_Pin, GPIO_PIN_SET);
	  	  	  }
    osDelay(30);
  }
  /* USER CODE END StartTaskDHT11 */
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
