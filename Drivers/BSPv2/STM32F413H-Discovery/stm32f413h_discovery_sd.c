/**
  ******************************************************************************
  * @file    stm32f413h_discovery_sd.c
  * @author  MCD Application Team
  * @brief   This file includes the uSD card driver mounted on STM32F413H-DISCO
  *          evaluation boards.
  @verbatim
  How To use this driver:
  -----------------------
   - This driver is used to drive the micro SD external cards mounted on STM32F413H-DISCO
     evaluation board.
   - This driver does not need a specific component driver for the micro SD device
     to be included with.

  Driver description:
  ------------------
  + Initialization steps:
     o Initialize the micro SD card using the BSP_SD_Init() function. This
       function includes the MSP layer hardware resources initialization and the
       SDIO interface configuration to interface with the external micro SD. It
       also includes the micro SD initialization sequence for instance 0 or 1.

     o To check the SD card presence you can use the function BSP_SD_IsDetected() which
       returns the detection status for instance 0 or 1.

     o If SD presence detection interrupt mode is desired, you must configure the
       SD detection interrupt mode by calling the function BSP_SD_DetectITConfig().
       The interrupt is generated as an external interrupt whenever the micro SD card is
       plugged/unplugged in/from the evaluation board.
       The SD detection is managed by MFX, so the SD detection interrupt has to be
       treated by MFX_IRQOUT gpio pin IRQ handler. BSP_SD_DetectCallback() is called when
       SD is detected.

     o The function BSP_SD_GetCardInfo()are used to get the micro SD card information
       which is stored in the structure "HAL_SD_CardInfoTypedef".

  + Micro SD card operations
     o The micro SD card can be accessed with read/write block(s) operations once
       it is ready for access. The access can be performed whether
       using the polling mode by calling the functions BSP_SD_ReadBlocks()/BSP_SD_WriteBlocks(),
       using the interrupt mode by calling the functions BSP_SD_ReadBlocks_IT()/BSP_SD_WriteBlocks_IT(),
       or by DMA transfer using the functions BSP_SD_ReadBlocks_DMA()/BSP_SD_WriteBlocks_DMA().
       The access can be performed to instance 0 or 1.

     o The DMA transfer complete is used with interrupt mode. Once the SD transfer
       is complete.
       when instance 0 is used or BSP_SDIO_IRQHandler() when instance 1 is used.
     o The SD erase block(s) is performed using the functions BSP_SD_Erase() with specifying
       the number of blocks to erase.
     o The SD runtime status is returned when calling the function BSP_SD_GetCardState().

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
#include "stm32f413h_discovery_sd.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_SD SD
  * @{
  */


/** @defgroup STM32F413H_DISCOVERY_SD_Private_TypesDefinitions SD Private TypesDefinitions
  * @{
  */
#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
/* Is Msp Callbacks registred */
static uint32_t   IsMspCallbacksValid[SD_INSTANCES_NBR] = {0};
#endif
typedef void (* BSP_EXTI_LineCallback) (void);
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_SD_Exported_Variables SD Exported Variables
  * @{
  */
SD_HandleTypeDef   hsd_sdio[SD_INSTANCES_NBR];
EXTI_HandleTypeDef hsd_exti[SD_INSTANCES_NBR];
uint32_t PinDetect[SD_INSTANCES_NBR]  = {SD_DETECT_PIN};

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_SD_Private_Functions_Prototypes SD Private Functions Prototypes
  * @{
  */
static void SD_MspInit(SD_HandleTypeDef *hsd);
static void SD_MspDeInit(SD_HandleTypeDef *hsd);
#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
static void SD_AbortCallback(SD_HandleTypeDef *hsd);
static void SD_TxCpltCallback(SD_HandleTypeDef *hsd);
static void SD_RxCpltCallback(SD_HandleTypeDef *hsd);
#endif /* (USE_HAL_SD_REGISTER_CALLBACKS == 1) */
static void SD_EXTI_Callback(void);

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_SD_Exported_Functions SD Exported Functions
  * @{
  */
/**
  * @brief  Initializes the SD card device.
  * @param  Instance      SD Instance
  * @retval BSP status
  */
int32_t BSP_SD_Init(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if((uint32_t)BSP_SD_IsDetected(Instance) != SD_PRESENT)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
    /* Register the SD MSP Callbacks */
    if(IsMspCallbacksValid[Instance] == 0UL)
    {
      if(BSP_SD_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }
#else
    /* Msp SD initialization */
    SD_MspInit(&hsd_sdio[Instance]);
#endif /* USE_HAL_SD_REGISTER_CALLBACKS */
    if(ret == BSP_ERROR_NONE)
    {
      /* HAL SD initialization and Enable wide operation */
      if(MX_SDIO_SD_Init(&hsd_sdio[Instance]) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(HAL_SD_ConfigWideBusOperation(&hsd_sdio[Instance], SDIO_BUS_WIDE_4B) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
        /* Register SD TC, HT and Abort callbacks */
        if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_TX_COMPLETE_CB_ID, SD_TxCpltCallback) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_RX_COMPLETE_CB_ID, SD_RxCpltCallback) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else
        {
          if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_ABORT_CB_ID, SD_AbortCallback) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }
        }
#endif /* USE_HAL_SD_REGISTER_CALLBACKS */
      }
    }
  }
  return ret;
}

