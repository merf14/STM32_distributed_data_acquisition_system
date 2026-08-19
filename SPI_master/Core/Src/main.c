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
#define BUFFER_SIZE 1
#define PCF8574_address 0x27
#define REC_CODE 0
#define SENT_CODE 1
#define HUNIDITY_CODE 2
#define TEMPERATURE_CODE 3
#include "task.h"
#include "string.h"
#include "stdio.h"
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
I2C_HandleTypeDef hi2c1;

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
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for SPI_Recieve */
osThreadId_t SPI_RecieveHandle;
const osThreadAttr_t SPI_Recieve_attributes = {
  .name = "SPI_Recieve",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for display */
osThreadId_t displayHandle;
const osThreadAttr_t display_attributes = {
  .name = "display",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Queue_LCD */
osMessageQueueId_t Queue_LCDHandle;
const osMessageQueueAttr_t Queue_LCD_attributes = {
  .name = "Queue_LCD"
};
/* Definitions for QueueRecieveSPI */
osMessageQueueId_t QueueRecieveSPIHandle;
const osMessageQueueAttr_t QueueRecieveSPI_attributes = {
  .name = "QueueRecieveSPI"
};
/* Definitions for QueueTransmit */
osMessageQueueId_t QueueTransmitHandle;
const osMessageQueueAttr_t QueueTransmit_attributes = {
  .name = "QueueTransmit"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
void StartDefaultTask(void *argument);
void Start_SPI_Transmit(void *argument);
void Start_SPI_Recieve(void *argument);
void Start_display(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
	{
	    if (hspi->Instance == SPI1)
	    {

	        BaseType_t xHigherPriorityTaskWoken = pdTRUE;

	        uint16_t toSend = (REC_CODE<<8)|receiveBuffer[0];
	        xQueueSendFromISR( Queue_LCDHandle, ( void * ) &toSend, &xHigherPriorityTaskWoken);
	        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	    }

	}

uint32_t task1Cnt = 0;
	uint32_t task2Cnt = 0;
	LCD1602 scr;
 // Функция sendData отправляет байт данных по шине I2C
// pData - отправляемый на расширитель портов байт, например 0x25,
// где верхний полубайт 2(0b0010) соответствует линиям DB7-DB4 дисплея,
// а нижний полубайт 5(0b0101) соответствует линиям LED, E, RW, RS дисплея
void sendData(LCD1602 * scr, uint8_t * pData) {
    * pData |= (1 << E); // установка синхросигнала E=1
    while (HAL_I2C_Master_Transmit(scr -> hi2c, scr -> DevAddress, pData, 1,
            1000) != HAL_OK) {}
    HAL_Delay(25);
    * pData &= ~(1 << E); // установка синхросигнала E=0
    while (HAL_I2C_Master_Transmit(scr -> hi2c, scr -> DevAddress, pData, 1,
            1000) != HAL_OK) {}
    HAL_Delay(25);
}
// Функция начальной инициализации дисплея scr,
// выполняет инструкции в соответствии с документацией
void initLCD(LCD1602 * scr) {
    uint8_t buf;
    buf = 0x30; // первый отправляемый байт 0b00110000. Верхний полубайт представляет данные
    // нижний полубайт 0000 представляет сигналы RS,RW,E,LED
    HAL_Delay(100); // ожидание установки питающего напряжения (например, после включения устройства)
    sendData(scr, & buf); // отправка три раза байта 0b00110000
    sendData(scr, & buf); // в соответствии с инструкцией по инициализации дисплея
    sendData(scr, & buf);
    buf = 0x20; // команда 0b00100000, выбор 4-х битного интерфейса
    sendData(scr, & buf);
    buf = 0x20; // 0b00100000, выбор количества строк дисплея и шрифт
    sendData(scr, & buf);
    buf = 0xC0; // 0b11000000, N=1(две строки), F=1(5*10)
    sendData(scr, & buf);
    buf = 0; // выключение дисплея в соответствии с инструкцией
    sendData(scr, & buf);
    buf = 0x80;
    sendData(scr, & buf);
    buf = 0; // очистка дисплея
    sendData(scr, & buf);
    buf = 0x10;
    sendData(scr, & buf);
    //I/D - установка направления движения курсора после ввода символа (1 - влево, 0 - вправо)
    //S - сдвиг курсора сопровождается сдвигом символов
    buf = 0;
    sendData(scr, & buf);
    buf = 0x30;
    sendData(scr, & buf);
    buf = 0; // включение дисплея
    sendData(scr, & buf);
    buf = 0xC8;
    sendData(scr, & buf);
}
// Функция writeLCD выводит символ s на дисплей scr
void writeLCD(LCD1602 * scr, uint8_t s) {
    uint8_t command;
    command = ((s & 0xf0) | 0x09); // верхний полубайт для дисплея
    sendData(scr, & command);
    command = ((s & 0x0f) << 4) | 0x09; // нижний полубайт для дисплея
    sendData(scr, & command);
}
// Функция writeStringLCD выводит строку str на дисплей src
void writeStringLCD(LCD1602 * scr, char * str) {
    for (int i = 0; i < strlen(str); i++) {
        writeLCD(scr, str[i]);
    }
}
// Функция moveXY перемещает курсор на позицию X,Y
void moveXY(LCD1602 * scr, uint8_t x, uint8_t y) {
    uint8_t command, adr;
    // проверка выхода X,Y за пределы максимальных значений
    if (y > 1) y = 1;
    if (x > 39) x = 39;
    // перевод координат X,Y в адрес памяти DDRAM
    if (y == 0)
        adr = x;
    if (y == 1)
        adr = x + 0x40;
    // так как используется 4-битный интерфейс, формируются 2 команды для перемещения в нужную позицию DDRAM
    command = ((adr & 0xf0) | 0x80) | 0x08; //adr&0xf0 выделение старшей тетрады,
    //|0x80 установка старшего бита (DB7=1)
    //|0x08 добавление 8 (1000 - LED,E,RW,RS) в младшую тетраду
    sendData(scr, & command);
    command = (adr << 4) | 0x08;
    sendData(scr, & command);
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
  scr.hi2c = &hi2c1;
  		scr.DevAddress = (PCF8574_address<<1);
  		transmitBuffer[0]=0;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  initLCD(&scr);

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
  /* creation of Queue_LCD */
  Queue_LCDHandle = osMessageQueueNew (16, sizeof(uint16_t), &Queue_LCD_attributes);

  /* creation of QueueRecieveSPI */
  QueueRecieveSPIHandle = osMessageQueueNew (8, sizeof(uint8_t), &QueueRecieveSPI_attributes);

  /* creation of QueueTransmit */
  QueueTransmitHandle = osMessageQueueNew (16, sizeof(uint8_t), &QueueTransmit_attributes);

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

  /* creation of display */
  displayHandle = osThreadNew(Start_display, NULL, &display_attributes);

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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
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
  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : Led_Pin */
  GPIO_InitStruct.Pin = Led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Temp_button_Pin Humidity_button_Pin */
  GPIO_InitStruct.Pin = Temp_button_Pin|Humidity_button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint16_t toSend;
	 if (GPIO_Pin == Humidity_button_Pin)
	 {
		 toSend = HUNIDITY_CODE;
	 }
	 else if (GPIO_Pin == Temp_button_Pin)
	 {
		 toSend = TEMPERATURE_CODE;
	 }
	 BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	 xQueueSendFromISR( QueueTransmitHandle, ( void * ) &toSend, &xHigherPriorityTaskWoken);
	 portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

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

  /* Infinite loop */
  for(;;)
  {
	if(uxQueueMessagesWaiting(QueueTransmitHandle)>1){
		uint8_t Recieved;
		HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);
		xQueueReceive( QueueTransmitHandle, &( Recieved ), portMAX_DELAY );
		transmitBuffer[0] = Recieved;
		HAL_SPI_Transmit_IT(&hspi1, transmitBuffer, BUFFER_SIZE);
		uint16_t toSend = (SENT_CODE<<8)|transmitBuffer[0];
		xQueueSend( Queue_LCDHandle, ( void * ) &toSend, portMAX_DELAY  );
		uint8_t toSend2 = 0;
		xQueueSend( QueueRecieveSPIHandle, ( void * ) &toSend2, portMAX_DELAY  );
	}
	HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
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
	  if(uxQueueMessagesWaiting(QueueRecieveSPIHandle)>1){
		  uint16_t Recieved;
		  xQueueReceive( QueueRecieveSPIHandle, &( Recieved ), portMAX_DELAY );
		  HAL_SPI_Receive_IT(&hspi1, receiveBuffer, BUFFER_SIZE);
		  osDelay(30);
	  }
	  osDelay(30);
  }
  /* USER CODE END Start_SPI_Recieve */
}

/* USER CODE BEGIN Header_Start_display */
/**
* @brief Function implementing the display thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_display */
void Start_display(void *argument)
{
  /* USER CODE BEGIN Start_display */
  /* Infinite loop */
  for(;;)
  {
	  if(uxQueueMessagesWaiting(Queue_LCDHandle)>1)
	  {

		  uint16_t Recieved;
		  xQueueReceive( Queue_LCDHandle, &( Recieved ), portMAX_DELAY );
		  if ((Recieved>>8)==(REC_CODE)){
			  char str[13];
			  snprintf(str, 13, "Recieved %u  ", (Recieved&255));
			  moveXY(&scr,0,1);
			  writeStringLCD(&scr,str);
		  }
		  else if ((Recieved>>8)==(SENT_CODE)){
			  char str[9];
			  snprintf(str, 9, "Sent %u  ", (Recieved&255));
			  moveXY(&scr,0,0);
			  writeStringLCD(&scr,str);
		  }
	  }
	  osDelay(30);

  }
  /* USER CODE END Start_display */
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
