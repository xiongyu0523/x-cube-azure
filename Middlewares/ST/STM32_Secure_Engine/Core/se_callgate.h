/**
  ******************************************************************************
  * @file    se_callgate.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for Secure Engine CALLGATE module
  *          functionalities.
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
#ifndef SE_CALLGATE_H
#define SE_CALLGATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "se_def.h"


/** @addtogroup SE Secure Engine
  * @{
  */

/** @addtogroup SE_CORE SE Core
  * @{
  */

/** @addtogroup SE_CALLGATE SE CallGate
  * @{
  */

/** @defgroup SE_CALLGATE_Exported_Constants Exported Constants
  * @{
  */

/**
  * @brief  Secure Engine CallGate Function ID structure definition
  */
typedef enum
{
  /*Generic functions*/
  SE_INIT_ID = 0x00U,                                    /*!< Secure Engine Init     */

  /* CRYPTO Low level functions for bootloader only */
  SE_CRYPTO_LL_ENCRYPT_INIT_ID    = 0x01U,         /*!< CRYPTO Low level Encrypt_Init               */
  SE_CRYPTO_LL_ENCRYPT_APPEND_ID  = 0x02U,         /*!< CRYPTO Low level Encrypt_Append             */
  SE_CRYPTO_LL_ENCRYPT_FINISH_ID  = 0x03U,         /*!< CRYPTO Low level Encrypt_Finish             */
  SE_CRYPTO_LL_DECRYPT_INIT_ID    = 0x04U,         /*!< CRYPTO Low level Decrypt_Init               */
  SE_CRYPTO_LL_DECRYPT_APPEND_ID  = 0x05U,         /*!< CRYPTO Low level Decrypt_Append             */
  SE_CRYPTO_LL_DECRYPT_FINISH_ID  = 0x06U,         /*!< CRYPTO Low level Decrypt_Finish             */
  SE_CRYPTO_LL_AUTHENTICATE_FW_INIT_ID    = 0x07U, /*!< CRYPTO Low level Authenticate_FW_Init       */
  SE_CRYPTO_LL_AUTHENTICATE_FW_APPEND_ID  = 0x08U, /*!< CRYPTO Low level Authenticate_FW_Append     */
  SE_CRYPTO_LL_AUTHENTICATE_FW_FINISH_ID  = 0x09U, /*!< CRYPTO Low level Authenticate_FW_Finish     */

  /* CRYPTO High level functions for bootloader only */
  SE_CRYPTO_HL_AUTHENTICATE_METADATA = 0x10U,       /*!< CRYPTO High level Authenticate Metadata */

  /* Next ranges are kept for future use (additional crypto schemes, additional user code) */
  SE_APP_GET_ACTIVE_FW_INFO = 0x20U,                     /*!< User Application retrieves the Active Firmware Info */

  /* BootInfo access functions (bootloader only) */
  SE_BOOT_INFO_READ_ALL_ID        = 0x80U,               /*!< SE_INFO_ReadBootInfo (bootloader only)     */
  SE_BOOT_INFO_WRITE_ALL_ID       = 0x81U,               /*!< SE_INFO_WriteBootInfo (bootloader only)    */

  /* SE IMG interface (bootloader only) */
  SE_IMG_READ = 0x92U,                                   /*!< SFU reads a Flash protected area (bootloader only) */
  SE_IMG_WRITE   = 0x93U,                                /*!< SFU write a Flash protected area (bootloader only) */

  /* LOCK service to be used by the bootloader only */
  SE_LOCK_RESTRICT_SERVICES   = 0x100U,                   /*!< SFU lock part of SE services (bootloader only) */

} SE_FunctionIDTypeDef;

/**
  * @}
  */

/** @addtogroup SE_CALLGATE_Exported_Functions
  * @{
  */
SE_ErrorStatus SE_CallGate(SE_FunctionIDTypeDef eID, SE_StatusTypeDef *peSE_Status, ...);

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

#endif /* SE_CALLGATE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


