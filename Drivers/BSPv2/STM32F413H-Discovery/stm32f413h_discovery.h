/**
  ******************************************************************************
  * @file    stm32f413h_discovery.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for STM32F413H_DISCOVERY's LEDs,
  *          push-buttons and COM ports hardware resources.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F413H_DISCOVERY_H
#define STM32F413H_DISCOVERY_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_conf.h"
#include "stm32f413h_discovery_errno.h"
#if (USE_BSP_COM_FEATURE > 0)
  #if (USE_COM_LOG > 0)
    #ifndef __GNUC__
      #include <stdio.h>
    #endif
  #endif
#endif

/** @addtogroup BSP
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_LOW_LEVEL LOW LEVEL
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_LOW_LEVEL_Exported_Constants LOW LEVEL Exported Constants
  * @{
  */

/**
  * @brief  Define for STM32F413H_DISCOVERY board
  */
/**
  * @brief STM32F413H DISCOVERY BSP Driver version number V2.0.0RC1
  */
#define STM32F413H_DISCOVERY_BSP_VERSION_MAIN   (0x02) /*!< [31:24] main version */
#define STM32F413H_DISCOVERY_BSP_VERSION_SUB1   (0x00) /*!< [23:16] sub1 version */
#define STM32F413H_DISCOVERY_BSP_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version */
#define STM32F413H_DISCOVERY_BSP_VERSION_RC     (0x01) /*!< [7:0]  release candidate */
#define STM32F413H_DISCOVERY_BSP_VERSION        ((STM32F413H_DISCOVERY_BSP_VERSION_MAIN << 24)\
                                                 |(STM32F413H_DISCOVERY_BSP_VERSION_SUB1 << 16)\
                                                 |(STM32F413H_DISCOVERY_BSP_VERSION_SUB2 << 8 )\
                                                 |(STM32F413H_DISCOVERY_BSP_VERSION_RC))

#if !defined (USE_STM32F413H_DISCOVERY)
 #define USE_STM32F413H_DISCOVERY
#endif

#ifndef USE_BSP_COM_FEATURE
   #define USE_BSP_COM_FEATURE                  1U
#endif

#ifndef USE_COM_LOG
  #define USE_COM_LOG                           0U
#endif

#define LED3_GPIO_PORT                   GPIOC
#define LED3_PIN                         GPIO_PIN_5
#define LED3_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOC_CLK_ENABLE()
#define LED3_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOC_CLK_DISABLE()

#define LED4_GPIO_PORT                   GPIOE
#define LED4_PIN                         GPIO_PIN_3
#define LED4_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOE_CLK_ENABLE()
#define LED4_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOE_CLK_DISABLE()

/* Button state */
#define BUTTON_RELEASED                  0U
#define BUTTON_PRESSED                   1U

/* USER push-button */
#define BUTTON_USER_PIN                  GPIO_PIN_0
#define BUTTON_USER_GPIO_PORT            GPIOA
#define BUTTON_USER_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()
#define BUTTON_USER_GPIO_CLK_DISABLE()   __HAL_RCC_GPIOA_CLK_DISABLE()
#define BUTTON_USER_EXTI_IRQn            EXTI0_IRQn
#define BUTTON_USER_EXTI_LINE            EXTI_LINE_0

/* Definition for COM port1, connected to USART6 */
#if (USE_BSP_COM_FEATURE > 0)
#define COM1_UART                        USART6
#define COM1_CLK_ENABLE()                __HAL_RCC_USART6_CLK_ENABLE()
#define COM1_CLK_DISABLE()               __HAL_RCC_USART6_CLK_DISABLE()

#define COM1_TX_PIN                      GPIO_PIN_14
#define COM1_TX_GPIO_PORT                GPIOG
#define COM1_TX_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOG_CLK_ENABLE()
#define COM1_TX_GPIO_CLK_DISABLE()       __HAL_RCC_GPIOG_CLK_DISABLE()
#define COM1_TX_AF                       GPIO_AF8_USART6

#define COM1_RX_PIN                      GPIO_PIN_9
#define COM1_RX_GPIO_PORT                GPIOG
#define COM1_RX_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOG_CLK_ENABLE()
#define COM1_RX_GPIO_CLK_DISABLE()       __HAL_RCC_GPIOG_CLK_DISABLE()
#define COM1_RX_AF                       GPIO_AF8_USART6

#define COM_POLL_TIMEOUT                1000U

