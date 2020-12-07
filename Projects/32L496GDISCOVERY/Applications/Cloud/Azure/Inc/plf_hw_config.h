/**
  ******************************************************************************
  * @file    plf_hw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the hardware configuration of the platform
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_HW_CONFIG_H
#define PLF_HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* MISRAC messages linked to HAL include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
/*cstat +MISRAC2012-* */

#include "main.h"
#ifdef USE_MODEM_UG96
#include "plf_modem_config_ug96.h"
#endif
#ifdef USE_MODEM_BG96
#include "plf_modem_config_bg96.h"
#endif
#include "usart.h" /* for huartX and hlpuartX */

/* Exported constants --------------------------------------------------------*/

/* Platform defines ----------------------------------------------------------*/

/* MODEM configuration */
#if defined(CONFIG_MODEM_USE_STMOD_CONNECTOR)
#define MODEM_UART_HANDLE       huart1
#define MODEM_UART_INSTANCE     ((USART_TypeDef *)USART1)
#define MODEM_UART_AUTOBAUD     (1)
#define MODEM_UART_IRQn         USART1_IRQn
#else
#error Modem connector not specified or invalid for this board
#endif /* defined(CONFIG_MODEM_USE_STMOD_CONNECTOR) */

#define MODEM_UART_BAUDRATE     (CONFIG_MODEM_UART_BAUDRATE)
#if (CONFIG_MODEM_UART_RTS_CTS == 1)
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_RTS_CTS
#else
#define MODEM_UART_HWFLOWCTRL   UART_HWCONTROL_NONE
#endif /* (CONFIG_MODEM_UART_RTS_CTS == 1) */
#define MODEM_UART_WORDLENGTH   UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS     UART_STOPBITS_1
#define MODEM_UART_PARITY       UART_PARITY_NONE
#define MODEM_UART_MODE         UART_MODE_TX_RX

/* ---- MODEM other pins configuration ---- */
#if defined(CONFIG_MODEM_USE_STMOD_CONNECTOR)
#define MODEM_RST_GPIO_Port     ((GPIO_TypeDef *)MDM_RST_GPIO_Port)    /* for DiscoL496: GPIOB      */
#define MODEM_RST_Pin           MDM_RST_Pin                            /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_PWR_EN_GPIO_Port  ((GPIO_TypeDef *)MDM_PWR_EN_GPIO_Port) /* for DiscoL496: GPIOD      */
#define MODEM_PWR_EN_Pin        MDM_PWR_EN_Pin                         /* for DiscoL496: GPIO_PIN_3 */
#define MODEM_DTR_GPIO_Port     ((GPIO_TypeDef *)MDM_DTR_GPIO_Port)    /* for DiscoL496: GPIOA      */
#define MODEM_DTR_Pin           MDM_DTR_Pin                            /* for DiscoL496: GPIO_PIN_0 */
#else
#error Modem connector not specified or invalid for this board
#endif /* defined(CONFIG_MODEM_USE_STMOD_CONNECTOR) */

/* Resource LED definition */
#define NETWORK_LED          LED1
#define HTTPCLIENT_LED       LED2

/* Flash configuration   */
#define FLASH_LAST_PAGE_ADDR     ((uint32_t)0x080ff800) /* Base @ of Page 255, 2 Kbytes */
#define FLASH_LAST_PAGE_NUMBER     255
#define FLASH_BANK_NUMBER          FLASH_BANK_2

/* DEBUG INTERFACE CONFIGURATION */
#define TRACE_INTERFACE_UART_HANDLE     console_uart
#define TRACE_INTERFACE_INSTANCE        ((USART_TypeDef *)USART2)

/* COM INTERFACE CONFIGURATION */
#define COM_INTERFACE_UART_HANDLE     hlpuart1
#define COM_INTERFACE_INSTANCE        ((USART_TypeDef *)LPUART1)
#define COM_INTERFACE_UART_IRQ         LPUART1_IRQn
#define COM_INTERFACE_UART_INIT        MX_LPUART1_UART_Init(); \
  HAL_NVIC_EnableIRQ(COM_INTERFACE_UART_IRQ);

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef       console_uart;
extern UART_HandleTypeDef       huart1;
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_HW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
