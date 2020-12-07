/**
  ******************************************************************************
  * @file    se_low_level.c
  * @author  MCD Application Team
  * @brief   Secure Engine Interface module.
  *          This file provides set of firmware functions to manage SE low level
  *          interface functionalities.
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
#if defined(__ICCARM__)
#pragma default_function_attributes = @ ".SE_IF_Code"
#elif defined(__CC_ARM)
#pragma arm section code = ".SE_IF_Code"
#endif

/* Includes ------------------------------------------------------------------*/
#include "se_low_level.h"
#if defined(__CC_ARM)
/* to check  se call parameter we need to get se / sbsfu boundary as sfu_fwimg_regions.h does not include it for __CC_ARM */
#include "mapping_sbsfu.h"
#endif
#include "se_exception.h"
#include "string.h"

/** @addtogroup SE Secure Engine
  * @{
  */
/** @defgroup  SE_HARDWARE SE Hardware Interface
  * @{
  */

/** @defgroup SE_HARDWARE_Private_Variables Private Variables
  * @{
  */
static CRC_HandleTypeDef    CrcHandle;                  /*!< SE Crc Handle*/
static uint32_t FlashSectorsAddress[] = {0x08000000U, 0x08008000U, 0x08010000U, 0x08018000U, 0x08020000U, 0x08040000U, 0x08080000U, 0x080C0000U,
                                         0x08100000U, 0x08140000U, 0x08180000U, 0x081C0000U, 0x08200000U
                                        };           /* Flash sector start address */


/**
  * @}
  */

/** @defgroup SE_HARDWARE_Private_Functions Private Functions
  * @{
  */

static uint32_t SE_LL_GetSector(uint32_t Address);
/**
  * @}
  */

/** @defgroup SE_HARDWARE_Exported_Variables Exported Variables
  * @{
  */

/**
  * @}
  */

/** @defgroup SE_HARDWARE_Exported_Functions Exported Functions
  * @{
  */

/** @defgroup SE_HARDWARE_Exported_CRC_Functions CRC Exported Functions
  * @{
  */

/**
  * @brief  Set CRC configuration and call HAL CRC initialization function.
  * @param  None.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise
  */
SE_ErrorStatus SE_LL_CRC_Config(void)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;

  CrcHandle.Instance = CRC;
  /* The input data are not inverted */
  CrcHandle.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;

  /* The output data are not inverted */
  CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;

  /* The Default polynomial is used */
  CrcHandle.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  /* The default init value is used */
  CrcHandle.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  /* The input data are 32-bit long words */
  CrcHandle.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  /* CRC Init*/
  if (HAL_CRC_Init(&CrcHandle) == HAL_OK)
  {
    e_ret_status = SE_SUCCESS;
  }

  return e_ret_status;
}

/**
  * @brief  Wrapper to HAL CRC initilization function.
  * @param  None
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_LL_CRC_Init(void)
{
  /* CRC Peripheral clock enable */
  __HAL_RCC_CRC_CLK_ENABLE();

  return SE_LL_CRC_Config();
}

/**
  * @brief  Wrapper to HAL CRC de-initilization function.
  * @param  None
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_LL_CRC_DeInit(void)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;

  if (HAL_CRC_DeInit(&CrcHandle) == HAL_OK)
  {
    /* Initialization OK */
    e_ret_status = SE_SUCCESS;
  }

  return e_ret_status;
}

/**
  * @brief  Wrapper to HAL CRC Calculate function.
  * @param  pBuffer: pointer to data buffer.
  * @param  uBufferLength: buffer length in 32-bits word.
  * @retval uint32_t CRC (returned value LSBs for CRC shorter than 32 bits)
  */
uint32_t SE_LL_CRC_Calculate(uint32_t pBuffer[], uint32_t uBufferLength)
{
  return HAL_CRC_Calculate(&CrcHandle, pBuffer, uBufferLength);
}

/**
  * @}
  */

/** @defgroup SE_HARDWARE_Exported_FLASH_Functions FLASH Exported Functions
  * @{
  */

/**
  * @brief  This function does an erase of nb sectors in user flash area
  * @param  pStart: pointer to  user flash area
  * @param  Length: number of bytes.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_LL_FLASH_Erase(void *pStart, uint32_t Length)
{
  uint32_t sector_error = 0U;
  FLASH_EraseInitTypeDef p_erase_init;
  SE_ErrorStatus e_ret_status = SE_SUCCESS;

  /* Unlock the Flash to enable the flash control register access *************/
  if (HAL_FLASH_Unlock() == HAL_OK)
  {
    /* Fill EraseInit structure*/
    p_erase_init.TypeErase     = FLASH_TYPEERASE_SECTORS;
    p_erase_init.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    p_erase_init.Sector        = SE_LL_GetSector((uint32_t) pStart);
    p_erase_init.NbSectors     = SE_LL_GetSector(((uint32_t) pStart) + Length - 1U) - p_erase_init.Sector + 1U;
    if (HAL_FLASHEx_Erase(&p_erase_init, &sector_error) != HAL_OK)
    {
      e_ret_status = SE_ERROR;
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    (void)HAL_FLASH_Lock();
  }
  else
  {
    e_ret_status = SE_ERROR;
  }

  return e_ret_status;
}

