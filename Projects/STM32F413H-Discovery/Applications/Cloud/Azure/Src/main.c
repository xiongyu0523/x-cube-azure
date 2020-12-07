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


/* Global variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;
RNG_HandleTypeDef hrng;

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef hcom_uart[COMn];
static volatile uint8_t button_flags = 0;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
void SPI_WIFI_ISR(void);

static void Console_UART_Init(void);
static void RTC_Init(void);
static void Button_ISR(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval Status
  */
int main(void)
{
  unsigned int random_number = 0;
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();

  Periph_Config();
  
  BSP_LED_Init(LED_GREEN);
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
  
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
  platform_init();
  cloud_run(0);
  platform_deinit();
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 100000000
  *            HCLK(Hz)                       = 100000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 200
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 3
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  if(ret != HAL_OK)
  {
    while(1) { ; } 
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
  if(ret != HAL_OK)
  {
    while(1) { ; }  
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
  button_flags++;
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
  * @brief UART console init function
  */
static void Console_UART_Init(void)
{
  COM_InitTypeDef com_init;
  
  com_init.BaudRate = 115200;
  com_init.WordLength = COM_WORDLENGTH_8B;
  com_init.StopBits = COM_STOPBITS_1;
  com_init.Parity = COM_PARITY_NONE;
  com_init.HwFlowCtl = COM_HWCONTROL_NONE;
  BSP_COM_Init(COM1, &com_init);
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
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART2 and Loop until the end of transmission */
  while (HAL_OK != HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *) &ch, 1, 30000))
  {
    ;
  }
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
  while (HAL_OK != HAL_UART_Receive(&hcom_uart[COM1], (uint8_t *)&ch, 1, 30000))
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
  /* 32kHz LSI clock input */
  hrtc.Init.AsynchPrediv = 99;
  hrtc.Init.SynchPrediv = 319;
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
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (BUTTON_USER_PIN):
    {
      Button_ISR();
      break;
    }
    case (GPIO_PIN_12):
    {
      SPI_WIFI_ISR();
      break;
    }
    default:
    {
      break;
    }
  }
}


void SPI3_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi);
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


#if defined(BOOTLOADER)

/**
 * @brief This is a helper function to enter the unprivileged level for software execution
 * @param void
 * @retval void
 */
void MPU_EnterUnprivilegedMode(void)
{
  __set_PSP(__get_MSP()); /* set up Process Stack Pointer to current stack pointer */
  __set_MSP(SE_REGION_SRAM1_STACK_TOP); /* change main stack to point on privileged stack */
  __set_CONTROL(__get_CONTROL() | 3); /* bit 0 = 1: unpriviledged      bit 1=1: stack=PSP */
  __ISB();
}

#endif

uint32_t sys_now(void)
{
  return HAL_GetTick();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
