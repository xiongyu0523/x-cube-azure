/**
  ******************************************************************************
  * @file    stm32f413h_discovery_config.h
  * @author  MCD Application Team
  * @brief   STM32F413H Discovery board configuration file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef STM32F413H_DISCO_CONFIG_H
#define STM32F413H_DISCO_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* COM define */
#define USE_COM_LOG                         0U


/* LCD controllers defines */
#define USE_LCD_CTRL_ST7789H2               1U


/* Audio codecs defines */
#define USE_AUDIO_CODEC_WM8994              1U

/* TS supported features defines */
#define USE_TS_GESTURE                      0U
#define USE_TS_MULTI_TOUCH                  1U

/* IRQ priorities */
#define BSP_PSRAM_IT_PRIORITY               15U
#define BSP_BUTTON_USER_IT_PRIORITY         15U
#define BSP_AUDIO_OUT_IT_PRIORITY           14U
#define BSP_AUDIO_IN_IT_PRIORITY            15U
#define BSP_SD_IT_PRIORITY                  14U
#define BSP_SD_RX_IT_PRIORITY               14U
#define BSP_SD_TX_IT_PRIORITY               15U
#define BSP_TS_IT_PRIORITY                  15U

/* Default Audio IN internal buffer size */
#define DEFAULT_AUDIO_IN_BUFFER_SIZE        2048U

/* Default TS touch number */
#define TS_TOUCH_NBR                        2U

/* Default EEPROM max trials */
#define EEPROM_MAX_TRIALS                   3000U

#ifdef __cplusplus
}
#endif

#endif /* STM32F413H_DISCO_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
