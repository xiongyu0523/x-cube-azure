/**
  ******************************************************************************
  * @file    se_callgate.c
  * @author  MCD Application Team
  * @brief   Secure Engine CALLGATE module.
  *          This file provides set of firmware functions to manage SE Callgate
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
/* Place code in a specific section*/

/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>
#include "se_crypto_bootloader.h" /* crypto services dedicated to bootloader */
#include "se_crypto_common.h"     /* common crypto services: can be used by bootloader services and user application services */
#include "se_user_application.h"  /* user application services called by the User Application */
#include "se_callgate.h"
#include "se_key.h"
#include "se_bootinfo.h"
#include "se_utils.h"
#include "se_fwimg.h"
#include "se_low_level.h"
#include "se_startup.h"
#include "string.h"
#include "se_intrinsics.h"
#if defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#elif defined(__CC_ARM)
#include "mapping_sbsfu.h"
#endif
/** @addtogroup SE Secure Engine
  * @{
  */

/** @addtogroup SE_CORE SE Core
  * @{
  */

/** @defgroup  SE_CALLGATE SE CallGate
  * @brief Implementation of the call gate API.
  *        The call gate allows the execution of code inside the protected area.
  * @{
  */

/** @defgroup SE_CALLGATE_Private_Constants Private Constants
  * @{
  */

/**
  * @brief Secure Engine Lock Status enum definition
  */
typedef enum
{
  SE_UNLOCKED = 0U,
  SE_LOCKED = !SE_UNLOCKED
} SE_LockStatus;


/** @defgroup SE_CALLGATE_Private_Macros Private Macros
  * @{
  */

/**
  * @brief Check validity of callgate function IDS
  */
#define IS_SE_FUNCTIONID(ID) (((ID) == SE_INIT_ID) || \
                              ((ID) == SE_CRYPTO_LL_ENCRYPT_INIT_ID) || \
                              ((ID) == SE_CRYPTO_LL_ENCRYPT_APPEND_ID) || \
                              ((ID) == SE_CRYPTO_LL_ENCRYPT_FINISH_ID) || \
                              ((ID) == SE_CRYPTO_HL_AUTHENTICATE_METADATA) || \
                              ((ID) == SE_CRYPTO_LL_DECRYPT_INIT_ID) || \
                              ((ID) == SE_CRYPTO_LL_DECRYPT_APPEND_ID) || \
                              ((ID) == SE_CRYPTO_LL_DECRYPT_FINISH_ID) || \
                              ((ID) == SE_BOOT_INFO_READ_ALL_ID) || \
                              ((ID) == SE_BOOT_INFO_WRITE_ALL_ID) || \
                              ((ID) == SE_IMG_READ) || \
                              ((ID) == SE_IMG_WRITE) || \
                              ((ID) == SE_LOCK_RESTRICT_SERVICES) || \
                              ((ID) == SE_APP_GET_ACTIVE_FW_INFO))


/**
  * @brief Check if the caller is located in SE Interface region
  */
#define __IS_CALLER_SE_IF() \
do{ \
  if ((__get_LR())< SE_IF_REGION_ROM_START){\
    return SE_ERROR;}\
  if ((__get_LR())> SE_IF_REGION_ROM_END){\
    return SE_ERROR;}\
}while(0)

/**
  * @brief If lock restriction service enabled, execution is forbidden
  */

#define __IS_SE_LOCKED_SERVICES()   if (SE_LockRestrictedServices == SE_LOCKED){\
    e_ret_status = SE_ERROR;\
    break;}

/**
  * @}
  */
SE_ErrorStatus SE_CallGateService(SE_FunctionIDTypeDef eID, SE_StatusTypeDef *peSE_Status, va_list arguments);
SE_ErrorStatus SE_SP_SMUGGLE(SE_FunctionIDTypeDef eID, SE_StatusTypeDef *peSE_Status, va_list arguments);

/* Place code in a specific section*/
#if defined(__ICCARM__)
#pragma default_function_attributes = @ ".SE_CallGate_Code"
#elif defined(__CC_ARM)
#pragma arm section code = ".SE_CallGate_Code"
#else
__attribute__((section(".SE_CallGate_Code")))
#endif

