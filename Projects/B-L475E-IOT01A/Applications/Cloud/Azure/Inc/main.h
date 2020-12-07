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
#include "stm32l475e_iot01.h"
#include "stm32l4xx_hal_iwdg.h"
#include "azure_version.h"
#ifdef SENSOR
#include "stm32l475e_iot01_motion_sensors.h"
#include "stm32l475e_iot01_env_sensors.h"
#include "vl53l0x_proximity.h"
#endif
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


/* Imported functions ------------------------------------------------------- */
void cloud_run(void const *arg);

/* Exported functions --------------------------------------------------------*/
void    Error_Handler(void);
uint8_t Button_WaitForPush(uint32_t timeout);
uint8_t Button_WaitForMultiPush(uint32_t timeout);
void    Led_SetState(bool on);
void    Led_Blink(int period, int duty, int count);
void    Periph_Config(void);
void SPI3_IRQHandler(void);
extern  SPI_HandleTypeDef hspi;
extern RNG_HandleTypeDef hrng;
extern RTC_HandleTypeDef hrtc;

extern const user_config_t *lUserConfigPtr;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
