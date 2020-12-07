/**
  ******************************************************************************
  * @file    stm32f413h_discovery_lcd.c
  * @author  MCD Application Team
  * @brief   This file includes the driver for Liquid Crystal Display (LCD) module
  *          mounted on STM32F413H-DISCOVERY board.
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

/* File Info : -----------------------------------------------------------------
                                   User NOTES
1. How To use this driver:
--------------------------
   - This driver is used to drive indirectly an LCD TFT.
   - This driver supports the LS016B8UY LCD.
   - The LS016B8UY component driver MUST be included with this driver.

2. Driver description:
---------------------
  + Initialization steps:
     o Initialize the LCD using the BSP_LCD_Init() function.

  + Display on LCD
     o Clear the hole LCD using BSP_LCD_Clear() function or only one specified string
       line using the BSP_LCD_ClearStringLine() function.
     o Display a character on the specified line and column using the BSP_LCD_DisplayChar()
       function or a complete string line using the BSP_LCD_DisplayStringAtLine() function.
     o Display a string line on the specified position (x,y in pixel) and align mode
       using the BSP_LCD_DisplayStringAtLine() function.
     o Draw and fill a basic shapes (dot, line, rectangle, circle, ellipse, .. bitmap)
       on LCD using the available set of functions.

------------------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_lcd.h"
#include "stm32f413h_discovery_bus.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_LCD LCD
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Exported_Variables LCD Exported Variables
  * @{
  */
void               *Lcd_CompObj[LCD_INSTANCES_NBR] = {NULL};
LCD_Drv_t          *Lcd_Drv[LCD_INSTANCES_NBR] = {NULL};
SRAM_HandleTypeDef hlcd_sram[LCD_INSTANCES_NBR];
BSP_LCD_Ctx_t      Lcd_Ctx[LCD_INSTANCES_NBR];
/**
  * @}
  */
/** @defgroup STM32F413H_DISCOVERY_LCD_Private_Types LCD Private Types
  * @{
  */