/** @defgroup SE_CALLGATE_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief Secure Engine CallGate function.
  *        It is the only access/exit point to code inside protected area.
  *        In order to call others functions included in the protected area, the specific ID
  *        has to be specified.
  * @note  It is a variable argument function. The correct number and order of parameters
  *        has to be used in order to call internal function in the right way.
  * @note DO NOT MODIFY THIS FUNCTION.
  *       New services can be implemented in @ref SE_CallGateService.
  * @param eID: Secure Engine protected function ID.
  * @param peSE_Status: Secure Engine Status.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CallGate(SE_FunctionIDTypeDef eID, SE_StatusTypeDef *peSE_Status, ...)
{
#ifndef SFU_ISOLATE_SE_WITH_MPU
  /* When Firewall isolation is used, SE_CoreBin can have its own vectors. When MPU isolation is used this is not possible. */
  uint32_t *p_appli_vector_addr = (uint32_t *)(SE_REGION_SRAM1_STACK_TOP - 4U);               /* Variable used to store Appli vector position */
#endif
  SE_ErrorStatus e_ret_status;
  va_list arguments;
  /* Check the Callgate was called only from SE Interface */
  __IS_CALLER_SE_IF();
  /* Check the pointers allocation */
  if (SE_LL_Buffer_in_ram(peSE_Status, sizeof(*peSE_Status))== SE_ERROR)
  {
    return SE_ERROR;
  }
  if (SE_BufferCheck_in_se_ram(peSE_Status, sizeof(*peSE_Status)) == SE_SUCCESS)
  {
    return SE_ERROR;
  }

#ifndef SFU_ISOLATE_SE_WITH_MPU
  /*  retriev Appli Vector Value   */
  *p_appli_vector_addr = SCB->VTOR;
  /*  set SE vector */
  SCB->VTOR = (uint32_t)&__vector_table;
#endif
  /* Enter the protected area */
  ENTER_PROTECTED_AREA();

  /*Check parameters*/
  assert_param(IS_SE_FUNCTIONID(eID));

  *peSE_Status =  SE_OK;

  /*Initializing arguments to store all values after peSE_Status*/
  va_start(arguments, peSE_Status);
  /*  call service implementation , this is split to have a fixed size */
#ifdef SFU_ISOLATE_SE_WITH_MPU
  /*  no need to use a specific Stack */
  e_ret_status =  SE_CallGateService(eID, peSE_Status, arguments);
#else
  /*  set SE specific stack before executing SE service */
  e_ret_status =  SE_SP_SMUGGLE(eID, peSE_Status, arguments);
#endif
  /*Clean up arguments list*/
  va_end(arguments);

  /*  restore Appli Vector Value   */
#ifndef SFU_ISOLATE_SE_WITH_MPU
  SCB->VTOR = *p_appli_vector_addr;
