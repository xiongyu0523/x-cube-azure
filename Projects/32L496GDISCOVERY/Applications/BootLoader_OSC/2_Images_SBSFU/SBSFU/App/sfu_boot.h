/**
  ******************************************************************************
  * @file    sfu_boot.h
  * @author  MCD Application Team
  * @brief   Header the Secure Boot module, part of the SFU-En project (SB/SFU).
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
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
#ifndef SFU_BOOT_H
#define SFU_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_BOOT SB Secure Boot
  * @{
  */

/** @defgroup SFU_BOOT_Exported_Types Exported Type(s).
  * @brief Secure bootloader Exported Type(s).
  * @{
  */

/**
  * @brief  SFU_BOOT Init Error Type Definition
  */
typedef enum
{
  SFU_BOOT_SECENG_INIT_FAIL,   /*!< failure at secure engine initialization stage */
  SFU_BOOT_SECIPS_CFG_FAIL,    /*!< failure when configuring the security IPs  */
  SFU_BOOT_INIT_FAIL,          /*!< failure when initializing the secure boot service  */
  SFU_BOOT_INIT_ERROR          /*!< Service cannot start: unspecified error */
} SFU_BOOT_InitErrorTypeDef;


/**
  * @}
  */

/** @addtogroup SFU_BOOT_EntryPoint_Function
  * @{
  */
SFU_BOOT_InitErrorTypeDef SFU_BOOT_RunSecureBootService(void);
/**
  * @}
  */

/** @addtogroup SFU_BOOT_ShutdownOnError_Function
  * @{
  */
void                    SFU_BOOT_ForceReboot(void);

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

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* SFU_BOOT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

