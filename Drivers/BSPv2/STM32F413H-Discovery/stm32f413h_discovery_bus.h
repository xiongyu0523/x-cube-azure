/**
  ******************************************************************************
  * @file    stm32f413h_discovery_bus.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32f413h_discovery_bus.c driver.
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
#ifndef STM32F413H_DISCOVERY_BUS_H
#define STM32F413H_DISCOVERY_BUS_H

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

/** @defgroup STM32F413H_DISCOVERY_BUS BUS
  * @{
  */
/** @defgroup STM32F413H_DISCOVERY_BUS_Exported_Types Bus Exported Types
  * @{
  */
#if (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  pFMPI2C_CallbackTypeDef  pMspI2cInitCb;
  pFMPI2C_CallbackTypeDef  pMspI2cDeInitCb;
}BSP_FMPI2C_Cb_t;
#endif /* (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 1) */
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Exported_Constants Bus Exported Constants
  * @{
  */

/* User can use this section to tailor I2C instance used and associated resources */
#define BUS_FMPI2C1                             FMPI2C1
#define BUS_FMPI2C1_CLK_ENABLE()                __HAL_RCC_FMPI2C1_CLK_ENABLE()
#define BUS_FMPI2C1_CLK_DISABLE()               __HAL_RCC_FMPI2C1_CLK_DISABLE()
#define BUS_FMPI2C1_SCL_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOC_CLK_ENABLE()
#define BUS_FMPI2C1_SDA_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOC_CLK_ENABLE()

#define BUS_FMPI2C1_FORCE_RESET()               __HAL_RCC_FMPI2C1_FORCE_RESET()
#define BUS_FMPI2C1_RELEASE_RESET()             __HAL_RCC_FMPI2C1_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define BUS_FMPI2C1_SCL_PIN                     GPIO_PIN_6
#define BUS_FMPI2C1_SCL_GPIO_PORT               GPIOC
#define BUS_FMPI2C1_SCL_AF                      GPIO_AF4_FMPI2C1
#define BUS_FMPI2C1_SDA_PIN                     GPIO_PIN_7
#define BUS_FMPI2C1_SDA_GPIO_PORT               GPIOC
#define BUS_FMPI2C1_SDA_AF                      GPIO_AF4_FMPI2C1

#ifndef BUS_FMPI2C1_FREQUENCY
   #define BUS_FMPI2C1_FREQUENCY                100000U /* Frequency of I2Cn = 100 KHz*/
#endif

#ifndef BUS_FMPI2C1_POLL_TIMEOUT
   #define BUS_FMPI2C1_POLL_TIMEOUT             0x1000U
#endif

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Exported_Functions Bus Exported Functions
  * @{
  */
int32_t BSP_FMPI2C1_Init(void);
int32_t BSP_FMPI2C1_DeInit(void);
int32_t BSP_FMPI2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_FMPI2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_FMPI2C1_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_FMPI2C1_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_FMPI2C1_IsReady(uint16_t DevAddr, uint32_t Trials);
int32_t BSP_GetTick(void);

HAL_StatusTypeDef MX_FMPI2C1_Init(FMPI2C_HandleTypeDef *phfmpi2c, uint32_t timing);
#if (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 1)
int32_t BSP_FMPI2C1_RegisterDefaultMspCallbacks(void);
int32_t BSP_FMPI2C1_RegisterMspCallbacks(BSP_FMPI2C_Cb_t *Callback);
#endif
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

#endif /* STM32F413H_DISCOVERY_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