#endif
  /* Exit the protected area */
  EXIT_PROTECTED_AREA();

  return e_ret_status;
}
/* Stop placing data in specified section*/
#if defined(__ICCARM__)
#pragma default_function_attributes =
#elif defined(__CC_ARM)
#pragma arm section code
#endif
/**
  * @brief Dispatch function used by the Secure Engine CallGate function.
  *        Calls other functions included in the protected area based on the eID parameter.
  * @note  It is a variable argument function. The correct number and order of parameters
  *        has to be used in order to call internal function in the right way.
  * @param eID: Secure Engine protected function ID.
  * @param peSE_Status: Secure Engine Status.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CallGateService(SE_FunctionIDTypeDef eID, SE_StatusTypeDef *peSE_Status, va_list arguments)
{

  /*
   * For the time being we consider that the user keys can be handled in SE_CallGate.
   * Nevertheless, if this becomes too crypto specific then it will have to be moved to the user application code.
   */
  static SE_LockStatus SE_LockRestrictedServices = SE_UNLOCKED;

  SE_ErrorStatus e_ret_status;

  switch (eID)
  {
    /* ============================ */
    /* ===== BOOTLOADER PART  ===== */
    /* ============================ */
    case SE_INIT_ID:
    {
      uint32_t se_system_core_clock;
      SE_INFO_StatusTypedef e_boot_info_status;

      /* Check that the Secure Engine services are not locked */
      __IS_SE_LOCKED_SERVICES();

      se_system_core_clock = va_arg(arguments, uint32_t);

      *peSE_Status = SE_INIT_ERR;

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /*Initialization of SystemCoreClock variable in Secure Engine binary*/
      SE_SetSystemCoreClock(se_system_core_clock);

      /*Initialization of Shared Info. If BootInfoArea hw/Fw initialization not possible,
      the only option is to try a Factory Reset */
      e_ret_status = SE_INFO_BootInfoAreaInit(&e_boot_info_status);
      if (e_ret_status != SE_SUCCESS)
      {
        e_ret_status = SE_INFO_BootInfoAreaFactoryReset();
        if (e_ret_status == SE_SUCCESS)
        {
          *peSE_Status = SE_BOOT_INFO_ERR_FACTORY_RESET;
        }
        else
        {
          *peSE_Status = SE_BOOT_INFO_ERR;
        }
      }
      else
      {
        *peSE_Status = SE_OK;
      }

      /* NOTE : Other initialization may be added here. */
      break;
    }

    case SE_CRYPTO_LL_ENCRYPT_INIT_ID:
    {
      SE_FwRawHeaderTypeDef *p_x_se_Metadata;

      p_x_se_Metadata = va_arg(arguments, SE_FwRawHeaderTypeDef *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check the Init structure allocation */
      if (SE_BufferCheck_SBSFU(p_x_se_Metadata, sizeof(*p_x_se_Metadata))== SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /*
         * The se_callgate code must be crypto-agnostic.
         * But, when it comes to encrypt, we can use:
         * - either a real encrypt operation (AES GCM)
         * - or no encrypt (FW authentication based on header authentication and clear FW SHA256)
         * As a consequence, retrieving the key cannot be done at the call gate stage.
         * This is implemented in the SE_CRYPTO_Encrypt_Init code.
         */

        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_Encrypt_Init(p_x_se_Metadata);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }


      break;
    }

    case SE_CRYPTO_LL_ENCRYPT_APPEND_ID:
    {

      const uint8_t *input_buffer;
      int32_t      input_size;
      uint8_t       *output_buffer;
      int32_t      *output_size;

      input_buffer = va_arg(arguments, const uint8_t *);
      input_size = va_arg(arguments, int32_t);
      output_buffer = va_arg(arguments, uint8_t *);
      output_size = va_arg(arguments, int32_t *);
      /* Check the pointers allocation */
      if (input_size <=0)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(input_buffer, (uint32_t)input_size) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }        
      if (SE_BufferCheck_SBSFU(output_size, sizeof(*output_size)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_buffer, (uint32_t)input_size) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_Encrypt_Append(input_buffer, input_size, output_buffer, output_size);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }


      break;
    }

    case SE_CRYPTO_LL_ENCRYPT_FINISH_ID:
    {

      uint8_t       *output_buffer;
      int32_t       *output_size;

      output_buffer = va_arg(arguments, uint8_t *);
      output_size = va_arg(arguments, int32_t *);

      /* Check the pointers allocation */
      if (SE_BufferCheck_SBSFU(output_size, sizeof(*output_size)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /*  in AES-GCM 16 bytes can be written  */
      if (SE_BufferCheck_SBSFU(output_buffer, (uint32_t)16U) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_Encrypt_Finish(output_buffer, output_size);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }

      break;
    }

    case SE_CRYPTO_LL_DECRYPT_INIT_ID:
    {
      SE_FwRawHeaderTypeDef *p_x_se_Metadata;

      p_x_se_Metadata = va_arg(arguments, SE_FwRawHeaderTypeDef *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check the Init structure allocation */
      if (SE_BufferCheck_SBSFU(p_x_se_Metadata, sizeof(*p_x_se_Metadata))== SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /*
         * The se_callgate code must be crypto-agnostic.
         * But, when it comes to decrypt, we can use:
         * - either a real decrypt operation (AES GCM or AES CBC FW encryption)
         * - or no decrypt (clear FW)
         * As a consequence, retrieving the key cannot be done at the call gate stage.
         * This is implemented in the SE_CRYPTO_Decrypt_Init code.
         */

        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_Decrypt_Init(p_x_se_Metadata);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_CRYPTO_LL_DECRYPT_APPEND_ID:
    {
      const uint8_t *input_buffer;
      int32_t      input_size;
      uint8_t      *output_buffer;
      int32_t      *output_size;

      input_buffer = va_arg(arguments, const uint8_t *);
      input_size = va_arg(arguments, int32_t);
      output_buffer = va_arg(arguments, uint8_t *);
      output_size = va_arg(arguments, int32_t *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check the pointers allocation */
      if (input_size <= 0)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(input_buffer, (uint32_t)input_size) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_size, sizeof(*output_size)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_buffer, (uint32_t)input_size) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_Decrypt_Append(input_buffer, input_size, output_buffer, output_size);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_CRYPTO_LL_DECRYPT_FINISH_ID:
    {
      uint8_t      *output_buffer;
      int32_t      *output_size;

      output_buffer = va_arg(arguments, uint8_t *);
      output_size = va_arg(arguments, int32_t *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_size, sizeof(*output_size)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      /*  in AES-GCM 16 bytes can be written  */
      /* Check the pointers allocation */
      if (SE_BufferCheck_SBSFU(output_buffer, (uint32_t)16U) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* Call SE CRYPTO function*/
        e_ret_status =  SE_CRYPTO_Decrypt_Finish(output_buffer, output_size);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_CRYPTO_LL_AUTHENTICATE_FW_INIT_ID:
    {
      SE_FwRawHeaderTypeDef *p_x_se_Metadata;

      p_x_se_Metadata = va_arg(arguments, SE_FwRawHeaderTypeDef *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check the Init structure allocation */
      if (SE_BufferCheck_SBSFU(p_x_se_Metadata, sizeof(*p_x_se_Metadata))== SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /*
         * The se_callgate code must be crypto-agnostic.
         * But, when it comes to FW authentication, we can use:
         * - either AES GCM authentication
         * - or SHA256 (stored in an authenticated FW header)
         * So this service can rely:
         * - either on the symmetric key
         * - or a SHA256
         * As a consequence, retrieving the key cannot be done at the call gate stage.
         * This is implemented in the SE_CRYPTO_AuthenticateFW_Init code.
         */

        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_AuthenticateFW_Init(p_x_se_Metadata);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_CRYPTO_LL_AUTHENTICATE_FW_APPEND_ID:
    {

      const uint8_t *input_buffer;
      int32_t      input_size;
      uint8_t       *output_buffer;
      int32_t      *output_size;

      input_buffer = va_arg(arguments, const uint8_t *);
      input_size = va_arg(arguments, int32_t);
      output_buffer = va_arg(arguments, uint8_t *);
      output_size = va_arg(arguments, int32_t *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check the pointers allocation */
      if (input_size <=0)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(input_buffer, (uint32_t)input_size) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_size, sizeof(*output_size)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_buffer, (uint32_t)input_size) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }


      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_AuthenticateFW_Append(input_buffer, input_size, output_buffer, output_size);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_CRYPTO_LL_AUTHENTICATE_FW_FINISH_ID:
    {

      uint8_t       *output_buffer;
      int32_t       *output_size;

      output_buffer = va_arg(arguments, uint8_t *);
      output_size = va_arg(arguments, int32_t *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_SBSFU(output_size, sizeof(*output_size)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      /*  in AES-GCM 16 bytes can be written  */
      /* Check the pointers allocation */
      if (SE_BufferCheck_SBSFU(output_buffer, (uint32_t)16U) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_AuthenticateFW_Finish(output_buffer, output_size);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_CRYPTO_HL_AUTHENTICATE_METADATA:
    {
      /*
       * The se_callgate code must be crypto-agnostic.
       * But, when it comes to metadata authentication, we can use:
       * - either AES GCM authentication
       * - or SHA256 signed with ECCDSA authentication
       * So this service can rely:
       * - either on the symmetric key
       * - or on the asymmetric keys
       * As a consequence, retrieving the appropriate key cannot be done at the call gate stage.
       * This is implemented in the SE_CRYPTO_Authenticate_Metadata code.
       */
      SE_FwRawHeaderTypeDef *p_x_se_Metadata;

      p_x_se_Metadata = va_arg(arguments, SE_FwRawHeaderTypeDef *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check the metadata structure allocation */
      if (SE_BufferCheck_SBSFU(p_x_se_Metadata, sizeof(*p_x_se_Metadata))== SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      if (SE_LockRestrictedServices != SE_LOCKED)
      {
        /* We cannot read the key at this stage as we do not know yet if we use symmetric or asymmetric crypto scheme */

        /* Call SE CRYPTO function*/
        e_ret_status = SE_CRYPTO_Authenticate_Metadata(p_x_se_Metadata);
      }
      else
      {
        /* This function can be called only by the bootloader */
        e_ret_status = SE_ERROR;
      }
      break;
    }

    case SE_BOOT_INFO_READ_ALL_ID:
    {
      SE_BootInfoTypeDef    *p_boot_info;
      SE_INFO_StatusTypedef e_boot_info_status;

      /* Check that the Secure Engine services are not locked */
      __IS_SE_LOCKED_SERVICES();

      p_boot_info = va_arg(arguments, SE_BootInfoTypeDef *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check structure allocation */
      if (SE_BufferCheck_SBSFU(p_boot_info, sizeof(*p_boot_info))== SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Read BootInfo */
      if (SE_INFO_ReadBootInfoArea(p_boot_info, &e_boot_info_status) != SE_SUCCESS)
      {
        e_ret_status = SE_ERROR;
        *peSE_Status = SE_BOOT_INFO_ERR;
      }
      else
      {
        e_ret_status = SE_SUCCESS;
        *peSE_Status = SE_OK;
      }

      break;
    }

    case SE_BOOT_INFO_WRITE_ALL_ID:
    {
      SE_BootInfoTypeDef    *p_boot_info;
      SE_INFO_StatusTypedef e_boot_info_status;

      /* Check that the Secure Engine services are not locked */
      __IS_SE_LOCKED_SERVICES();

      p_boot_info = va_arg(arguments, SE_BootInfoTypeDef *);

      /* CRC configuration may have been changed by application */
      if (SE_LL_CRC_Config() == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Check structure allocation */
      if (SE_BufferCheck_SBSFU(p_boot_info, sizeof(*p_boot_info))== SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }

      /* Add CRC.
       * The CRC is computed with the structure without its CRC field and the length is provided to SE_HAL_CRC_Calculate in 32-bit word.
       * Please note that this works only if the CRC field is kept as the last uint32_t of the SE_BootInfoTypeDef structure.
       */
      p_boot_info->CRC32 = SE_LL_CRC_Calculate((uint32_t *)(p_boot_info),
                                               (sizeof(SE_BootInfoTypeDef) - sizeof(uint32_t)) / sizeof(uint32_t));

      /* Write BootInfo */
      if (SE_INFO_WriteBootInfoArea(p_boot_info, &e_boot_info_status) != SE_SUCCESS)
      {
        e_ret_status = SE_ERROR;
        *peSE_Status = SE_BOOT_INFO_ERR;
      }
      else
      {
        e_ret_status = SE_SUCCESS;
        *peSE_Status = SE_OK;
      }

      break;
    }

    case SE_IMG_READ:
    {
      void *p_destination;
      const void *p_source;
      uint32_t length;
      /* Check that the Secure Engine services are not locked */
      __IS_SE_LOCKED_SERVICES();

      p_destination = va_arg(arguments, void *);
      p_source = va_arg(arguments, const void *);
      length = va_arg(arguments, uint32_t);
      /* check the destination buffer */
      if (SE_BufferCheck_SBSFU(p_destination,length) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      e_ret_status = SE_IMG_Read(p_destination, p_source, length);
      break;
    }
    case SE_IMG_WRITE:
    {
      void *p_destination;
      const void *p_source;
      uint32_t length;
      /* Check that the Secure Engine services are not locked */
      __IS_SE_LOCKED_SERVICES();

      p_destination = va_arg(arguments, void *);
      p_source = va_arg(arguments, const void *);
      length = va_arg(arguments, uint32_t);
      /* check the source buffer */
      if (SE_BufferCheck_SBSFU(p_source, length) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      e_ret_status = SE_IMG_Write(p_destination, p_source, length);
      break;
    }

    case SE_LOCK_RESTRICT_SERVICES:
    {
      /* Check that the Secure Engine services are not locked, LOCK shall be called only once */
      __IS_SE_LOCKED_SERVICES();
      SE_LL_CORE_Cleanup();
      SE_LockRestrictedServices = SE_LOCKED;
      e_ret_status = SE_SUCCESS;
      break;
    }

    /* ============================ */
    /* ===== APPLICATION PART ===== */
    /* ============================ */

    /* --------------------------------- */
    /* FIRMWARE IMAGES HANDLING SERVICES */
    /* --------------------------------- */

    /* No protected service needed for this */

    /* --------------------------------- */
    /* USER APPLICATION SERVICES         */
    /* --------------------------------- */
    case SE_APP_GET_ACTIVE_FW_INFO:
    {
      SE_APP_ActiveFwInfo *p_FwInfo;

      p_FwInfo = va_arg(arguments, SE_APP_ActiveFwInfo *);
      if (SE_LL_Buffer_in_ram((void *)p_FwInfo, sizeof(*p_FwInfo)) == SE_ERROR)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      if (SE_BufferCheck_in_se_ram((void *)p_FwInfo, sizeof(*p_FwInfo)) == SE_SUCCESS)
      {
        e_ret_status = SE_ERROR;
        break;
      }
      e_ret_status = SE_APPLI_GetActiveFwInfo(p_FwInfo);
      break;
    }

    default:
    {
      e_ret_status = SE_ERROR;

      break;
    }
  }
  if ((e_ret_status == SE_ERROR) && (*peSE_Status==SE_OK))
  {
    *peSE_Status=SE_KO;
  }
  return e_ret_status;
}

/* Stop placing data in specified section*/



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
