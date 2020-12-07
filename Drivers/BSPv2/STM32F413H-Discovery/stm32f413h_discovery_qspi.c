/**
  ******************************************************************************
  * @file    stm32f413h_discovery_qspi.c
  * @author  MCD Application Team
  * @brief   This file includes a standard driver for the N25Q128A QSPI
  *          memory mounted on STM32F413H-DISCO board.
  ******************************************************************************
  @verbatim
  How To use this driver:
  -----------------------
   - This driver is used to drive the N25Q128A QSPI external
       memory mounted on STM32F413H-DISCO evaluation board.

   - This driver need a specific component driver (N25Q128A) to be included with.

  Driver description:
  -------------------
   - Initialization steps:
       + Initialize the QPSI external memory using the BSP_QSPI_Init() function. This
            function includes the MSP layer hardware resources initialization and the
            QSPI interface with the external memory.
         Only STR transfer rate is supported.
         SPI, SPI 1I-2O, SPI 2-IO, SPI 1I-4O, SPI-4IO, DPI and QPI modes are supported

   - QSPI memory operations
       + QSPI memory can be accessed with read/write operations once it is
            initialized.
            Read/write operation can be performed with AHB access using the functions
            BSP_QSPI_Read()/BSP_QSPI_Write().
       + The function BSP_QSPI_GetInfo() returns the configuration of the QSPI memory.
            (see the QSPI memory data sheet)
       + Perform erase block operation using the function BSP_QSPI_EraseBlock() and by
            specifying the block address. You can perform an erase operation of the whole
            chip by calling the function BSP_QSPI_EraseChip().
       + The function BSP_QSPI_GetStatus() returns the current status of the QSPI memory.
            (see the QSPI memory data sheet)
       + The function BSP_QSPI_EnableMemoryMappedMode enables the QSPI memory mapped mode
       + The function BSP_QSPI_DisableMemoryMappedMode disables the QSPI memory mapped mode
       + The function BSP_QSPI_ConfigFlash() allow to configure the QSPI mode and transfer rate

  Note:
  --------
    Regarding the "Instance" parameter, needed for all functions, it is used to select
    an QSPI instance. On the STM32F413H_DISCOVERY board, there's one instance. Then, this
    parameter should be 0.

  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_qspi.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_QSPI QSPI
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_QSPI_Exported_Variables QSPI Exported Variables
  * @{
  */
QSPI_HandleTypeDef hqspi[QSPI_INSTANCES_NUMBER];
QSPI_Ctx_t Qspi_Ctx[QSPI_INSTANCES_NUMBER] = {0};
/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/

/** @defgroup STM32F413H_DISCOVERY_QSPI_Private_Functions QSPI Private Functions
  * @{
  */