const GUI_Drv_t LCD_Driver =
{
  BSP_LCD_DrawBitmap,
  BSP_LCD_FillRGBRect,
  BSP_LCD_DrawHLine,
  BSP_LCD_DrawVLine,
  BSP_LCD_FillRect,
  BSP_LCD_ReadPixel,
  BSP_LCD_WritePixel,
  BSP_LCD_GetXSize,
  BSP_LCD_GetYSize,
  NULL,
  BSP_LCD_GetPixelFormat
};

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Private_Defines LCD Private Defines
  * @{
  */
#define LCD_REGISTER_ADDR  0x68000000UL
#define LCD_DATA_ADDR     (LCD_REGISTER_ADDR | 0x00000002UL)

#define LCD_FMC_ADDRESS  ((uint16_t)1)
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Private_FunctionPrototypes LCD Private Functions Prototypes
  * @{
  */
static int32_t ST7789H2_Probe(uint32_t Orientation);

static int32_t LCD_FMC_Init(void);
static int32_t LCD_FMC_DeInit(void);
static int32_t LCD_FMC_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
static int32_t LCD_FMC_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
static int32_t LCD_FMC_Send(uint8_t *pData, uint16_t Length);
static int32_t LCD_FMC_GetTick(void);

static void LCD_InitSequance(void);
static void LCD_DeInitSequence(void);
static void FMC_BANK3_MspInit(SRAM_HandleTypeDef *hsram);
static void FMC_BANK3_MspDeInit(SRAM_HandleTypeDef *hsram);

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Exported_Functions LCD Exported Functions
  * @{
  */

/**
  * @brief  Initializes the LCD with a given orientation.
  * @param  Instance    LCD Instance
  * @param  Orientation LCD_ORIENTATION_PORTRAIT, LCD_ORIENTATION_PORTRAIT_ROT180
  *         LCD_ORIENTATION_LANDSCAPE or LCD_ORIENTATION_LANDSCAPE_ROT180
  * @retval BSP status
  */
int32_t BSP_LCD_Init(uint32_t Instance, uint32_t Orientation)
{
  int32_t ret = BSP_ERROR_NONE;

  if((Orientation > LCD_ORIENTATION_LANDSCAPE_ROT180) || (Instance >= LCD_INSTANCES_NBR))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Lcd_Ctx[Instance].PixelFormat = LCD_PIXEL_FORMAT_RGB565;
    Lcd_Ctx[Instance].XSize       = LCD_DEFAULT_WIDTH;
    Lcd_Ctx[Instance].YSize       = LCD_DEFAULT_HEIGHT;

    /* Initialize LCD special pins GPIOs */
    LCD_InitSequance();

    if(ST7789H2_Probe(Orientation) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_UNKNOWN_COMPONENT;
    }
  }

  return ret;
}

/**
  * @brief  DeInitializes the LCD.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    LCD_DeInitSequence();

#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 0)
    /* SRAM controller de-initialization */
    FMC_BANK3_MspDeInit(&hlcd_sram[Instance]);
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 0) */

    /* De-Init the SRAM */
    if (HAL_SRAM_DeInit(&hlcd_sram[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Initializes LCD IOs.
  * @param  hsram SRAM handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_FSMC_BANK3_Init(SRAM_HandleTypeDef *hsram)
{
  FSMC_NORSRAM_TimingTypeDef sram_timing;

  /*** Configure the SRAM Bank 3 ***/
  /* Configure IPs */
  hsram->Instance  = FSMC_NORSRAM_DEVICE;
  hsram->Extended  = FSMC_NORSRAM_EXTENDED_DEVICE;

  /* Timing config */
  sram_timing.AddressSetupTime      = 3;
  sram_timing.AddressHoldTime       = 1;
  sram_timing.DataSetupTime         = 4;
  sram_timing.BusTurnAroundDuration = 1;
  sram_timing.CLKDivision           = 2;
  sram_timing.DataLatency           = 2;
  sram_timing.AccessMode            = FSMC_ACCESS_MODE_A;

  hsram->Init.NSBank                = FSMC_NORSRAM_BANK3;
  hsram->Init.DataAddressMux        = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram->Init.MemoryType            = FSMC_MEMORY_TYPE_SRAM;
  hsram->Init.MemoryDataWidth       = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram->Init.BurstAccessMode       = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram->Init.WaitSignalPolarity    = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram->Init.WrapMode              = FSMC_WRAP_MODE_DISABLE;
  hsram->Init.WaitSignalActive      = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram->Init.WriteOperation        = FSMC_WRITE_OPERATION_ENABLE;
  hsram->Init.WaitSignal            = FSMC_WAIT_SIGNAL_DISABLE;
  hsram->Init.ExtendedMode          = FSMC_EXTENDED_MODE_ENABLE;
  hsram->Init.AsynchronousWait      = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram->Init.WriteBurst            = FSMC_WRITE_BURST_DISABLE;
  hsram->Init.WriteFifo             = FSMC_WRITE_FIFO_DISABLE;
  hsram->Init.PageSize              = FSMC_PAGE_SIZE_NONE;
  hsram->Init.ContinuousClock       = FSMC_CONTINUOUS_CLOCK_SYNC_ONLY;

  return HAL_SRAM_Init(hsram, &sram_timing, &sram_timing);
}

/**
  * @brief  Gets the LCD Active LCD Pixel Format.
  * @param  Instance    LCD Instance
  * @param  PixelFormat Active LCD Pixel Format
  * @retval BSP status
  */
int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Only RGB565 format is supported */
    *PixelFormat = LCD_PIXEL_FORMAT_RGB565;
  }

  return ret;
}


/**
  * @brief  Gets the LCD X size.
  * @param  Instance  LCD Instance
  * @param  XSize     LCD width
  * @retval BSP status
  */
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Drv[Instance]->GetXSize(Lcd_CompObj[Instance], XSize) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Gets the LCD Y size.
  * @param  Instance  LCD Instance
  * @param  YSize     LCD Height
  * @retval BSP status
  */
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Drv[Instance]->GetYSize(Lcd_CompObj[Instance], YSize) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Reads an LCD pixel.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color RGB pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->GetPixel != NULL)
  {
    if(Lcd_Drv[Instance]->GetPixel(Lcd_CompObj[Instance], Xpos, Ypos, Color) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws a pixel on LCD.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color Pixel color in RGB mode (5-6-5)
  * @retval BSP status
  */
int32_t BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->SetPixel != NULL)
  {
    if(Lcd_Drv[Instance]->SetPixel(Lcd_CompObj[Instance], Xpos, Ypos, Color) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws an horizontal line.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Length Line length
  * @param  Color Pixel color in RGB mode (5-6-5)  
  * @retval BSP status
  */
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->DrawHLine != NULL)
  {
    if(Lcd_Drv[Instance]->DrawHLine(Lcd_CompObj[Instance], Xpos, Ypos, Length, Color) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws a vertical line.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Length Line length
  * @param  Color Pixel color in RGB mode (5-6-5)
  * @retval BSP status
  */
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->DrawVLine != NULL)
  {
    if(Lcd_Drv[Instance]->DrawVLine(Lcd_CompObj[Instance], Xpos, Ypos, Length, Color) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws a bitmap picture (16 bpp).
  * @param  Instance    LCD Instance
  * @param  Xpos Bmp X position in the LCD
  * @param  Ypos Bmp Y position in the LCD
  * @param  pBmp Pointer to Bmp picture address.
  * @retval BSP status
  */
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->DrawBitmap != NULL)
  {
    if(Lcd_Drv[Instance]->DrawBitmap(Lcd_CompObj[Instance], Xpos, Ypos, pBmp) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draw a horizontal line on LCD.
  * @param  Instance LCD Instance.
  * @param  Xpos X position.
  * @param  Ypos Y position.
  * @param  pData Pointer to RGB line data
  * @param  Width Rectangle width.
  * @param  Height Rectangle Height.
  * @retval BSP status.
  */
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->FillRect != NULL)
  {
    if(Lcd_Drv[Instance]->FillRGBRect(Lcd_CompObj[Instance], Xpos, Ypos, pData, Width, Height) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Draws a full rectangle.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width Rectangle width
  * @param  Height Rectangle height
  * @param  Color Pixel color in RGB mode (5-6-5)  
  * @retval BSP status
  */
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->FillRect != NULL)
  {
    if(Lcd_Drv[Instance]->FillRect(Lcd_CompObj[Instance], Xpos, Ypos, Width, Height, Color) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Enables the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOn(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->DisplayOn != NULL)
  {
    if(Lcd_Drv[Instance]->DisplayOn(Lcd_CompObj[Instance]) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Disables the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOff(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->DisplayOff != NULL)
  {
    if(Lcd_Drv[Instance]->DisplayOff(Lcd_CompObj[Instance]) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}


/**
  * @brief  Set the brightness value
  * @param  Instance    LCD Instance
  * @param  Brightness [00: Min (black), 100 Max]
  * @retval BSP status
  */
int32_t BSP_LCD_SetBrightness(uint32_t Instance, uint32_t Brightness)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->SetBrightness != NULL)
  {
    if(Lcd_Drv[Instance]->SetBrightness(Lcd_CompObj[Instance], Brightness) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

/**
  * @brief  Get the brightness value
  * @param  Instance    LCD Instance
  * @param  Brightness [00: Min (black), 100 Max]
  * @retval BSP status
  */
int32_t BSP_LCD_GetBrightness(uint32_t Instance, uint32_t *Brightness)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv[Instance]->GetBrightness != NULL)
  {
    if(Lcd_Drv[Instance]->GetBrightness(Lcd_CompObj[Instance], Brightness) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }

  return ret;
}

#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP LCD SRAM Msp Callbacks
  * @param Instance      LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance != 0UL)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if((HAL_SRAM_RegisterCallback(&hlcd_sram[Instance], HAL_SRAM_MSPINIT_CB_ID, FMC_BANK3_MspInit) != HAL_OK) ||
       (HAL_SRAM_RegisterCallback(&hlcd_sram[Instance], HAL_SRAM_MSPDEINIT_CB_ID, FMC_BANK3_MspDeInit) != HAL_OK))
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      Lcd_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP LCD SRAM Msp Callback registering
  * @param Instance     LCD Instance
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_LCD_RegisterMspCallbacks(uint32_t Instance, BSP_LCD_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance != 0UL)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if((HAL_SRAM_RegisterCallback(&hlcd_sram[Instance], HAL_SRAM_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK) ||
       (HAL_SRAM_RegisterCallback(&hlcd_sram[Instance], HAL_SRAM_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK))
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      Lcd_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 1) */
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Private_Functions LCD Private Functions
  * @{
  */
/**
  * @brief  Initializes the LCD GPIO special pins MSP.
  * @retval None
  */
static void LCD_InitSequance(void)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Enable GPIOs clock */
  LCD_RESET_GPIO_CLK_ENABLE();
  LCD_BL_CTRL_GPIO_CLK_ENABLE();

  /* LCD_RESET GPIO configuration */
  gpio_init_structure.Pin       = LCD_RESET_PIN;     /* LCD_RESET pin has to be manually controlled */
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(LCD_RESET_GPIO_PORT, &gpio_init_structure);

  /* Apply hardware reset according to procedure indicated in FRD154BP2901 documentation */
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);   /* Reset signal asserted during 5ms  */
  HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);
  HAL_Delay(120);  /* Reset signal released during 10ms */

  /* LCD_BL_CTRL GPIO configuration */
  gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;   /* LCD_BL_CTRL pin has to be manually controlled */
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);

  /* Backlight control signal assertion */
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
}

/**
  * @brief  DeInitializes LCD GPIO special pins MSP.
  * @retval None
  */
static void LCD_DeInitSequence(void)
{
  GPIO_InitTypeDef  gpio_init_structure;

  /* LCD_RESET GPIO deactivation */
  gpio_init_structure.Pin       = LCD_RESET_PIN;
  HAL_GPIO_DeInit(LCD_RESET_GPIO_PORT, gpio_init_structure.Pin);

  /* LCD_TE GPIO deactivation */
  gpio_init_structure.Pin       = LCD_TE_PIN;
  HAL_GPIO_DeInit(LCD_TE_GPIO_PORT, gpio_init_structure.Pin);

  /* LCD_BL_CTRL GPIO deactivation */
  gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
  HAL_GPIO_DeInit(LCD_BL_CTRL_GPIO_PORT, gpio_init_structure.Pin);
}

/**
  * @brief  Probe the ST7789H2 LCD driver.
  * @param  Orientation LCD_ORIENTATION_PORTRAIT, LCD_ORIENTATION_LANDSCAPE,
  *                     LCD_ORIENTATION_PORTRAIT_ROT180 or LCD_ORIENTATION_LANDSCAPE_ROT180.
  * @retval BSP status.
  */
static int32_t ST7789H2_Probe(uint32_t Orientation)
{
  int32_t                  status;
  ST7789H2_IO_t            IOCtx;
  uint32_t                 st7789h2_id;
  static ST7789H2_Object_t ST7789H2Obj;
  uint32_t                 lcd_orientation;

  /* Configure the LCD driver */
  IOCtx.Address     = LCD_FMC_ADDRESS;
  IOCtx.Init        = LCD_FMC_Init;
  IOCtx.DeInit      = LCD_FMC_DeInit;
  IOCtx.ReadReg     = LCD_FMC_ReadReg16;
  IOCtx.WriteReg    = LCD_FMC_WriteReg16;
  IOCtx.SendData    = LCD_FMC_Send;
  IOCtx.GetTick     = LCD_FMC_GetTick;

  if (ST7789H2_RegisterBusIO(&ST7789H2Obj, &IOCtx) != ST7789H2_OK)
  {
    status = BSP_ERROR_BUS_FAILURE;
  }
  else if (ST7789H2_ReadID(&ST7789H2Obj, &st7789h2_id) != ST7789H2_OK)
  {
    status = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if (st7789h2_id != ST7789H2_ID)
  {
    status = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    Lcd_CompObj[0] = &ST7789H2Obj;
    Lcd_Drv[0] = (LCD_Drv_t *) &ST7789H2_Driver;
    if (Orientation == LCD_ORIENTATION_PORTRAIT)
    {
      lcd_orientation = ST7789H2_ORIENTATION_LANDSCAPE;
    }
    else if (Orientation == LCD_ORIENTATION_LANDSCAPE)
    {
      lcd_orientation = ST7789H2_ORIENTATION_PORTRAIT_ROT180;
    }
    else if (Orientation == LCD_ORIENTATION_PORTRAIT_ROT180)
    {
      lcd_orientation = ST7789H2_ORIENTATION_LANDSCAPE_ROT180;
    }
    else
    {
      lcd_orientation = ST7789H2_ORIENTATION_PORTRAIT;
    }
    if (Lcd_Drv[0]->Init(Lcd_CompObj[0], ST7789H2_FORMAT_RBG565, lcd_orientation) < 0)
    {
      status = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      status = BSP_ERROR_NONE;
    }
  }

  return status;
}

/**
  * @brief  Initialize FMC.
  * @retval BSP status.
  */
static int32_t LCD_FMC_Init(void)
{
  int32_t status = BSP_ERROR_NONE;

  hlcd_sram[0].Instance    = FMC_NORSRAM_DEVICE;
  hlcd_sram[0].Extended    = FMC_NORSRAM_EXTENDED_DEVICE;
  hlcd_sram[0].Init.NSBank = FMC_NORSRAM_BANK1;

  if (HAL_SRAM_GetState(&hlcd_sram[0]) == HAL_SRAM_STATE_RESET)
  {
#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 0)
    /* Init the FMC Msp */
    FMC_BANK3_MspInit(&hlcd_sram[0]);

    if (MX_FSMC_BANK3_Init(&hlcd_sram[0]) != HAL_OK)
    {
      status = BSP_ERROR_BUS_FAILURE;
    }
#else
    if (IsLcdMspCbValid == 0U)
    {
      if (BSP_LCD_RegisterDefaultMspCallbacks(0) != BSP_ERROR_NONE)
      {
        status = BSP_ERROR_MSP_FAILURE;
      }
    }

    if (status == BSP_ERROR_NONE)
    {
      if (MX_FMC_BANK1_Init(&hlcd_sram[0]) != HAL_OK)
      {
        status = BSP_ERROR_BUS_FAILURE;
      }
    }
#endif
  }
  return status;
}

/**
  * @brief  DeInitialize BSP FMC.
  * @retval BSP status.
  */
static int32_t LCD_FMC_DeInit(void)
{
  int32_t status = BSP_ERROR_NONE;

#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 0)
  FMC_BANK3_MspDeInit(&hlcd_sram[0]);
#endif

  /* De-Init the FMC */
  if (HAL_SRAM_DeInit(&hlcd_sram[0]) != HAL_OK)
  {
    status = BSP_ERROR_PERIPH_FAILURE;
  }

  return status;
}

/**
  * @brief  Write 16bit values in registers of the device through BUS.
  * @param  DevAddr Device address on Bus.
  * @param  Reg     The target register start address to write.
  * @param  pData   Pointer to data buffer.
  * @param  Length  Number of data.
  * @retval BSP status.
  */
static int32_t LCD_FMC_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t  ret = BSP_ERROR_NONE;
  uint16_t i = 0;

  if ((DevAddr != LCD_FMC_ADDRESS) || (pData == NULL) || (Length == 0U))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Write register address */
    *(uint16_t *)LCD_REGISTER_ADDR = Reg;
    while (i < (2U * Length))
    {
      /* Write register value */
      *(uint16_t *)LCD_DATA_ADDR = (((uint16_t)pData[i + 1U] << 8U) & 0xFF00U) | ((uint16_t)pData[i] & 0x00FFU);
      /* Update data pointer */
      i += 2U;
    }
  }

  /* BSP status */
  return ret;
}

/**
  * @brief  Read 16bit values in registers of the device through BUS.
  * @param  DevAddr Device address on Bus.
  * @param  Reg     The target register start address to read.
  * @param  pData   Pointer to data buffer.
  * @param  Length  Number of data.
  * @retval BSP status.
  */
static int32_t LCD_FMC_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t  ret = BSP_ERROR_NONE;
  uint16_t i = 0;
  uint16_t tmp;

  if ((DevAddr != LCD_FMC_ADDRESS) || (pData == NULL) || (Length == 0U))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Write register address */
    *(uint16_t *)LCD_REGISTER_ADDR = Reg;
    while (i < (2U * Length))
    {
      tmp = *(uint16_t *)LCD_DATA_ADDR;
      pData[i]    = (uint8_t) tmp;
      pData[i + 1U] = (uint8_t)(tmp >> 8U);
      /* Update data pointer */
      i += 2U;
    }
  }

  /* BSP status */
  return ret;
}

/**
  * @brief  Send 16bit values to device through BUS.
  * @param  pData   Pointer to data buffer.
  * @param  Length  Number of data.
  * @retval BSP status.
  */
static int32_t LCD_FMC_Send(uint8_t *pData, uint16_t Length)
{
  int32_t  ret = BSP_ERROR_NONE;
  uint16_t i = 0;

  if ((pData == NULL) || (Length == 0U))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    while (i < (2U * Length))
    {
      /* Send value */
      *(uint16_t *)LCD_REGISTER_ADDR = (((uint16_t)pData[i + 1U] << 8U) & 0xFF00U) | ((uint16_t)pData[i] & 0x00FFU);
      /* Update data pointer */
      i += 2U;
    }
  }

  /* BSP status */
  return ret;
}

/**
  * @brief  Provide a tick value in millisecond.
  * @retval Tick value.
  */
static int32_t LCD_FMC_GetTick(void)
{
  return (int32_t)HAL_GetTick();
}

/**
  * @brief  Initializes FMC_BANK3 MSP.
  */
static void FMC_BANK3_MspInit(SRAM_HandleTypeDef *hsram)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsram);

  /* Enable FSMC clock */
  __HAL_RCC_FSMC_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();


  /* Common GPIO configuration */
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init_structure.Alternate = GPIO_AF12_FSMC;
  gpio_init_structure.Pull      = GPIO_PULLUP;

  /*## NE configuration #######*/
  /* NE3 : LCD */
  gpio_init_structure.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOG, &gpio_init_structure);

  gpio_init_structure.Pull      = GPIO_NOPULL;
  /*## NOE and NWE configuration #######*/
  gpio_init_structure.Pin = GPIO_PIN_4 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);

  /*## RS configuration #######*/
  gpio_init_structure.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOF, &gpio_init_structure);

  /*## Data Bus #######*/
  /* GPIOD configuration */
  gpio_init_structure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_8 | GPIO_PIN_9 | \
                            GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);

  /* GPIOE configuration */
  gpio_init_structure.Pin = GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | \
                            GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | \
                            GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &gpio_init_structure);
}

/**
  * @brief  Initializes FMC_BANK3 MSP.
  */
static void FMC_BANK3_MspDeInit(SRAM_HandleTypeDef *hsram)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsram);

  /* GPIOD configuration: GPIO_PIN_7 is  FMC_NE1 , GPIO_PIN_11 ans GPIO_PIN_12 are PSRAM_A16 and PSRAM_A17 */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 |\
                              GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_7|\
                              GPIO_PIN_11 | GPIO_PIN_12;

  HAL_GPIO_DeInit(GPIOD, gpio_init_structure.Pin);

  /* GPIOE configuration */
  gpio_init_structure.Pin   = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |\
                              GPIO_PIN_12 |GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_DeInit(GPIOE, gpio_init_structure.Pin);

  /* GPIOF configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_DeInit(GPIOF, gpio_init_structure.Pin);

  /* GPIOG configuration */
  gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_10 ;
  HAL_GPIO_DeInit(GPIOG, gpio_init_structure.Pin);

  /* Disable FSMC clock */
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
