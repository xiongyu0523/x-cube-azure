/**
  ******************************************************************************
  * @file    sfu_low_level_flash.c
  * @author  MCD Application Team
  * @brief   SFU Flash Low Level Interface module
  *          This file provides set of firmware functions to manage SFU flash
  *          low level interface.
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
#include "main.h"
#include "sfu_low_level_flash.h"
#include "sfu_low_level_security.h"
#include "se_interface_bootloader.h"
#include "string.h"


/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup  SFU_LOW_LEVEL
  * @{
  */

/** @defgroup SFU_LOW_LEVEL_FLASH Flash Low Level Interface
  * @{
  */

/** @defgroup SFU_FLASH_Private_Definition Private Definitions
  * @{
  */
#define NB_PAGE_SECTOR_PER_ERASE  2U    /*!< Nb page erased per erase */

/**
  * @}
  */
/** @defgroup SFU_FLASH_Private_Variables Private Variables
  * @{
  */

uint32_t FlashSectorsAddress[] = {0x08000000U, 0x08008000U, 0x08010000U, 0x08018000U, 0x08020000U, 0x08040000U, 0x08080000U, 0x080C0000U,
                                  0x08100000U, 0x08140000U, 0x08180000U, 0x081C0000U, 0x08200000U
                                 };

/**
  * @}
  */

/** @defgroup SFU_FLASH_Private_Functions Private Functions
  * @{
  */
static SFU_ErrorStatus SFU_LL_FLASH_Init(void);

/**
  * @}
  */

/** @addtogroup SFU_FLASH_Exported_Functions
  * @{
  */

/**
 * @brief  This function does an erase of n (depends on Length) pages in user flash area
 * @param  pFlashStatus: SFU_FLASH Status pointer
 * @param  pStart: pointer to  user flash area
 * @param  Length: number of bytes.
 * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
 */
SFU_ErrorStatus SFU_LL_FLASH_Erase_Size(SFU_FLASH_StatusTypeDef *pFlashStatus, void *pStart, uint32_t Length)
{
  uint32_t sector_error = 0U;
  uint32_t start = (uint32_t)pStart;
  FLASH_EraseInitTypeDef p_erase_init;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint32_t first_sector = 0U, nb_sectors = 0U;
  uint32_t chunk_nb_sectors;

  /* Check the pointers allocation */
  if (pFlashStatus == NULL)
  {
    return SFU_ERROR;
  }

  *pFlashStatus = SFU_FLASH_SUCCESS;

  /* Initialize Flash - erase application code area*/
  e_ret_status = SFU_LL_FLASH_Init();

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Unlock the Flash to enable the flash control register access *************/
    if (HAL_FLASH_Unlock() == HAL_OK)
    {
      first_sector = SFU_LL_FLASH_GetSector(start);
      /* Get the number of sectors to erase from 1st sector */
      nb_sectors = SFU_LL_FLASH_GetSector(start + Length - 1U) - first_sector + 1U;

      /* Fill EraseInit structure*/
      p_erase_init.TypeErase     = FLASH_TYPEERASE_SECTORS;
      p_erase_init.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
      /* Erase flash per NB_PAGE_SECTOR_PER_ERASE to avoid watch-dog */
      do
      {
        chunk_nb_sectors = (nb_sectors >= NB_PAGE_SECTOR_PER_ERASE) ? NB_PAGE_SECTOR_PER_ERASE : nb_sectors;
        p_erase_init.Sector = first_sector;
        p_erase_init.NbSectors = chunk_nb_sectors;
        first_sector += chunk_nb_sectors;
        nb_sectors -= chunk_nb_sectors;
        if (HAL_FLASHEx_Erase(&p_erase_init, &sector_error) != HAL_OK)
        {
          e_ret_status = SFU_ERROR;
          *pFlashStatus = SFU_FLASH_ERR_ERASE;
        }
        SFU_LL_SECU_IWDG_Refresh(); /* calling this function which checks the compiler switch */
      }
      while (nb_sectors > 0);

      /* Lock the Flash to disable the flash control register access (recommended
      to protect the FLASH memory against possible unwanted operation) *********/
      HAL_FLASH_Lock();

    }
    else
    {
      *pFlashStatus = SFU_FLASH_ERR_HAL;
    }
  }

  return e_ret_status;
}