static void QSPI_MspInit(QSPI_HandleTypeDef *hQspi);
static void QSPI_MspDeInit(QSPI_HandleTypeDef *hQspi);
static int32_t QSPI_ResetMemory(uint32_t Instance);
static int32_t QSPI_SetODS(uint32_t Instance);
static int32_t QUADSPI_SwitchMode(uint32_t Instance, BSP_QSPI_Interface_t Mode, BSP_QSPI_Interface_t NewMode);

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_QSPI_Exported_Functions QSPI Exported Functions
  * @{
  */

/**
  * @brief  Initializes the QSPI interface.
  * @param  Instance   QSPI Instance
  * @param  Init       QSPI Init structure
  * @retval BSP status
  */
int32_t BSP_QSPI_Init(uint32_t Instance, BSP_QSPI_Init_t *Init)
{
  int32_t ret;
  BSP_QSPI_Info_t pInfo;
  MX_QSPI_Config qspi_config;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Check if instance is already initialized */
    if(Qspi_Ctx[Instance].IsInitialized == QSPI_ACCESS_NONE)
    {
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
      /* Register the QSPI MSP Callbacks */
      if(Qspi_Ctx[Instance].IsMspCallbacksValid == 0UL)
      {
        if(BSP_QSPI_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
        {
          return BSP_ERROR_PERIPH_FAILURE;
        }
      }
#else
      /* Msp QSPI initialization */
      QSPI_MspInit(&hqspi[Instance]);
#endif /* USE_HAL_QSPI_REGISTER_CALLBACKS */

      /* STM32 QSPI interface initialization */
      (void)N25Q128A_GetFlashInfo(&pInfo);
      /* The max frequency in STR mode is 100MHZ so prescaler is set to 0
         Only single flash is supported so DualFlash mode is always QSPI_DUALFLASH_DISABLE */
      qspi_config.ClockPrescaler = 0;
      qspi_config.DualFlashMode  = BSP_QSPI_DUALFLASH_DISABLE;
      qspi_config.FlashSize      = (uint32_t)POSITION_VAL((uint32_t)pInfo.FlashSize) - 1U;
      qspi_config.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;

      if(MX_QSPI_Init(&hqspi[Instance], &qspi_config) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
       /* QSPI memory reset */
      else if(QSPI_ResetMemory(Instance) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configuration of the Output driver strength on memory side **************/
      else if(QSPI_SetODS(Instance) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure Flash to desired mode, only Single Transfer Rate is supported */
      else if(BSP_QSPI_ConfigFlash(Instance, Init->InterfaceMode, BSP_QSPI_STR_TRANSFER) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  De-Initializes the QSPI interface.
  * @param  Instance   QSPI Instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
	  if(Qspi_Ctx[Instance].IsInitialized != QSPI_ACCESS_NONE)
	  {
    if(Qspi_Ctx[Instance].IsInitialized == QSPI_ACCESS_MMP)
    {
      if(BSP_QSPI_DisableMemoryMappedMode(Instance) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_COMPONENT_FAILURE;
      }
    }

    /* Set default Qspi_Ctx values */
    Qspi_Ctx[Instance].IsInitialized = QSPI_ACCESS_NONE;
    Qspi_Ctx[Instance].InterfaceMode = BSP_QSPI_SPI_MODE;
    Qspi_Ctx[Instance].TransferRate  = BSP_QSPI_STR_TRANSFER;
    Qspi_Ctx[Instance].DualFlashMode = BSP_QSPI_DUALFLASH_DISABLE;

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 0)
    QSPI_MspDeInit(&hqspi[Instance]);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 0) */

    /* Call the DeInit function to reset the driver */
    if (HAL_QSPI_DeInit(&hqspi[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }
	}

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Initializes the QSPI interface.
  * @param  hQspi       QSPI handle
  * @param  Config      QSPI configuration structure
  * @retval BSP status
  */
__weak HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef *hQspi, MX_QSPI_Config *Config)
{
  /* QSPI initialization */
  /* QSPI freq = SYSCLK /(1 + ClockPrescaler) Mhz */
  hQspi->Instance                = QUADSPI;
  hQspi->Init.ClockPrescaler     = Config->ClockPrescaler;
  hQspi->Init.FifoThreshold      = 4;
  hQspi->Init.SampleShifting     = Config->SampleShifting;
  hQspi->Init.FlashSize          = Config->FlashSize;
  hQspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_5_CYCLE; /* Min 50ns for nonRead */
  hQspi->Init.ClockMode          = QSPI_CLOCK_MODE_0;
  hQspi->Init.FlashID            = QSPI_FLASH_ID_1;
  hQspi->Init.DualFlash          = Config->DualFlashMode;

  return HAL_QSPI_Init(hQspi);
}

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP QSPI Msp Callbacks
  * @param Instance      QSPI Instance
  * @retval BSP status
  */
int32_t BSP_QSPI_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_QSPI_RegisterCallback(&hqspi[Instance], HAL_QSPI_MSPINIT_CB_ID, QSPI_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_QSPI_RegisterCallback(&hqspi[Instance], HAL_QSPI_MSPDEINIT_CB_ID, QSPI_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      Qspi_Ctx[Instance].IsMspCallbacksValid = 1U;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP QSPI Msp Callback registering
  * @param Instance     QSPI Instance
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_QSPI_RegisterMspCallbacks (uint32_t Instance, BSP_QSPI_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_QSPI_RegisterCallback(&hqspi[Instance], HAL_QSPI_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_QSPI_RegisterCallback(&hqspi[Instance], HAL_QSPI_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      Qspi_Ctx[Instance].IsMspCallbacksValid = 1U;
    }
  }

  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  Instance  QSPI instance
  * @param  pData     Pointer to data to be read
  * @param  ReadAddr  Read start address
  * @param  Size      Size of data to read
  * @retval BSP status
  */
int32_t BSP_QSPI_Read(uint32_t Instance, uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(N25Q128A_ReadSTR(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode, pData, ReadAddr, Size) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  Instance   QSPI instance
  * @param  pData      Pointer to data to be written
  * @param  WriteAddr  Write start address
  * @param  Size       Size of data to write
  * @retval BSP status
  */
int32_t BSP_QSPI_Write(uint32_t Instance, uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t end_addr, current_size, current_addr;
  uint8_t *write_data;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Calculation of the size between the write address and the end of the page */
    current_size = N25Q128A_PAGE_SIZE - (WriteAddr % N25Q128A_PAGE_SIZE);

    /* Check if the size of the data is less than the remaining place in the page */
    if (current_size > Size)
    {
      current_size = Size;
    }

    /* Initialize the address variables */
    current_addr = WriteAddr;
    end_addr = WriteAddr + Size;
    write_data = pData;

    /* Perform the write page by page */
    do
    {
      /* Check if Flash busy ? */
      if(N25Q128A_AutoPollingMemReady(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(N25Q128A_WriteEnable(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Issue page program command */
      else if(N25Q128A_PageProgram(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode, write_data, current_addr, current_size) != N25Q128A_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Configure automatic polling mode to wait for end of program */
      else if (N25Q128A_AutoPollingMemReady(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        /* Update the address and size variables for next page programming */
        current_addr += current_size;
        write_data += current_size;
        current_size = ((current_addr + N25Q128A_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : N25Q128A_PAGE_SIZE;
      }
    } while ((current_addr < end_addr) && (ret == BSP_ERROR_NONE));
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  *         N25Q128A support 4K and 64K size block erase commands.
  * @param  Instance     QSPI instance
  * @param  BlockAddress Block address to erase
  * @param  BlockSize    Erase Block size
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseBlock(uint32_t Instance, uint32_t BlockAddress, BSP_QSPI_Erase_t BlockSize)
{
  int32_t ret;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(N25Q128A_WriteEnable(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Issue Block Erase command */
    else if(N25Q128A_BlockErase(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode, BlockAddress, BlockSize) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Erases the entire QSPI memory.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseChip(uint32_t Instance)
{
  int32_t ret;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Check Flash busy ? */
    if(N25Q128A_AutoPollingMemReady(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Enable write operations */
    else if (N25Q128A_WriteEnable(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Issue Chip erase command */
    else if(N25Q128A_ChipErase(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Reads current status of the QSPI memory.
  *         If WIP != 0 then return busy.
  * @param  Instance  QSPI instance
  * @retval QSPI memory status: whether busy or not
  */
int32_t BSP_QSPI_GetStatus(uint32_t Instance)
{
  uint8_t reg;
  int32_t ret;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(N25Q128A_ReadStatusRegister(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode, &reg) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Check the value of the register */
    else if ((reg & N25Q128A_SR_WIP) != 0U)
    {
      ret = BSP_ERROR_BUSY;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  Instance  QSPI instance
  * @param  pInfo     pointer on the configuration structure
  * @retval BSP status
  */
int32_t BSP_QSPI_GetInfo(uint32_t Instance, BSP_QSPI_Info_t *pInfo)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    (void)N25Q128A_GetFlashInfo(pInfo);
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  *         Only 1 Instance can running MMP mode. And it will lock system at this mode.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnableMemoryMappedMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(N25Q128A_EnableMemoryMappedModeSTR(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else /* Update QSPI context if all operations are well done */
    {
      Qspi_Ctx[Instance].IsInitialized = QSPI_ACCESS_MMP;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Exit form memory-mapped mode
  *         Only 1 Instance can running MMP mode. And it will lock system at this mode.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DisableMemoryMappedMode(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Qspi_Ctx[Instance].IsInitialized != QSPI_ACCESS_MMP)
    {
      ret = BSP_ERROR_QSPI_MMP_UNLOCK_FAILURE;
    }/* Abort MMP back to indirect mode */
    else if(HAL_QSPI_Abort(&hqspi[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else /* Update QSPI context if all operations are well done */
    {
      Qspi_Ctx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get flash ID, 3 Byte
  *         Manufacturer ID, Memory type, Memory density
  * @param  Instance  QSPI instance
  * @param  Id Pointer to Flash ID  
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadID(uint32_t Instance, uint8_t *Id)
{
  int32_t ret;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(N25Q128A_ReadID(&hqspi[Instance], Qspi_Ctx[Instance].InterfaceMode, Id) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set Flash to desired Interface mode. And this instance becomes current instance.
  *         If current instance running at MMP mode then this function isn't work.
  *         Indirect -> Indirect
  * @param  Instance  QSPI instance
  * @param  Mode      QSPI mode
  * @param  Rate      QSPI transfer rate
  * @retval BSP status
  */
int32_t BSP_QSPI_ConfigFlash(uint32_t Instance, BSP_QSPI_Interface_t Mode, BSP_QSPI_Transfer_t Rate)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= QSPI_INSTANCES_NUMBER)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Check if MMP mode locked ************************************************/
    if(Qspi_Ctx[Instance].IsInitialized == QSPI_ACCESS_MMP)
    {
      ret = BSP_ERROR_QSPI_MMP_LOCK_FAILURE;
    }
    else if(QUADSPI_SwitchMode(Instance, Qspi_Ctx[Instance].InterfaceMode, Mode) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* Update current status parameter *****************************************/
      Qspi_Ctx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;
      Qspi_Ctx[Instance].InterfaceMode = Mode;
      Qspi_Ctx[Instance].TransferRate  = BSP_QSPI_STR_TRANSFER;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Selectt the Flash ID.
  * @param  Instance  QSPI instance
  * @param  FlashID   Id of the flash to be used.
  * @note   This function should be used to select the flash ID in case a dual flash
  *         memory is mounted on the board. For the STM32F413H-Discovery, the dual flash
  *         is not supported
  * @retval BSP status
  */
int32_t BSP_QSPI_SelectFlashID(uint32_t Instance, uint32_t FlashID)
{
  return BSP_ERROR_FEATURE_NOT_SUPPORTED;
}

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_QSPI_Private_Functions QSPI Private Functions
  * @{
  */

/**
  * @brief QSPI MSP Initialization
  * @param hQspi QSPI handle
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - NVIC configuration for QSPI interrupt
  * @retval None
  */
static void QSPI_MspInit(QSPI_HandleTypeDef *hQspi)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hQspi);

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable the QuadSPI memory interface clock */
  QSPI_CLK_ENABLE();
  /* Reset the QuadSPI memory interface */
  QSPI_FORCE_RESET();
  QSPI_RELEASE_RESET();
  /* Enable GPIO clocks */
  QSPI_CS_GPIO_CLK_ENABLE();
  QSPI_CLK_GPIO_CLK_ENABLE();
  QSPI_D0_GPIO_CLK_ENABLE();
  QSPI_D1_GPIO_CLK_ENABLE();
  QSPI_D2_GPIO_CLK_ENABLE();
  QSPI_D3_GPIO_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* QSPI CS GPIO pin configuration  */
  gpio_init_structure.Pin       = QSPI_CS_PIN;
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull      = GPIO_PULLUP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio_init_structure.Alternate = QSPI_CS_PIN_AF;
  HAL_GPIO_Init(QSPI_CS_GPIO_PORT, &gpio_init_structure);

  /* QSPI CLK GPIO pin configuration  */
  gpio_init_structure.Pin       = QSPI_CLK_PIN;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Alternate = QSPI_CLK_PIN_AF;
  HAL_GPIO_Init(QSPI_CLK_GPIO_PORT, &gpio_init_structure);

  /* QSPI D0 GPIO pin configuration  */
  gpio_init_structure.Pin       = QSPI_D0_PIN;
  gpio_init_structure.Alternate = QSPI_D0_PIN_AF;
  HAL_GPIO_Init(QSPI_D0_GPIO_PORT, &gpio_init_structure);

  /* QSPI D1 GPIO pin configuration  */
  gpio_init_structure.Pin       = QSPI_D1_PIN;
  gpio_init_structure.Alternate = QSPI_D1_PIN_AF;
  HAL_GPIO_Init(QSPI_D1_GPIO_PORT, &gpio_init_structure);

  /* QSPI D2 GPIO pin configuration  */
  gpio_init_structure.Pin       = QSPI_D2_PIN;
  gpio_init_structure.Alternate = QSPI_D2_PIN_AF;
  HAL_GPIO_Init(QSPI_D2_GPIO_PORT, &gpio_init_structure);

  /* QSPI D3 GPIO pin configuration  */
  gpio_init_structure.Pin       = QSPI_D3_PIN;
  gpio_init_structure.Alternate = QSPI_D3_PIN_AF;
  HAL_GPIO_Init(QSPI_D3_GPIO_PORT, &gpio_init_structure);
}

/**
  * @brief QSPI MSP De-Initialization
  * @param hQspi QSPI handle
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO and NVIC configuration to their default state
  * @retval None
  */
static void QSPI_MspDeInit(QSPI_HandleTypeDef *hQspi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hQspi);

  /*##-2- Disable peripherals and GPIO Clocks ################################*/
  /* De-Configure QSPI pins */
  HAL_GPIO_DeInit(QSPI_CS_GPIO_PORT, QSPI_CS_PIN);
  HAL_GPIO_DeInit(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN);
  HAL_GPIO_DeInit(QSPI_D0_GPIO_PORT, QSPI_D0_PIN);
  HAL_GPIO_DeInit(QSPI_D1_GPIO_PORT, QSPI_D1_PIN);
  HAL_GPIO_DeInit(QSPI_D2_GPIO_PORT, QSPI_D2_PIN);
  HAL_GPIO_DeInit(QSPI_D3_GPIO_PORT, QSPI_D3_PIN);

  /*##-3- Reset peripherals ##################################################*/
  /* Reset the QuadSPI memory interface */
  QSPI_FORCE_RESET();
  QSPI_RELEASE_RESET();

  /* Disable the QuadSPI memory interface clock */
  QSPI_CLK_DISABLE();
}

/**
  * @brief  This function reset the QSPI Flash memory.
  *         Fore QPI+DPI+SPI reset to avoid system come from unknown status.
  *         Flash accept 1-1-1, 2-2-2, 4-4-4 commands after reset.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
static int32_t QSPI_ResetMemory(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Send RESET ENABLE command in QPI mode (QUAD I/Os, 4-4-4) */
  if(N25Q128A_ResetEnable(&hqspi[Instance], N25Q128A_QPI_MODE) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }/* Send RESET memory command in QPI mode (QUAD I/Os, 4-4-4) */
  else if(N25Q128A_ResetMemory(&hqspi[Instance], N25Q128A_QPI_MODE) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  } /* Send RESET ENABLE command in QPI mode (DUAL I/Os, 2-2-2) */
  if(N25Q128A_ResetEnable(&hqspi[Instance], N25Q128A_DPI_MODE) != N25Q128A_OK)
  {
    ret =BSP_ERROR_COMPONENT_FAILURE;
  }/* Send RESET memory command in DPI mode (DUAL I/Os, 2-2-2) */
  else if(N25Q128A_ResetMemory(&hqspi[Instance], N25Q128A_DPI_MODE) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }/* Send RESET ENABLE command in SPI mode (1-1-1) */
  else if(N25Q128A_ResetEnable(&hqspi[Instance], BSP_QSPI_SPI_MODE) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }/* Send RESET memory command in SPI mode (1-1-1) */
  else if(N25Q128A_ResetMemory(&hqspi[Instance], BSP_QSPI_SPI_MODE) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Qspi_Ctx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;  /* After reset S/W setting to indirect access   */
    Qspi_Ctx[Instance].InterfaceMode = BSP_QSPI_SPI_MODE;     /* After reset H/W back to SPI mode by default  */
    Qspi_Ctx[Instance].TransferRate  = N25Q128A_STR_TRANSFER; /* After reset S/W setting to STR mode          */
  }

  HAL_Delay(1000);

  /* Return BSP status */
  return ret;
}

/**
  * @brief  This function configure the Output Driver Strength on memory side.
  *         ODS bit locate in Configuration Register[2:0]
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
static int32_t QSPI_SetODS(uint32_t Instance)
{
  int32_t ret;
  uint8_t reg;

  if(N25Q128A_ReadConfigurationRegister(&hqspi[Instance], BSP_QSPI_SPI_MODE, &reg) != N25Q128A_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Set Output Strength of the QSPI memory as FlashODS variable ohms */
    MODIFY_REG(reg, N25Q128A_CR_ODS, (((uint32_t)CONF_QSPI_ODS) << POSITION_VAL((uint32_t)N25Q128A_CR_ODS)));

    /* Enable write operations */
    if (N25Q128A_WriteEnable(&hqspi[Instance], BSP_QSPI_SPI_MODE) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Issue Write command */
    else if(N25Q128A_WriteStatusConfigurationRegister(&hqspi[Instance], BSP_QSPI_SPI_MODE, &reg, 1) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }/* Configure automatic polling mode to wait for end of program */
    else if (N25Q128A_AutoPollingMemReady(&hqspi[Instance], BSP_QSPI_SPI_MODE) != N25Q128A_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Switch between extended, dual and quad SPI mode
  * @param  Instance  QSPI instance  
  * @param  Mode actual flash mode
  * @param  NewMode Flash mode to switch to
  * @retval BSP status
  */
static int32_t QUADSPI_SwitchMode(uint32_t Instance, BSP_QSPI_Interface_t Mode, BSP_QSPI_Interface_t NewMode)
{
  int32_t ret = BSP_ERROR_NONE;
  uint8_t reg;
  BSP_QSPI_Interface_t old_mode, new_mode;

  switch(Mode)
  {
  case BSP_QSPI_SPI_1I2O_MODE :              /* 1-1-2 commands */
  case BSP_QSPI_SPI_2IO_MODE :               /* 1-2-2 commands */
  case BSP_QSPI_SPI_1I4O_MODE :              /* 1-1-4 program commands */
  case BSP_QSPI_SPI_4IO_MODE :               /* 1-4-4 program commands */
  case BSP_QSPI_SPI_MODE :                   /* 1-1-1 commands, Power on H/W default setting */
    old_mode = BSP_QSPI_SPI_MODE;
    break;
  default :
    old_mode = Mode;
    break;
  }

  switch(NewMode)
  {
  case BSP_QSPI_SPI_1I2O_MODE :              /* 1-1-2 commands */
  case BSP_QSPI_SPI_2IO_MODE :               /* 1-2-2 commands */
  case BSP_QSPI_SPI_1I4O_MODE :              /* 1-1-4 program commands */
  case BSP_QSPI_SPI_4IO_MODE :               /* 1-4-4 program commands */
  case BSP_QSPI_SPI_MODE :                   /* 1-1-1 commands, Power on H/W default setting */
    new_mode = BSP_QSPI_SPI_MODE;
    break;
  default :
    new_mode = NewMode;
    break;
  }

  if(old_mode != new_mode)
  {
    /* Read Enhanced Volatile Configuration register */
    if(N25Q128A_ReadEnhancedVolCfgRegister(&hqspi[Instance], Mode, &reg) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    } /* Enable write */
    else if(N25Q128A_WriteEnable(&hqspi[Instance], Mode) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      switch(NewMode)
      {
      case N25Q128A_DPI_MODE :                   /* 2-2-2 commands */
        MODIFY_REG(reg, 0xC0, N25Q128A_CR_QUAD);
        break;

      case N25Q128A_QPI_MODE :                   /* 4-4-4 commands */
        MODIFY_REG(reg, 0xC0, N25Q128A_CR_DUAL);
        break;

      case BSP_QSPI_SPI_1I2O_MODE :              /* 1-1-2 commands */
      case BSP_QSPI_SPI_2IO_MODE :               /* 1-2-2 commands */
      case BSP_QSPI_SPI_1I4O_MODE :              /* 1-1-4 program commands */
      case BSP_QSPI_SPI_4IO_MODE :               /* 1-4-4 program commands */
      case BSP_QSPI_SPI_MODE :                   /* 1-1-1 commands, Power on H/W default setting */
      default :
        MODIFY_REG(reg, 0xC0, N25Q128A_CR_QUAD | N25Q128A_CR_DUAL);
        break;
      }

      /* Write Enhanced Volatile Configuration register with new mode) */
      if(N25Q128A_WriteEnhancedVolCfgRegister(&hqspi[Instance], Mode, &reg, 1) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      } /* Read Configuration register */
      else if(N25Q128A_ReadConfigurationRegister(&hqspi[Instance], NewMode, &reg) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }/* Enable write operations */
      else if(N25Q128A_WriteEnable(&hqspi[Instance], NewMode) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        /* Write Volatile Configuration register (with new dummy cycles) */
        MODIFY_REG(reg, N25Q128A_VCR_NB_DUMMY, (N25Q128A_DUMMY_CYCLES_READ_QUAD << 4));
        if(N25Q128A_WriteConfigurationRegister(&hqspi[Instance], NewMode, &reg, 1) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }
    }
  }

  return ret;
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

