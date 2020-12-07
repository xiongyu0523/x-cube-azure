/**
  ******************************************************************************
  * @file    stm32f413h_discovery_psram.c
  * @author  MCD Application Team
  * @brief   This file includes the SRAM driver for the IS61WV51216BLL-10MLI memory
  *          device mounted on STM32F413H-DISCOVERY board.
  ******************************************************************************
  @verbatim
  How To use this driver:
  -----------------------
   - This driver is used to drive the IS61WV51216BLL-10MLI SRAM external memory mounted
     on STM32F413H-DISCOVERY board.
   - This driver does not need a specific component driver for the SRAM device
     to be included with.

  Driver description:
  ------------------
  + Initialization steps:
     o Initialize the SRAM external memory using the BSP_PSRAM_Init() function. This
       function includes the MSP layer hardware resources initialization and the
       FMC controller configuration to interface with the external SRAM memory.
       Note that BSP_PSRAM_Init is calling MX_SRAM_BANK1_Init which is a weak function
       that can be updated by user.

  + SRAM read/write operations
     o SRAM external memory can be accessed with read/write operations once it is
       initialized.
       Read/write operation can be performed with AHB access using the HAL functions
       HAL_SRAM_Read_Xb()/HAL_SRAM_Write_Xb() (with X=8,16 or 32),
       or by DMA transfer using the functions
       HAL_SRAM_Read_DMA()/HAL_SRAM_Write_DMA().

     o The AHB access is performed with 16-bit width transaction, the DMA transfer
       configuration is fixed at single (no burst) halfword transfer
       (see the SRAM_MspInit() static function).
     o User can implement his own BSP functions for read/write access with his desired
       configurations.

  Note:
  --------
    Regarding the "Instance" parameter, needed for all functions, it is used to select
    an SRAM instance. On the STM32F413H_DISCOVERY board, there's one instance. Then, this
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
#include "stm32f413h_discovery_psram.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM PSRAM
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Exported_Variables PSRAM Exported Variables
  * @{
  */