#define MX_UART_InitTypeDef              COM_InitTypeDef
#endif /* (USE_BSP_COM_FEATURE > 0) */
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_LOW_LEVEL_Exported_Types LOW LEVEL Exported Types
  * @{
  */
typedef enum
{
  LED3 = 0U,
  LED_GREEN = LED3,
  LED4 = 1U,
  LED_RED = LED4,
  LEDn
}Led_TypeDef;

typedef enum
{
  BUTTON_USER    = 0U,
  BUTTONn
}Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
}ButtonMode_TypeDef;

#if (USE_BSP_COM_FEATURE > 0)
typedef enum
{
  COM1 = 0U,
  COMn
}COM_TypeDef;

typedef enum
{
 COM_WORDLENGTH_8B     =   UART_WORDLENGTH_8B,
 COM_WORDLENGTH_9B     =   UART_WORDLENGTH_9B,
}COM_WordLengthTypeDef;

typedef enum
{
  COM_STOPBITS_1     =   UART_STOPBITS_1,
  COM_STOPBITS_2     =   UART_STOPBITS_2,
}COM_StopBitsTypeDef;

typedef enum
{
  COM_PARITY_NONE     =  UART_PARITY_NONE,
  COM_PARITY_EVEN     =  UART_PARITY_EVEN,
  COM_PARITY_ODD      =  UART_PARITY_ODD,
}COM_ParityTypeDef;

typedef enum
{
  COM_HWCONTROL_NONE    =  UART_HWCONTROL_NONE,
  COM_HWCONTROL_RTS     =  UART_HWCONTROL_RTS,
  COM_HWCONTROL_CTS     =  UART_HWCONTROL_CTS,
  COM_HWCONTROL_RTS_CTS =  UART_HWCONTROL_RTS_CTS,
}COM_HwFlowCtlTypeDef;

typedef struct
{
  uint32_t              BaudRate;
  COM_WordLengthTypeDef WordLength;
  COM_StopBitsTypeDef   StopBits;
  COM_ParityTypeDef     Parity;
  COM_HwFlowCtlTypeDef  HwFlowCtl;
}COM_InitTypeDef;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
typedef struct
{
  pUART_CallbackTypeDef  pMspInitCb;
  pUART_CallbackTypeDef  pMspDeInitCb;
}BSP_COM_Cb_t;
#endif /* (USE_HAL_UART_REGISTER_CALLBACKS == 1) */
#endif /* (USE_BSP_COM_FEATURE > 0) */

/**
  * @}
  */

/** @addtogroup STM32F413H_DISCOVERY_LOW_LEVEL_Exported_Variables
  * @{
  */
extern EXTI_HandleTypeDef hpb_exti[];
#if (USE_BSP_COM_FEATURE > 0)
extern UART_HandleTypeDef hcom_uart[];
#endif /* (USE_BSP_COM_FEATURE > 0) */
/**
  * @}
  */

/** @addtogroup STM32F413H_DISCOVERY_LOW_LEVEL_Exported_Functions
  * @{
  */
int32_t  BSP_GetVersion(void);
int32_t  BSP_LED_Init(Led_TypeDef Led);
int32_t  BSP_LED_DeInit(Led_TypeDef Led);
int32_t  BSP_LED_On(Led_TypeDef Led);
int32_t  BSP_LED_Off(Led_TypeDef Led);
int32_t  BSP_LED_Toggle(Led_TypeDef Led);
int32_t  BSP_LED_GetState (Led_TypeDef Led);
int32_t  BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
int32_t  BSP_PB_DeInit(Button_TypeDef Button);
int32_t  BSP_PB_GetState(Button_TypeDef Button);
void     BSP_PB_Callback(Button_TypeDef Button);
void     BSP_PB_IRQHandler(Button_TypeDef Button);
#if (USE_BSP_COM_FEATURE > 0)
int32_t  BSP_COM_Init(COM_TypeDef COM, COM_InitTypeDef *COM_Init);
int32_t  BSP_COM_DeInit(COM_TypeDef COM);
#if( USE_COM_LOG > 0)
int32_t  BSP_COM_SelectLogPort (COM_TypeDef COM);
#endif
HAL_StatusTypeDef MX_USART6_Init(UART_HandleTypeDef *huart, MX_UART_InitTypeDef *COM_Init) ;
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
int32_t BSP_COM_RegisterDefaultMspCallbacks(COM_TypeDef COM);
int32_t BSP_COM_RegisterMspCallbacks(COM_TypeDef COM, BSP_COM_Cb_t *Callback);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */
#endif /* (USE_BSP_COM_FEATURE > 0) */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32F413H_DISCOVERY_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
