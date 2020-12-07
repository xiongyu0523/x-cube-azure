/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main program body.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

extern void MX_USART1_UART_Init(void);

/* Global variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;
RNG_HandleTypeDef hrng;
UART_HandleTypeDef console_uart;

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static volatile uint8_t button_flags = 0;
static volatile uint8_t joystick_down_flags = 0;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

static void Console_UART_Init(void);
static void RTC_Init(void);
static void Button_ISR(void);
static void MainThread(void const * argument);
static void MX_GPIO_Init(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval Status
  */
int main(void)
{
  unsigned int random_number = 0;
  osThreadId MainThreadId = NULL;

  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();

  Periph_Config();
  
  BSP_LED_Init(LED_GREEN);
  BSP_JOY_Init(JOY_MODE_EXTI);
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  
  /* RNG init function */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  else if (HAL_RNG_GenerateRandomNumber(&hrng, (uint32_t *) &random_number) == HAL_OK)
  {
    /* Initialize the seed of the stdlib rand() SW implementation. */
    srand(random_number);
  }
  
  /* RTC init */
  RTC_Init();
  /* UART console init */
  Console_UART_Init();

#if (defined(__GNUC__) && !defined(__CC_ARM))
  /* Do not buffer stdout, so that single chars are output without any delay to the console. */
  setvbuf(stdout, NULL, _IONBF, 0);
#endif

   /* Init thread */
  osThreadDef(Start, MainThread, osPriorityHigh, 0, MAIN_THREAD_STACK_SIZE);
  MainThreadId = osThreadCreate (osThread(Start), NULL);

  if (MainThreadId == NULL)
  {
     Error_Handler();
  }
#if (STACK_ANALYSIS_TRACE == 1)
  else
  {
     stackAnalysis_addStackSizeByHandle(MainThreadId, MAIN_THREAD_STACK_SIZE);
  }
#endif
  
  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */
  Error_Handler();
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 7;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}


/**
  * @brief Set LED state
  */
void Led_SetState(bool on)
{
  if (on == true)
  {
    BSP_LED_On(LED_GREEN);
  }
  else
  {
    BSP_LED_Off(LED_GREEN);
  }
}


/**
 * @brief Blink LED for 'count' cycles of 'period' period and 'duty' ON duration.
 * duty < 0 tells to start with an OFF state.
 */ 
void Led_Blink(int period, int duty, int count)
{
  if ( (duty > 0) && (period >= duty) )
  {
    /*  Shape:   ____
                  on |_off__ */
    do
    {
      Led_SetState(true);
      HAL_Delay(duty);
      Led_SetState(false);
      HAL_Delay(period - duty);
    } while (count--);
  }
  if ( (duty < 0) && (period >= -duty) )
  {
    /*  Shape:         ____
                __off_| on   */
    do
    {
      Led_SetState(false);
      HAL_Delay(period + duty);
      Led_SetState(true);
      HAL_Delay(-duty);
    } while (count--);
  }
}

/**
  * @brief Update button ISR status
  */
static void Button_ISR(void)
{
  static uint32_t last_isr = 0;
  uint32_t current_isr = HAL_GetTick();
  /* Debounce: consider ISR only if enough time (in ms) has elapsed
   * since last detected ISR */
  if ( (current_isr - last_isr) > 50)
  {
    button_flags++;
  }
  last_isr = HAL_GetTick();
}


/**
  * @brief Waiting for button to be pushed
  */
uint8_t Button_WaitForPush(uint32_t delay)
{
  uint32_t time_out = HAL_GetTick() + delay;

  do
  {
    if (button_flags > 1)
    {
      HAL_Delay(100);
      button_flags = 0;
      return BP_MULTIPLE_PUSH;
    }

    if (button_flags == 1)
    {
      HAL_Delay(100);
      button_flags = 0;
      return BP_SINGLE_PUSH;
    }
  }
  while (HAL_GetTick() < time_out);

  return BP_NOT_PUSHED;
}

/**
  * @brief Waiting for button to be pushed
  */
uint8_t Button_WaitForMultiPush(uint32_t delay)
{
  HAL_Delay(delay);
  if (button_flags > 1)
  {
    button_flags = 0;
    return BP_MULTIPLE_PUSH;
  }

  if (button_flags == 1)
  {
    button_flags = 0;
    return BP_SINGLE_PUSH;
  }
  return BP_NOT_PUSHED;
}


