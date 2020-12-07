/**
  ******************************************************************************
  * @file    stm32f413h_discovery_psram.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32f413h_discovery_psram.c driver.
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
#ifndef STM32F413H_DISCOVERY_PSRAM_H
#define STM32F413H_DISCOVERY_PSRAM_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_conf.h"
#include "stm32f413h_discovery_errno.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY_PSRAM
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Exported_Types PSRAM Exported Types
  * @{
  */
#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
typedef struct
{
  pSRAM_CallbackTypeDef  pMspInitCb;
  pSRAM_CallbackTypeDef  pMspDeInitCb;
}BSP_PSRAM_Cb_t;
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 1) */
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Exported_Constants PSRAM Exported Constants
  * @{
  */

#define PSRAM_INSTANCES_NBR   1U

/**
  * @brief  PSRAM status structure definition
  */
#define PSRAM_DEVICE_ADDR  0x60000000U
#define PSRAM_DEVICE_SIZE  0x80000U  /* 512 KBytes */

/* DMA definitions for SRAM DMA transfer */
#define PSRAM_DMAx_CLK_ENABLE              __HAL_RCC_DMA2_CLK_ENABLE
#define PSRAM_DMAx_CLK_DISABLE             __HAL_RCC_DMA2_CLK_DISABLE
#define PSRAM_DMAx_CHANNEL                 DMA_CHANNEL_0
#define PSRAM_DMAx_STREAM                  DMA2_Stream5
#define PSRAM_DMAx_IRQn                    DMA2_Stream5_IRQn
#define PSRAM_DMA_IRQHandler               DMA2_Stream5_IRQHandler
/**
  * @}
  */

/** @addtogroup STM32F413H_DISCOVERY_PSRAM_Exported_Variables
  * @{
  */
extern SRAM_HandleTypeDef hpsram[];
/**
  * @}
  */

/** @addtogroup STM32F413H_DISCOVERY_PSRAM_Exported_Functions
  * @{
  */
int32_t BSP_PSRAM_Init(uint32_t Instance);
int32_t BSP_PSRAM_DeInit(uint32_t Instance);
#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
int32_t BSP_PSRAM_RegisterDefaultMspCallbacks (uint32_t Instance);
int32_t BSP_PSRAM_RegisterMspCallbacks (uint32_t Instance, BSP_PSRAM_Cb_t *CallBacks);
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)  */
void BSP_PSRAM_DMA_IRQHandler(uint32_t Instance);

/* These functions can be modified in case the current settings
   need to be changed for specific application needs */
HAL_StatusTypeDef MX_SRAM_BANK1_Init(SRAM_HandleTypeDef *hsram);

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

#endif /* STM32F413H_DISCOVERY_SRAM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
