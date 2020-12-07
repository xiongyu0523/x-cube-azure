/**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @brief   main application header file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MAIN_H
#define MAIN_H
#define __main_h__
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l496g_discovery.h"
#include "stm32l4xx_hal_iwdg.h"
#include "azure_version.h"
#include "stm32l4xx_ll_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "timedate.h"
#include "flash.h"
#include "iot_flash_config.h"
#include "msg.h"
#include "net_connect.h"
#include "cloud.h"

#include "cmsis_os.h"
uint32_t sys_now(void);


#if  defined(BOOTLOADER)
#if defined(__CC_ARM)
#include "MDK-ARM/mapping_fwimg.h"
#elif defined (__ICCARM__)
#include "EWARM/mapping_export.h"
#else
#include "SW4STM32/mapping_export.h"
#endif

void MPU_EnterUnprivilegedMode(void);
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#if defined(USE_WIFI)
#define NET_IF  NET_IF_WLAN
#elif defined(USE_LWIP)
#define NET_IF  NET_IF_ETH
#elif defined(USE_C2C)
#define NET_IF  NET_IF_C2C
#endif

#if defined(BOOTLOADER)
#define GENERIC_OTA
#define CLD_OTA
#endif

/*
 * Device Provisioning Service support.
 * The device registers through a DPS server first and
 * is automatically redirected to the appropriate IoT Hub.
 */
#define AZURE_DPS_PROV
/*
 * Enabling the support of the "proof of possession" computation
 * to activate the X.509 HSM Root Certificate uploaded in the DPS portal.
 * This is useful for group enrollment.
 */
#define AZURE_DPS_PROOF_OF_POSS


enum {BP_NOT_PUSHED=0, BP_SINGLE_PUSH, BP_MULTIPLE_PUSH};

#define MDM_SIM_SELECT_0_Pin GPIO_PIN_2
#define MDM_SIM_SELECT_0_GPIO_Port GPIOC

#define MDM_SIM_SELECT_1_Pin GPIO_PIN_3
#define MDM_SIM_SELECT_1_GPIO_Port GPIOI

#define MDM_SIM_CLK_Pin GPIO_PIN_4
#define MDM_SIM_CLK_GPIO_Port GPIOA

#define MDM_SIM_DATA_Pin GPIO_PIN_12
#define MDM_SIM_DATA_GPIO_Port GPIOB

#define MDM_SIM_RST_Pin GPIO_PIN_7
#define MDM_SIM_RST_GPIO_Port GPIOC

#define MDM_PWR_EN_Pin GPIO_PIN_3
#define MDM_PWR_EN_GPIO_Port GPIOD

#define MDM_DTR_Pin GPIO_PIN_0
#define MDM_DTR_GPIO_Port GPIOA

#define MDM_RST_Pin GPIO_PIN_2
#define MDM_RST_GPIO_Port GPIOB

#define USART1_TX_Pin GPIO_PIN_6
#define USART1_TX_GPIO_Port GPIOB

#define UART1_RX_Pin GPIO_PIN_10
#define UART1_RX_GPIO_Port GPIOG

#define UART1_CTS_Pin GPIO_PIN_11
#define UART1_CTS_GPIO_Port GPIOG

#define UART1_RTS_Pin GPIO_PIN_12
#define UART1_RTS_GPIO_Port GPIOG


/* Imported functions ------------------------------------------------------- */
void cloud_run(void const *arg);

/* Exported functions --------------------------------------------------------*/
void    Error_Handler(void);
uint8_t Button_WaitForPush(uint32_t timeout);
uint8_t Button_WaitForMultiPush(uint32_t timeout);
uint8_t JoyDown_WaitForPush(void);
void    Led_SetState(bool on);
void    Led_Blink(int period, int duty, int count);
void    Periph_Config(void);
extern RNG_HandleTypeDef hrng;
extern RTC_HandleTypeDef hrtc;

extern const user_config_t *lUserConfigPtr;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