/**
  * @brief  Write in Flash  protected area
  * @param  pDestination pointer to destination area in Flash
  * @param  pSource pointer to input buffer
  * @param  Length number of bytes to be written
  * @retval SE_SUCCESS if successful, otherwise SE_ERROR
  */

SE_ErrorStatus SE_LL_FLASH_Write(void *pDestination, const void *pSource, uint32_t Length)
{
  SE_ErrorStatus ret = SE_SUCCESS;
  uint32_t i;
  uint32_t pdata = (uint32_t)pSource;
  uint32_t areabegin = (uint32_t)pDestination;

  /* Test if access is in this range : SLOT 0 header */
  if (Length == 0)
  {  
    return SE_ERROR;
  }
  
  if (((uint32_t)pDestination < SFU_IMG_SLOT_0_REGION_BEGIN_VALUE) ||
      (((uint32_t)pDestination + Length) > (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE + SFU_IMG_IMAGE_OFFSET)))
  {
    return SE_ERROR;
  }
  
  /* Unlock the Flash to enable the flash control register access *************/
  if (HAL_FLASH_Unlock() == HAL_OK)
  {
    for (i = 0U; i < Length; i += 4U)
    {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (areabegin + i), *(uint32_t *)(pdata + i)) != HAL_OK)
      {
        ret = SE_ERROR;
        break;
      }
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) */
    (void)HAL_FLASH_Lock();
  }
  else
  {
    ret = SE_ERROR;
  }
  return ret;
}

/**
  * @brief  Read in Flash protected area
  * @param  pDestination: Start address for target location
  * @param  pSource: pointer on buffer with data to read
  * @param  Length: Length in bytes of data buffer
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SE_ErrorStatus SE_LL_FLASH_Read(void *pDestination, const void *pSource, uint32_t Length)
{

  /* Test if access is in this range : SLOT 0 header */
  if (Length == 0)
  {
    return SE_ERROR;
  }

  if (((uint32_t)pSource < SFU_IMG_SLOT_0_REGION_BEGIN_VALUE) ||
      ((uint32_t)pSource + Length) > (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE + SFU_IMG_IMAGE_OFFSET))
  {
    return SE_ERROR;
  }

  (void)memcpy(pDestination, pSource, Length);
  return SE_SUCCESS;
}

/**
 * @brief Check if an array is inside the RAM of the product
 * @param Addr : address  of array
 * @param Length : legnth of array in byte
 */
SE_ErrorStatus SE_LL_Buffer_in_ram(void *pBuff, uint32_t Length)
{
  SE_ErrorStatus ret = SE_ERROR;
  uint32_t addr_start = (uint32_t)pBuff;
  uint32_t addr_end = addr_start + Length - 1U;
  if ((Length != 0U) && (addr_start >= RAMDTCM_BASE) && (addr_end <= 0x2007FFFFU))

            {
              ret = SE_SUCCESS;
            }
  return ret;
}
/**
  * @brief function checking if a buffer is in sbsfu ram.
  * @param pBuff: Secure Engine protected function ID.
  * @param length: length of buffer in bytes
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_BufferCheck_SBSFU(const void *pBuff, uint32_t length)
{
  SE_ErrorStatus e_ret_status;
  uint32_t addr_start = (uint32_t)pBuff;
  uint32_t addr_end = addr_start + length - 1U;
  if ((length != 0U) && ((addr_end  <= SB_REGION_SRAM1_END) && (addr_start >= SB_REGION_SRAM1_START)))
  {
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    e_ret_status = SE_ERROR;
  }
  return e_ret_status;
}
/**
  * @brief function checking if a buffer is in se ram.
  * @param pBuff: Secure Engine protected function ID.
  * @param length: length of buffer in bytes
  * @retval SE_ErrorStatus SE_SUCCESS for buffer in se ram, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_BufferCheck_in_se_ram(const void *pBuff, uint32_t length)
{
  SE_ErrorStatus e_ret_status;
  uint32_t addr_start = (uint32_t)pBuff;
  uint32_t addr_end = addr_start + length - 1U;
  if ((length != 0U) && ((addr_end  <= SE_REGION_SRAM1_END) && (addr_start >= SE_REGION_SRAM1_START)))
  {
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    e_ret_status = SE_ERROR;
  }
  return e_ret_status;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup SE_HARDWARE_Private_Functions
  * @{
  */

/**
 * @brief Check if an array is inside the RAM of the product
 * @param Addr : address  of array
 * @param Length : legnth of array in byte
 */

/**
  * @brief  Gets the sector of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The sector of a given address
  */
static uint32_t SE_LL_GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  while (Address >= FlashSectorsAddress[sector + 1])
  {
    sector++;
  }
  return sector;
}


/**
  * @brief  Cleanup SE CORE
  * The fonction is called  during SE_LOCK_RESTRICT_SERVICES.
  *
  */
void  SE_LL_CORE_Cleanup(void)
{
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

/* Stop placing data in specified section*/
#if defined(__ICCARM__)
#pragma default_function_attributes =
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
