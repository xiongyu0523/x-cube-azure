/**
  ******************************************************************************
  * @file    se_user_application.c
  * @author  MCD Application Team
  * @brief   Secure Engine USER APPLICATION module.
  *          This file is a placeholder for the code dedicated to the user application.
  *          These services are used by the application.
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

/* Includes ------------------------------------------------------------------*/
#include "se_user_application.h"
#include "se_low_level.h"
#include "string.h"

/** @addtogroup SE Secure Engine
  * @{
  */

/** @addtogroup SE_CORE SE Core
  * @{
  */

/** @defgroup  SE_APPLI SE Code for Application
  * @brief This file is a placeholder for the code dedicated to the user application.
  * It contains the code written by the end user.
  * The code used by the application to handle the Firmware images is located in se_fwimg.c.
  * @{
  */

/** @defgroup SE_APPLI_Private_Variables Private Variables
  * @brief Use this section to declare User Application data to be protected.
  * @note For instance a user application key stored in protected FLASH could be stored here like this:
  *
  * #if defined(__ICCARM__)
  * #pragma default_variable_attributes  = @ ".SE_Key_Const"
  * #else
  * __attribute__((section(".SE_Key_Const")))
  * #endif
  * static const  uint8_t m_aSE_UserAppKey[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  * #if defined(__ICCARM__)
  * #pragma default_variable_attributes  =
  * #endif
  * @{
  */

/**
  * @}
  */

/** @defgroup SE_APPLI_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief Service called by the User Application to retrieve the Active Firmware Info.
  * @param p_FwInfo Active Firmware Info structure that will be filled.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_APPLI_GetActiveFwInfo(SE_APP_ActiveFwInfo *p_FwInfo)
{
  SE_ErrorStatus e_ret_status;
  uint8_t buffer[SE_FW_HEADER_TOT_LEN];  /* to read FW metadata from FLASH */
  SE_FwRawHeaderTypeDef fw_image_header; /* FW metadata */

  /* Check the pointer allocation */
  if (NULL == p_FwInfo)
  {
    return SE_ERROR;
  }

  /*
   * The Firmware Information is available in the header of the slot #0.
   */
  e_ret_status = SE_LL_FLASH_Read(buffer, SFU_IMG_SLOT_0_REGION_BEGIN, sizeof(buffer));
  if (e_ret_status != SE_ERROR)
  {
    /*
     * The endianess is the same for storing and reading the data so memcpy is acceptable here.
     * Of course, this code is wrong if the endianess is not the same for reading and writing.
     */
    (void) memcpy(&fw_image_header, buffer, sizeof(SE_FwRawHeaderTypeDef));

    /*
     * We do not check the header validity.
     * We just copy the information.
     */
    p_FwInfo->ActiveFwVersion = fw_image_header.FwVersion;
    p_FwInfo->ActiveFwSize = fw_image_header.FwSize;
  }

  return e_ret_status;
}


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


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