/**
  * @brief  DeInitializes the SD card device.
  * @param Instance      SD Instance
  * @retval SD status
  */
int32_t BSP_SD_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(HAL_SD_DeInit(&hsd_sdio[Instance]) != HAL_OK)/* HAL SD deinitialization */
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Msp SD deinitialization */
#if (USE_HAL_SD_REGISTER_CALLBACKS == 0)
    SD_MspDeInit(&hsd_sdio[Instance]);
#endif /* (USE_HAL_SD_REGISTER_CALLBACKS == 0) */
  }

  return ret;
}

/**
  * @brief  Initializes the SDIO peripheral.
  * @param  hsd  SD handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_SDIO_SD_Init(SD_HandleTypeDef *hsd)
{
  /* uSD device interface configuration */
  hsd->Instance                 = SDIO;
  hsd->Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  hsd->Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
  hsd->Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd->Init.BusWide             = SDIO_BUS_WIDE_1B;
  hsd->Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd->Init.ClockDiv            = SDIO_TRANSFER_CLK_DIV;

  /* HAL SD initialization */
  return HAL_SD_Init(hsd);
}


#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP SD Msp Callbacks
  * @param Instance      SD Instance
  * @retval BSP status
  */
int32_t BSP_SD_RegisterDefaultMspCallbacks(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_MSPINIT_CB_ID, SD_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_MSPDEINIT_CB_ID, SD_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1U;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP SD Msp Callback registering
  * @param Instance     SD Instance
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_SD_RegisterMspCallbacks(uint32_t Instance, BSP_SD_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_SD_RegisterCallback(&hsd_sdio[Instance], HAL_SD_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1U;
    }
  }

  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_SD_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Configures Interrupt mode for SD1 detection pin.
  * @param Instance      SD Instance
  * @retval Returns 0
  */
int32_t BSP_SD_DetectITConfig(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure;
  static const uint32_t SD_EXTI_LINE[SD_INSTANCES_NBR]   = {SD_DETECT_EXTI_LINE};
  static BSP_EXTI_LineCallback SdCallback[SD_INSTANCES_NBR] = {SD_EXTI_Callback};

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    SD_DETECT_GPIO_CLK_ENABLE();

    /* Configure Interrupt mode for SD detection pin */
    gpio_init_structure.Pin = PinDetect[Instance];
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init_structure.Mode = GPIO_MODE_IT_RISING_FALLING;
    HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &gpio_init_structure);

    if(HAL_EXTI_GetHandle(&hsd_exti[Instance], SD_EXTI_LINE[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_EXTI_RegisterCallback(&hsd_exti[Instance],  HAL_EXTI_COMMON_CB_ID, SdCallback[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      /* Enable and set Button EXTI Interrupt to the lowest priority */
      HAL_NVIC_SetPriority(SD_DETECT_EXTI_IRQn, BSP_SD_IT_PRIORITY, 0x00);
      HAL_NVIC_EnableIRQ(SD_DETECT_EXTI_IRQn);
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  BSP SD Callback.
  * @param  Instance SD Instance
  * @param  Status   Pin status
  * @retval None.
  */
__weak void BSP_SD_DetectCallback(uint32_t Instance, uint32_t Status)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
  UNUSED(Status);

  /* This function should be implemented by the user application.
     It is called into this driver when an event on JoyPin is triggered. */
}

/**
 * @brief  Detects if SD card is correctly plugged in the memory slot or not.
  * @param Instance  SD Instance
 * @retval Returns if SD is detected or not
 */
int32_t BSP_SD_IsDetected(uint32_t Instance)
{
  int32_t ret;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Check SD card detect pin */
    if (HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) == GPIO_PIN_SET)
    {
      ret = SD_NOT_PRESENT;
    }
    else
    {
      ret = SD_PRESENT;
    }
  }

  return(int32_t)ret;
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  Instance   SD Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be read
  * @param  BlocksNbr  Number of SD blocks to read
  * @retval BSP status
  */
int32_t BSP_SD_ReadBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t timeout = SD_READ_TIMEOUT*BlocksNbr;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_ReadBlocks(&hsd_sdio[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
  * @param  Instance   SD Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be written
  * @param  BlocksNbr  Number of SD blocks to write
  * @retval BSP status
  */
int32_t BSP_SD_WriteBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t timeout = SD_READ_TIMEOUT*BlocksNbr;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_WriteBlocks(&hsd_sdio[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  Instance   SD Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be read
  * @param  BlocksNbr  Number of SD blocks to read
  * @retval BSP status
  */
int32_t BSP_SD_ReadBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_ReadBlocks_DMA(&hsd_sdio[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  Instance   SD Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be written
  * @param  BlocksNbr  Number of SD blocks to write
  * @retval BSP status
  */
int32_t BSP_SD_WriteBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_WriteBlocks_DMA(&hsd_sdio[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  Instance   SD Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be read
  * @param  BlocksNbr  Number of SD blocks to read
  * @retval SD status
  */
int32_t BSP_SD_ReadBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_ReadBlocks_IT(&hsd_sdio[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  Instance   SD Instance
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be written
  * @param  BlocksNbr  Number of SD blocks to write
  * @retval SD status
  */
int32_t BSP_SD_WriteBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_WriteBlocks_IT(&hsd_sdio[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Erases the specified memory area of the given SD card.
  * @param  Instance   SD Instance
  * @param  BlockIdx   Block index from where data is to be
  * @param  BlocksNbr  Number of SD blocks to erase
  * @retval SD status
  */
int32_t BSP_SD_Erase(uint32_t Instance, uint32_t BlockIdx, uint32_t BlocksNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_Erase(&hsd_sdio[Instance], BlockIdx, BlockIdx + BlocksNbr) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Gets the current SD card data status.
  * @param  Instance  SD Instance
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
int32_t BSP_SD_GetCardState(uint32_t Instance)
{
  return (int32_t)((HAL_SD_GetCardState(&hsd_sdio[Instance]) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
  * @brief  Get SD information about specific SD card.
  * @param  Instance  SD Instance
  * @param  CardInfo  Pointer to HAL_SD_CardInfoTypedef structure
  * @retval None
  */
int32_t BSP_SD_GetCardInfo(uint32_t Instance, BSP_SD_CardInfo *CardInfo)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_SD_GetCardInfo(&hsd_sdio[Instance], CardInfo) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  This function handles EXTI_LINE_10 for SD1 interrupt request.
  * @param  Instance  SD Instance
  * @retval None
  */
void BSP_SD_DETECT_IRQHandler(uint32_t Instance)
{
  HAL_EXTI_IRQHandler(&hsd_exti[Instance]);
}

/**
  * @brief  This function handles SDMMC interrupt requests.
  * @param  Instance  SD Instance
  * @retval None
  */
void BSP_SD_IRQHandler(uint32_t Instance)
{
  HAL_SD_IRQHandler(&hsd_sdio[Instance]);
}

/**
  * @brief  Handles SDMMC DMA Rx transfer interrupt request.
  * @param  Instance  SD Instance
  * @retval None
  */
void BSP_SD_DMA_RX_IRQHandler(uint32_t Instance)
{
  HAL_DMA_IRQHandler(hsd_sdio[Instance].hdmarx);
}

/**
  * @brief  Handles SDMMC DMA Tx transfer interrupt request.
  * @param  Instance  SD Instance
  * @retval None
  */
void BSP_SD_DMA_TX_IRQHandler(uint32_t Instance)
{
  HAL_DMA_IRQHandler(hsd_sdio[Instance].hdmatx);
}

#if (USE_HAL_SD_REGISTER_CALLBACKS == 0) || !defined (USE_HAL_SD_REGISTER_CALLBACKS)
/**
  * @brief SD Abort callbacks
  * @param hsd  SD handle
  * @retval None
  */
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback((hsd == &hsd_sdio[0]) ? 0UL : 1UL);
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hsd  SD handle
  * @retval None
  */
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback((hsd == &hsd_sdio[0]) ? 0UL : 1UL);
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd  SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback((hsd == &hsd_sdio[0]) ? 0UL : 1UL);
}
#endif /* (USE_HAL_SD_REGISTER_CALLBACKS == 1) */

/**
  * @brief BSP SD Abort callbacks
  * @param  Instance     SD Instance
  * @retval None
  */
__weak void BSP_SD_AbortCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
}

/**
  * @brief BSP Tx Transfer completed callbacks
  * @param  Instance     SD Instance
  * @retval None
  */
__weak void BSP_SD_WriteCpltCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
}

/**
  * @brief BSP Rx Transfer completed callbacks
  * @param  Instance     SD Instance
  * @retval None
  */
__weak void BSP_SD_ReadCpltCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
}
/**
  * @}
  */

/*******************************************************************************
                            Static Functions
*******************************************************************************/

/** @defgroup STM32F413H_DISCOVERY_SD_Private_Functions SD Private Functions
  * @{
  */

#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
/**
  * @brief SD Abort callbacks
  * @param hsd  SD handle
  * @retval None
  */
static void SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback((hsd == &hsd_sdio[0]) ? 0UL : 1UL);
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hsd  SD handle
  * @retval None
  */
static void SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback((hsd == &hsd_sdio[0]) ? 0UL : 1UL);
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd  SD handle
  * @retval None
  */
static void SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback((hsd == &hsd_sdio[0]) ? 0UL : 1UL);
}
#endif

/**
  * @brief  SD EXTI line detection callbacks.
  * @retval None
  */
static void SD_EXTI_Callback(void)
{
  uint32_t sd_status = SD_PRESENT;

  /* Check SD card detect pin */
  if (HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) == GPIO_PIN_SET)
  {
    sd_status = SD_NOT_PRESENT;
  }
  else
  {
    sd_status = SD_PRESENT;
  }

  BSP_SD_DetectCallback(0, sd_status);
}

/**
  * @brief  Initializes the SD MSP.
  * @param  hsd  SD handle
  * @retval None
  */
static void SD_MspInit(SD_HandleTypeDef *hsd)
{
  static DMA_HandleTypeDef dma_rx_handle;
  static DMA_HandleTypeDef dma_tx_handle;
  GPIO_InitTypeDef gpio_init_structure;

  if(hsd == &hsd_sdio[0])
  {
    /* Enable SDIO clock */
    __HAL_RCC_SDIO_CLK_ENABLE();

    /* Enable DMA2 clocks */
    SD_DMAx_TxRx_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Common GPIO configuration */
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_PULLUP;
    gpio_init_structure.Speed     = GPIO_SPEED_HIGH;
    gpio_init_structure.Alternate = GPIO_AF12_SDIO;

    /* GPIOC configuration: SD_D[0..3] and SD_clk */
    gpio_init_structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;

    HAL_GPIO_Init(GPIOC, &gpio_init_structure);

    /* GPIOA configuration: SD cmd */
    gpio_init_structure.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOA, &gpio_init_structure);

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriority(SDIO_IRQn, BSP_SD_IT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);


    dma_rx_handle.Init.Channel             = SD_SDIO_DMAx_Rx_CHANNEL;
    dma_rx_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    dma_rx_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    dma_rx_handle.Init.MemInc              = DMA_MINC_ENABLE;
    dma_rx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dma_rx_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    dma_rx_handle.Init.Mode                = DMA_PFCTRL;
    dma_rx_handle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    dma_rx_handle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    dma_rx_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    dma_rx_handle.Init.MemBurst            = DMA_MBURST_INC16;
    dma_rx_handle.Init.PeriphBurst         = DMA_PBURST_INC4;
    dma_rx_handle.Instance                 = SD_SDIO_DMAx_Rx_STREAM;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hsd, hdmarx, dma_rx_handle);

    /* Deinitialize the stream for new transfer */
    (void)HAL_DMA_DeInit(&dma_rx_handle);

    /* Configure the DMA stream */
    (void)HAL_DMA_Init(&dma_rx_handle);

    dma_tx_handle.Init.Channel             = SD_SDIO_DMAx_Tx_CHANNEL;
    dma_tx_handle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    dma_tx_handle.Init.PeriphInc           = DMA_PINC_DISABLE;
    dma_tx_handle.Init.MemInc              = DMA_MINC_ENABLE;
    dma_tx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dma_tx_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    dma_tx_handle.Init.Mode                = DMA_PFCTRL;
    dma_tx_handle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    dma_tx_handle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    dma_tx_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    dma_tx_handle.Init.MemBurst            = DMA_MBURST_INC16;
    dma_tx_handle.Init.PeriphBurst         = DMA_PBURST_INC4;
    dma_tx_handle.Instance                 = SD_SDIO_DMAx_Tx_STREAM;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hsd, hdmatx, dma_tx_handle);

    /* Deinitialize the stream for new transfer */
    (void)HAL_DMA_DeInit(&dma_tx_handle);

    /* Configure the DMA stream */
    (void)HAL_DMA_Init(&dma_tx_handle);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SD_SDIO_DMAx_Rx_IRQn, BSP_SD_RX_IT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SD_SDIO_DMAx_Rx_IRQn);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SD_SDIO_DMAx_Tx_IRQn, BSP_SD_TX_IT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SD_SDIO_DMAx_Tx_IRQn);
  }
}

/**
  * @brief  DeInitializes the SD MSP.
  * @param  hsd  SD handle
  * @retval None
  */
static void SD_MspDeInit(SD_HandleTypeDef *hsd)
{
  static DMA_HandleTypeDef dma_rx_handle;
  static DMA_HandleTypeDef dma_tx_handle;
  GPIO_InitTypeDef gpio_init_structure;

  if(hsd == &hsd_sdio[0])
  {    /* Disable NVIC for DMA transfer complete interrupts */
    HAL_NVIC_DisableIRQ(SD_SDIO_DMAx_Rx_IRQn);
    HAL_NVIC_DisableIRQ(SD_SDIO_DMAx_Tx_IRQn);

    /* Deinitialize the stream for new transfer */
    dma_rx_handle.Instance = SD_SDIO_DMAx_Rx_STREAM;
    (void)HAL_DMA_DeInit(&dma_rx_handle);

    /* Deinitialize the stream for new transfer */
    dma_tx_handle.Instance = SD_SDIO_DMAx_Tx_STREAM;
    (void)HAL_DMA_DeInit(&dma_tx_handle);

    /* Disable NVIC for SDIO interrupts */
    HAL_NVIC_DisableIRQ(SDIO_IRQn);

    /* Disable SD peripheral */
    __HAL_SD_DISABLE(hsd);

    /* GPIOC configuration: SD_D[0..3] and SD_clk */
    gpio_init_structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_DeInit(GPIOC, gpio_init_structure.Pin);
    /* GPIOA configuration: SD cmd */
    gpio_init_structure.Pin = GPIO_PIN_6;
    HAL_GPIO_DeInit(GPIOA, gpio_init_structure.Pin);

    /* Disable SDIO clock */
    __HAL_RCC_SDIO_CLK_DISABLE();
  }
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