/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  pFlashStatus: FLASH_StatusTypeDef
  * @param  pDestination: Start address for target location
  * @param  pSource: pointer on buffer with data to write
  * @param  Length: number of bytes (it has to be 64-bit aligned).
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_Write(SFU_FLASH_StatusTypeDef *pFlashStatus, void  *pDestination, const void *pSource,
                                   uint32_t Length)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  uint32_t i = 0U;
  uint32_t pdata = (uint32_t)pSource;

  /* Check the pointers allocation */
  if ((pFlashStatus == NULL) || (pSource == NULL))
  {
    return SFU_ERROR;
  }
  /* Test if access is in this range : SLOT 0 header */
  if ((Length != 0) && ((uint32_t)pDestination >= SFU_IMG_SLOT_0_REGION_BEGIN_VALUE) &&
      ((((uint32_t)pDestination + Length - 1)) < (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE + SFU_IMG_IMAGE_OFFSET))
     )
  {
    /* SE Access */
    SE_StatusTypeDef se_status;
    SE_ErrorStatus se_ret_status = SE_SFU_IMG_Write(&se_status, pDestination, pSource, Length);
    if (se_ret_status == SE_SUCCESS)
    {
      e_ret_status = SFU_SUCCESS;
      *pFlashStatus = SFU_FLASH_SUCCESS;
    }
    else
    {
      e_ret_status = SFU_ERROR;
      *pFlashStatus = SFU_FLASH_ERROR;
    }
  }
  else
  {
    *pFlashStatus = SFU_FLASH_ERROR;

    /* Initialize Flash - erase application code area*/
    e_ret_status = SFU_LL_FLASH_Init();

    if (e_ret_status == SFU_SUCCESS)
    {
      /* Unlock the Flash to enable the flash control register access *************/
      if (HAL_FLASH_Unlock() != HAL_OK)
      {
        *pFlashStatus = SFU_FLASH_ERR_HAL;

      }
      else
      {
        for (i = 0U; i < Length; i += sizeof(SFU_LL_FLASH_write_t))
        {
          *pFlashStatus = SFU_FLASH_ERROR;
          if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)pDestination, *((uint32_t *)(pdata + i))) == HAL_OK)
          {
            /* Check the written value */
            if (*(uint32_t *)pDestination != *(uint32_t *)(pdata + i))
            {
              /* Flash content doesn't match SRAM content */
              *pFlashStatus = SFU_FLASH_ERR_WRITINGCTRL;
              e_ret_status = SFU_ERROR;
              break;
            }
            else
            {
              /* Increment FLASH Destination address */
              pDestination = (uint8_t *)pDestination +  + sizeof(SFU_LL_FLASH_write_t);
              e_ret_status = SFU_SUCCESS;
              *pFlashStatus = SFU_FLASH_SUCCESS;
            }
          }
          else
          {
            /* Error occurred while writing data in Flash memory */
            *pFlashStatus = SFU_FLASH_ERR_WRITING;
            e_ret_status = SFU_ERROR;
            break;
          }
        }
        /* Lock the Flash to disable the flash control register access (recommended
        to protect the FLASH memory against possible unwanted operation) */
        HAL_FLASH_Lock();
      }
    }
  }
  return e_ret_status;
}

/**
  * @brief  This function reads flash
  * @param  pDestination: Start address for target location
  * @param  pSource: pointer on buffer with data to write
  * @param  Length: Length in bytes of data buffer
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_Read(void *pDestination, const void *pSource, uint32_t Length)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Test if access is in this range : SLOT 0 header */
  if (((uint32_t)pSource >= SFU_IMG_SLOT_0_REGION_BEGIN_VALUE) &&
      ((((uint32_t)pSource + Length - 1)) < (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE + SFU_IMG_IMAGE_OFFSET))
     )
  {
    /* SE Access */
    SE_StatusTypeDef se_status;
    SE_ErrorStatus se_ret_status = SE_SFU_IMG_Read(&se_status, pDestination, pSource, Length);
    if (se_ret_status == SE_SUCCESS)
    {
      e_ret_status = SFU_SUCCESS;
    }
    else
    {
      e_ret_status = SFU_ERROR;
    }
  }
  else
  {
    memcpy(pDestination, pSource, Length);
    e_ret_status = SFU_SUCCESS;
  }
  return e_ret_status;
}


/**
  * @brief  Gets the sector of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The sector of a given address
  */
uint32_t SFU_LL_FLASH_GetSector(uint32_t Add)
{
  uint32_t sector = 0;

  while (Add >= FlashSectorsAddress[sector + 1])
  {
    sector++;
  }
  return sector;
}





/**
  * @}
  */

/** @defgroup SFU_FLASH_Private_Functions Private Functions
  * @{
  */

/**
  * @brief  Unlocks Flash for write access.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_LL_FLASH_Init(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Unlock the Program memory */
  if (HAL_FLASH_Unlock() == HAL_OK)
  {

    /* Clear all FLASH flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    /* Unlock the Program memory */
    if (HAL_FLASH_Lock() == HAL_OK)
    {
      e_ret_status = SFU_SUCCESS;
    }
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