SRAM_HandleTypeDef hpsram[PSRAM_INSTANCES_NBR];
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Private_Variables PSRAM Private Variables
  * @{
  */
#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
static uint32_t IsMspCallbacksValid[PSRAM_INSTANCES_NBR] = {0};
#endif
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Private_Functions_Prototypes PSRAM Private Functions Prototypes
  * @{
  */
static void PSRAM_MspInit(SRAM_HandleTypeDef  *hsram);
static void PSRAM_MspDeInit(SRAM_HandleTypeDef  *hsram);
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Exported_Functions PSRAM Exported Functions
  * @{
  */

/**
  * @brief  Initializes the PSRAM memory.
  * @param  Instance PSRAM instance
  * @retval BSP status
  */
int32_t BSP_PSRAM_Init(uint32_t Instance)
{
  int32_t ret;

  if(Instance >= PSRAM_INSTANCES_NBR)
  {
    ret =  BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
    /* Register the SRAM MSP Callbacks */
    if(IsMspCallbacksValid == 0)
    {
      if(BSP_PSRAM_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_PERIPH_FAILURE;
      }
    }
#else
    /* Msp SRAM initialization */
    PSRAM_MspInit(&hpsram[Instance]);
#endif /* USE_HAL_SRAM_REGISTER_CALLBACKS */

    /* __weak function can be rewritten by the application */
    if(MX_SRAM_BANK1_Init(&hpsram[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_NO_INIT;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  return ret;
}

/**
  * @brief  DeInitializes the PSRAM memory.
  * @param  Instance PSRAM instance
  * @retval BSP status
  */
int32_t BSP_PSRAM_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= PSRAM_INSTANCES_NBR)
  {
    ret =  BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* SRAM device de-initialization */
    hpsram[Instance].Instance = FMC_NORSRAM_DEVICE;
    hpsram[Instance].Extended = FMC_NORSRAM_EXTENDED_DEVICE;

#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 0)
    /* SRAM controller de-initialization */
    PSRAM_MspDeInit(&hpsram[Instance]);
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 0) */
    if(HAL_SRAM_DeInit(&hpsram[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Initializes the SRAM periperal.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_SRAM_BANK1_Init(SRAM_HandleTypeDef *hsram)
{
  static FMC_NORSRAM_TimingTypeDef SramTiming;

  /* SRAM device configuration */
  hsram->Instance = FMC_NORSRAM_DEVICE;
  hsram->Extended = FMC_NORSRAM_EXTENDED_DEVICE;

  /* SRAM device configuration */
  hsram->Init.NSBank             = FSMC_NORSRAM_BANK1;
  hsram->Init.DataAddressMux     = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram->Init.MemoryType         = FSMC_MEMORY_TYPE_SRAM;
  hsram->Init.MemoryDataWidth    = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram->Init.BurstAccessMode    = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram->Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram->Init.WaitSignalActive   = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram->Init.WriteOperation     = FSMC_WRITE_OPERATION_ENABLE;
  hsram->Init.WaitSignal         = FSMC_WAIT_SIGNAL_DISABLE;
  hsram->Init.ExtendedMode       = FSMC_EXTENDED_MODE_DISABLE;
  hsram->Init.AsynchronousWait   = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram->Init.WriteBurst         = FSMC_WRITE_BURST_DISABLE;
  hsram->Init.ContinuousClock    = FSMC_CONTINUOUS_CLOCK_SYNC_ONLY;

  /* PSRAM device configuration */
  /* Timing configuration derived from system clock (up to 100Mhz)*/
  SramTiming.AddressSetupTime      = 3;
  SramTiming.AddressHoldTime       = 1;
  SramTiming.DataSetupTime         = 4;
  SramTiming.BusTurnAroundDuration = 1;
  SramTiming.CLKDivision           = 2;
  SramTiming.DataLatency           = 2;
  SramTiming.AccessMode            = FSMC_ACCESS_MODE_A;

  /* SRAM controller initialization */
  if(HAL_SRAM_Init(hsram, &SramTiming, NULL) != HAL_OK)
  {
    return  HAL_ERROR;
  }
  return HAL_OK;
}

#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP SRAM Msp Callbacks
  * @param Instance      SRAM Instance
  * @retval BSP status
  */
int32_t BSP_PSRAM_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= PSRAM_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if((HAL_SRAM_RegisterCallback(&hpsram[Instance], HAL_SRAM_MSPINIT_CB_ID, PSRAM_MspInit) != HAL_OK) ||
       (HAL_SRAM_RegisterCallback(&hpsram[Instance], HAL_SRAM_MSPDEINIT_CB_ID, PSRAM_MspDeInit) != HAL_OK))
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP PSRAM Msp Callback registering
  * @param Instance     PSRAM Instance
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_PSRAM_RegisterMspCallbacks(uint32_t Instance, BSP_PSRAM_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >= PSRAM_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if((HAL_SRAM_RegisterCallback(&hpsram[Instance], HAL_SRAM_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK) ||
       (HAL_SRAM_RegisterCallback(&hpsram[Instance], HAL_SRAM_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK))
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 1) */

/**
  * @brief  This function handles PSRAM DMA interrupt request.
  * @retval None
  */
void BSP_PSRAM_DMA_IRQHandler(uint32_t Instance)
{
  HAL_DMA_IRQHandler(hpsram[Instance].hdma);
}

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_PSRAM_Private_Functions Private Functions
  * @{
  */
/**
  * @brief  Initializes PSRAM MSP.
  * @param  hsram  SRAM handle
  * @retval None
  */
static void PSRAM_MspInit(SRAM_HandleTypeDef  *hsram)
{
  static DMA_HandleTypeDef dma_handle;
  GPIO_InitTypeDef gpio_init_structure;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsram);

  /* Enable FMC clock */
  __HAL_RCC_FSMC_CLK_ENABLE();

  /* Enable chosen DMAx clock */
  PSRAM_DMAx_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* Common GPIO configuration */
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull      = GPIO_PULLUP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init_structure.Alternate = GPIO_AF12_FSMC;

  /* GPIOD configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7      |\
                              GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12   |\
                              GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);

  /* GPIOE configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7  | GPIO_PIN_8 | GPIO_PIN_9     |\
                              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &gpio_init_structure);

  /* GPIOF configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4       |\
                              GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOF, &gpio_init_structure);

  /* GPIOG configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4       |\
                              GPIO_PIN_5;
  HAL_GPIO_Init(GPIOG, &gpio_init_structure);

  /* Configure common DMA parameters */
  dma_handle.Init.Channel             = PSRAM_DMAx_CHANNEL;
  dma_handle.Init.Direction           = DMA_MEMORY_TO_MEMORY;
  dma_handle.Init.PeriphInc           = DMA_PINC_ENABLE;
  dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
  dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  dma_handle.Init.Mode                = DMA_NORMAL;
  dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
  dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  dma_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  dma_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
  dma_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

  dma_handle.Instance = PSRAM_DMAx_STREAM;

   /* Associate the DMA handle */
  __HAL_LINKDMA(hsram, hdma, dma_handle);

  /* Deinitialize the Stream for new transfer */
  (void)HAL_DMA_DeInit(&dma_handle);

  /* Configure the DMA Stream */
  (void)HAL_DMA_Init(&dma_handle);

  /* NVIC configuration for DMA transfer complete interrupt */
  HAL_NVIC_SetPriority(PSRAM_DMAx_IRQn, BSP_PSRAM_IT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(PSRAM_DMAx_IRQn);
}


/**
  * @brief  DeInitializes PSRAM MSP.
  * @param  hsram  SRAM handle
  * @retval None
  */
static void PSRAM_MspDeInit(SRAM_HandleTypeDef  *hsram)
{
  GPIO_InitTypeDef gpio_init_structure;
  static DMA_HandleTypeDef dma_handle;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsram);

  /* GPIOD configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7      |\
                              GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12   |\
                              GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_DeInit(GPIOD, gpio_init_structure.Pin);

  /* GPIOE configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7  | GPIO_PIN_8 | GPIO_PIN_9     |\
                              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;
  HAL_GPIO_DeInit(GPIOE, gpio_init_structure.Pin);

  /* GPIOF configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4       |\
                              GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_DeInit(GPIOF, gpio_init_structure.Pin);

  /* GPIOG configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4       |\
                              GPIO_PIN_5;
  HAL_GPIO_DeInit(GPIOG, gpio_init_structure.Pin);

  /* Disable NVIC configuration for DMA interrupt */
  HAL_NVIC_DisableIRQ(PSRAM_DMAx_IRQn);

  /* Deinitialize the stream for new transfer */
  dma_handle.Instance = PSRAM_DMAx_STREAM;
  (void)HAL_DMA_DeInit(&dma_handle);

  /* Disable chosen DMAx clock */
  PSRAM_DMAx_CLK_DISABLE();

  /* Disable FMC clock */
  __HAL_RCC_FSMC_CLK_DISABLE();
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