/**
  * @brief Update joystick ISR status
  */
static void JoyDown_ISR(void)
{
  joystick_down_flags++;
}


/**
  * @brief Waiting for joystick to be pushed
  */
uint8_t JoyDown_WaitForPush()
{
  if (joystick_down_flags > 0)
  {
    joystick_down_flags = 0;
    return BP_SINGLE_PUSH;
  }
  return BP_NOT_PUSHED;
}


/**
  * @brief UART console init function
  */
static void Console_UART_Init(void)
{
  console_uart.Instance = USART2;
  console_uart.Init.BaudRate = 115200;
  console_uart.Init.WordLength = UART_WORDLENGTH_8B;
  console_uart.Init.StopBits = UART_STOPBITS_1;
  console_uart.Init.Parity = UART_PARITY_NONE;
  console_uart.Init.Mode = UART_MODE_TX_RX;
  console_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  console_uart.Init.OverSampling = UART_OVERSAMPLING_16;
#ifdef UART_ONE_BIT_SAMPLE_DISABLE
  console_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  console_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif
  BSP_COM_Init(COM1,&console_uart);
}

#if (defined(__GNUC__) && !defined(__CC_ARM))
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* redirect to USART */
  HAL_UART_Transmit(&console_uart, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/**
  * @brief  Retargets the C library scanf function to the USART.
  * @param  None
  * @retval None
  */
GETCHAR_PROTOTYPE
{
  /* Place your implementation of fgetc here */
  /* e.g. read a character on USART and loop until the end of read */
  uint8_t ch = 0;
  while (HAL_OK != HAL_UART_Receive(&console_uart, (uint8_t *)&ch, 1, 30000))
  {
    ;
  }
  return ch;
}

/**
  * @brief RTC init function
  */
static void RTC_Init(void)
{
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  /* 32.768kHz LSE clock input */
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
#ifdef RTC_OUTPUT_REMAP_NONE
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
#endif
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Start Thread
  * @param  argument not used
  * @retval None
  */
static void MainThread(void const * argument)
{
  osThreadId CloudThreadId = NULL;
  UNUSED(argument);
  msg_info("\r\n Starting Main Thread...\n");

  platform_init();

  /* Start application task */
  osThreadDef(CLOUDNAME, &cloud_run, osPriorityNormal, 0, CLOUD_THREAD_STACK_SIZE);
  CloudThreadId = osThreadCreate (osThread(CLOUDNAME), NULL);

  if (CloudThreadId == NULL)
  {
     Error_Handler();
  }
#if (STACK_ANALYSIS_TRACE == 1)
  else
  {
     stackAnalysis_addStackSizeByHandle(CloudThreadId, CLOUD_THREAD_STACK_SIZE);
  }
#endif

  for( ;; )
  {
    /* Delete the start Thread */
    osThreadTerminate(NULL);
  }
}

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (DOWN_JOY_PIN):
    {
      JoyDown_ISR();
      break;
    }
    case (SEL_JOY_PIN):
    {
      Button_ISR();
      break;
    }
    default:
    {
      break;
    }
  }
}


void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MDM_PWR_EN_GPIO_Port, MDM_PWR_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MDM_DTR_GPIO_Port, MDM_DTR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MDM_RST_GPIO_Port, MDM_RST_Pin, GPIO_PIN_SET);


  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_SIM_SELECT_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MDM_SIM_SELECT_1_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MDM_PWR_EN_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_SIM_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MDM_SIM_RST_GPIO_Port, &GPIO_InitStruct);



  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_SIM_SELECT_0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MDM_SIM_SELECT_0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_DTR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MDM_DTR_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_SIM_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MDM_SIM_CLK_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MDM_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = MDM_SIM_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MDM_SIM_DATA_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM3 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  while(1)
  {
    BSP_LED_Toggle(LED_GREEN);
    HAL_Delay(200);
  }
}

#ifdef configUSE_MALLOC_FAILED_HOOK
void vApplicationMallocFailedHook( void );

void vApplicationMallocFailedHook( void )
{
  NET_ASSERT(0,"Failed:Run out of memory");
}
#endif

#ifdef configCHECK_FOR_STACK_OVERFLOW

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
  NET_ASSERT(0,"Failed:Run out of stack ");
}
#endif




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
