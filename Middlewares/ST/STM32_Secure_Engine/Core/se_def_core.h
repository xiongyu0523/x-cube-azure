/**
  ******************************************************************************
  * @file    se_def_core.h
  * @author  MCD Application Team
  * @brief   This file contains core definitions for SE functionalities.
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
#ifndef SE_DEF_CORE_H
#define SE_DEF_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/** @addtogroup  SE Secure Engine
  * @{
  */

/** @addtogroup  SE_CORE SE Core
  * @{
  */

/** @addtogroup  SE_CORE_DEF SE Definitions
  * @{
  */

/** @addtogroup SE_DEF_CORE SE Core Definitions
  * @brief core definitions (constants, error codes, image handling) which are NOT crypto dependent
  * @{
  */

/** @defgroup SE_DEF_CORE_Exported_Constants Exported Constants
  * @{
  */

/**
  * @brief  Secure Engine Error definition
  */
typedef enum
{
  SE_ERROR = 0U,
  SE_SUCCESS = !SE_ERROR
} SE_ErrorStatus;

/**
  *  @brief  Secure Engine Status definition
  */
typedef enum
{
  SE_OK = 0U,                                        /*!< Secure Engine OK */
  SE_KO,                                             /*!< Secure Engine KO */
  SE_INIT_ERR,                                       /*!< Secure Engine initialization error */
  SE_BOOT_INFO_ERR,                                  /*!< An error occurred when accessing BootInfo area */
  SE_BOOT_INFO_ERR_FACTORY_RESET,                    /*!< A factory reset has been executed to recover the BootInfo Initialization failure */
  SE_SIGNATURE_ERR,                                  /*!< An error occurred when checking FW signature (Tag) */
  SE_ERR_FLASH_READ                                  /*!< An error occurred trying to read the Flash */
} SE_StatusTypeDef;

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

#endif /* SE_DEF_CORE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

