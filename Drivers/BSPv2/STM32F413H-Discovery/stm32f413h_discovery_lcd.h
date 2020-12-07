/**
  ******************************************************************************
  * @file    stm32f413h_discovery_lcd.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32f413h_discovery_lcd.c driver.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F413H_DISCOVERY_LCD_H
#define STM32F413H_DISCOVERY_LCD_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_conf.h"
#include "stm32f413h_discovery_errno.h"

#include "../Components/st7789h2/st7789h2.h"
#include "../Components/Common/lcd.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY_LCD
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Exported_Constants LCD Exported Constants
  * @{
  */
#define LCD_INSTANCES_NBR                  1U

#define  LCD_ORIENTATION_PORTRAIT          0U /*!< Portrait orientation choice of LCD screen  */
#define  LCD_ORIENTATION_LANDSCAPE         1U /*!< Landscape orientation choice of LCD screen */
#define  LCD_ORIENTATION_PORTRAIT_ROT180   2U /*!< Portrait rotated 180° orientation choice of LCD screen */
#define  LCD_ORIENTATION_LANDSCAPE_ROT180  3U /*!< Landscape rotated 180° orientation choice of LCD screen */

    /**
  * @brief  LCD default dimension values
  */
#define LCD_DEFAULT_WIDTH        240U
#define LCD_DEFAULT_HEIGHT       240U

/**
  * @brief LCD special pins
  */
/* LCD reset pin */
#define LCD_RESET_PIN                    GPIO_PIN_13
#define LCD_RESET_GPIO_PORT              GPIOB
#define LCD_RESET_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define LCD_RESET_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOB_CLK_DISABLE()

/* LCD tearing effect pin */
#define LCD_TE_PIN                       GPIO_PIN_14
#define LCD_TE_GPIO_PORT                 GPIOB
#define LCD_TE_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define LCD_TE_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()

/* Backlight control pin */
#define LCD_BL_CTRL_PIN                  GPIO_PIN_5
#define LCD_BL_CTRL_GPIO_PORT            GPIOE
#define LCD_BL_CTRL_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOE_CLK_ENABLE()
#define LCD_BL_CTRL_GPIO_CLK_DISABLE()   __HAL_RCC_GPIOE_CLK_DISABLE()

/**
  * @}
  */
/** @defgroup STM32F769I_EVAL_LCD_Exported_Types LCD Exported Types
  * @{
  */
typedef struct
{
  uint32_t XSize;
  uint32_t YSize;
  uint32_t PixelFormat;
  uint32_t IsMspCallbacksValid;
} BSP_LCD_Ctx_t;

#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
typedef struct
{
  pSRAM_CallbackTypeDef  pMspInitCb;
  pSRAM_CallbackTypeDef  pMspDeInitCb;
}BSP_LCD_Cb_t;
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 1) */

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_LCD_Exported_Variables LCD Exported Variables
  * @{
  */
extern const GUI_Drv_t    LCD_Driver;
extern SRAM_HandleTypeDef hlcd_sram[];
extern BSP_LCD_Ctx_t      Lcd_Ctx[];
extern void               *Lcd_CompObj[];
extern LCD_Drv_t          *Lcd_Drv[];
/**
  * @}
  */

/** @addtogroup STM32F413H_DISCOVERY_LCD_Exported_Functions LCD Exported Functions
  * @{
  */
int32_t BSP_LCD_Init(uint32_t Instance, uint32_t Orientation);
int32_t BSP_LCD_DeInit(uint32_t Instance);

/* LCD generic APIs: Display control */
int32_t BSP_LCD_DisplayOn(uint32_t Instance);
int32_t BSP_LCD_DisplayOff(uint32_t Instance);
int32_t BSP_LCD_SetBrightness(uint32_t Instance, uint32_t Brightness);
int32_t BSP_LCD_GetBrightness(uint32_t Instance, uint32_t *Brightness);
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize);
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize);
int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat);

/* LCD generic APIs: Draw operations. This list of APIs is required for
   lcd gfx utilities */
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp);
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height);
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);
int32_t BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color);
int32_t BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color);

HAL_StatusTypeDef MX_FSMC_BANK3_Init(SRAM_HandleTypeDef *hsram);
#if (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)
int32_t BSP_LCD_RegisterDefaultMspCallbacks (uint32_t Instance);
int32_t BSP_LCD_RegisterMspCallbacks (uint32_t Instance, BSP_LCD_Cb_t *CallBacks);
#endif /* (USE_HAL_SRAM_REGISTER_CALLBACKS == 1)  */
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

#endif /* STM32F413H_DISCOVERY_LCD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
