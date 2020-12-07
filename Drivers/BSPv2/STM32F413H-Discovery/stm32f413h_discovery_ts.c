/**
  ******************************************************************************
  * @file    stm32f413h_discovery_ts.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions needed to manage the Touch
  *          Screen on STM32F413H-DISCOVERY board.
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
   - This driver is used to drive the touch screen module of the STM32F413H-DISCOVERY
     evaluation board on the FRIDA LCD mounted on MB1209 daughter board.
     The touch screen driver IC is a FT6x36 type which share the same register naming
     with FT6206 type.

2. Driver description:
---------------------
  + Initialization steps:
     o Initialize the TS module using the BSP_TS_Init() function. This
       function includes the MSP layer hardware resources initialization and the
       communication layer configuration to start the TS use. The LCD size properties
       (x and y) are passed as parameters.
     o If TS interrupt mode is desired, you must configure the TS interrupt mode
       by calling the function BSP_TS_ITConfig(). The TS interrupt mode is generated
       as an external interrupt whenever a touch is detected.

  + Touch screen use
     o The touch screen state is captured whenever the function BSP_TS_GetState() is
       used. This function returns information about the last LCD touch occurred
       in the TS_StateTypeDef structure.
------------------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_ts.h"
#include "stm32f413h_discovery_bus.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_TS TS
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Private_Types_Definitions TS Private Types Definitions
  * @{
  */
typedef void (* BSP_EXTI_LineCallback) (void);
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Private_Defines TS Private Types Defines
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Private_Macros TS Private Macros
  * @{
  */
#define TS_MIN(a,b) ((a > b) ? b : a)
/**
  * @}
  */
/** @defgroup STM32F413H_DISCOVERY_TS_Private_Function_Prototypes TS Private Function Prototypes
  * @{
  */
static int32_t FT6X06_Probe(void);
static void TS_EXTI_Callback(void);
static void TS_RESET_MspInit(void);
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Privates_Variables TS Privates Variables
  * @{
  */
static TS_Drv_t   *Ts_Drv = NULL;

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Exported_Variables TS Exported Variables
  * @{
  */
FT6X06_Object_t    *Ts_CompObj;
EXTI_HandleTypeDef hts_exti[TS_INSTANCES_NBR];
TS_Ctx_t           Ts_Ctx[TS_INSTANCES_NBR] = {0};

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Exported_Functions TS Exported Functions
  * @{
  */

/**
  * @brief  Initializes and configures the touch screen functionalities and
  *         configures all necessary hardware resources (GPIOs, I2C, clocks..).
  * @param  Instance TS instance. Could be only 0.
  * @param  TS_Init  TS Init structure
  * @retval BSP status
  */
int32_t BSP_TS_Init(uint32_t Instance, TS_Init_t *TS_Init)
{
  int32_t ret = BSP_ERROR_NONE;

  if((Instance >= TS_INSTANCES_NBR) || (TS_Init->Width == 0U) ||\
    ( TS_Init->Width > TS_MAX_WIDTH) || (TS_Init->Height == 0U) ||\
    ( TS_Init->Height > TS_MAX_HEIGHT) ||\
    (TS_Init->Accuracy > TS_MIN((TS_Init->Width), (TS_Init->Height))))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    TS_RESET_MspInit();

    if(FT6X06_Probe() != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_NO_INIT;
    }
    else
    {
      Ts_Ctx[Instance].Width             = TS_Init->Width;
      Ts_Ctx[Instance].Height            = TS_Init->Height;
      Ts_Ctx[Instance].Orientation       = TS_Init->Orientation;
      Ts_Ctx[Instance].Accuracy          = TS_Init->Accuracy;
    }
  }

  return ret;
}

/**
  * @brief  De-Initializes the touch screen functionalities
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
int32_t BSP_TS_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Ts_Drv->DeInit(Ts_CompObj) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Get Touch Screen instance capabilities
  * @param  Instance Touch Screen instance
  * @param  Capabilities pointer to Touch Screen capabilities
  * @retval BSP status
  */
int32_t BSP_TS_GetCapabilities(uint32_t Instance, TS_Capabilities_t *Capabilities)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    (void)Ts_Drv->GetCapabilities(Ts_CompObj, Capabilities);
  }

  return ret;
}

/**
  * @brief  Configures and enables the touch screen interrupts.
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
int32_t BSP_TS_EnableIT(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  static const uint32_t TS_EXTI_LINE[TS_INSTANCES_NBR] = {TS_INT_LINE};
  static BSP_EXTI_LineCallback TsCallback[TS_INSTANCES_NBR] = {TS_EXTI_Callback};
  GPIO_InitTypeDef gpio_init_structure;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Configure Interrupt mode for TS_INT pin falling edge : when a new touch is available */
    /* TS_INT pin is active on low level on new touch available */
    gpio_init_structure.Pin = TS_INT_PIN;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FAST;
    gpio_init_structure.Mode = GPIO_MODE_IT_FALLING;
    HAL_GPIO_Init(TS_INT_GPIO_PORT, &gpio_init_structure);

    if(Ts_Drv->EnableIT(Ts_CompObj) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(HAL_EXTI_GetHandle(&hts_exti[Instance], TS_EXTI_LINE[Instance]) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_EXTI_RegisterCallback(&hts_exti[Instance],  HAL_EXTI_COMMON_CB_ID, TsCallback[Instance])!= HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      /* Enable and set the TS_INT EXTI Interrupt to an intermediate priority */
      HAL_NVIC_SetPriority((IRQn_Type)(TS_INT_EXTI_IRQn), BSP_TS_IT_PRIORITY, 0x00);
      HAL_NVIC_EnableIRQ((IRQn_Type)(TS_INT_EXTI_IRQn));
    }
  }

  return ret;
}

/**
  * @brief  Disables the touch screen interrupts.
  * @param  Instance TS instance. Could be only 0.
  * @retval BSP status
  */
int32_t BSP_TS_DisableIT(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Configure TS_INT_PIN low level to generate MFX_IRQ_OUT in EXTI on MCU            */
    gpio_init_structure.Pin  = TS_INT_PIN;
    HAL_GPIO_DeInit(TS_INT_GPIO_PORT, gpio_init_structure.Pin);

    /* Disable the TS in interrupt mode */
    /* In that case the INT output of FT6X06 when new touch is available */
    /* is active on low level and directed on EXTI */
    if(Ts_Drv->DisableIT(Ts_CompObj) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  BSP TS Callback.
  * @retval None.
  */
__weak void BSP_TS_Callback(uint32_t Instance)
{
  /* This function should be implemented by the user application.
     It is called into this driver when an event on TS touch detection */
}

/**
  * @brief  Returns positions of a single touch screen.
  * @param  Instance  TS instance. Could be only 0.
  * @param  TS_State  Pointer to touch screen current state structure
  * @retval BSP status
  */
int32_t BSP_TS_GetState(uint32_t Instance, TS_State_t *TS_State)
{
  int32_t ret = BSP_ERROR_NONE;
  static uint32_t _x = 0;
  static uint32_t _y = 0;
  uint32_t tmp;
  uint32_t Raw_x;
  uint32_t Raw_y;
  uint32_t xDiff;
  uint32_t yDiff;

  if(Instance >=TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Get Data from TS */
  else if(Ts_Drv->GetState(Ts_CompObj, TS_State) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if(TS_State->TouchDetected != 0U)
  {
    Raw_x = TS_State->TouchX;
    Raw_y = TS_State->TouchY;

    /* Apply Orientation */
    if((Ts_Ctx[Instance].Orientation & TS_SWAP_XY) == TS_SWAP_XY)
    {
      tmp   = Raw_x;
      Raw_x = Raw_y;
      Raw_y = tmp;
    }

    if((Ts_Ctx[Instance].Orientation & TS_SWAP_X) == TS_SWAP_X)
    {
      Raw_x = Ts_Ctx[Instance].MaxXl - 1UL - Raw_x;
    }

    if((Ts_Ctx[Instance].Orientation & TS_SWAP_Y) == TS_SWAP_Y)
    {
      Raw_y = Ts_Ctx[Instance].MaxYl - 1UL - Raw_y;
    }

    xDiff = (Raw_x > _x)? (Raw_x - _x): (_x - Raw_x);
    yDiff = (Raw_y > _y)? (Raw_y - _y): (_y - Raw_y);

    if (xDiff > Ts_Ctx[Instance].Accuracy)
    {
      _x = Raw_x;
    }

    if (yDiff > Ts_Ctx[Instance].Accuracy)
    {
      _y = Raw_y;
    }

    /* Apply boundaries */
    if(Ts_Ctx[Instance].Width != Ts_Ctx[Instance].MaxXl)
    {
      _x = ((_x*Ts_Ctx[Instance].Width)/Ts_Ctx[Instance].MaxXl);
    }

    if(Ts_Ctx[Instance].Height != Ts_Ctx[Instance].MaxYl)
    {
      _y = ((_y*Ts_Ctx[Instance].Height)/Ts_Ctx[Instance].MaxYl);
    }

    TS_State->TouchX = _x;
    TS_State->TouchY = _y;
  }
  else
  {
    ret = BSP_ERROR_TS_TOUCH_NOT_DETECTED;
  }

  return ret;
}

#if (USE_TS_MULTI_TOUCH > 0)
/**
  * @brief  Returns positions of multi touch screen.
  * @param  Instance  TS instance. Could be only 0.
  * @param  TS_State  Pointer to touch screen current state structure
  * @retval BSP status
  */
int32_t BSP_TS_Get_MultiTouchState(uint32_t Instance, TS_MultiTouch_State_t *TS_State)
{
  int32_t ret = BSP_ERROR_NONE;
  static uint32_t _x[TS_TOUCH_NBR] = {0, 0};
  static uint32_t _y[TS_TOUCH_NBR] = {0, 0};
  uint32_t tmp;
  uint32_t Raw_x[TS_TOUCH_NBR];
  uint32_t Raw_y[TS_TOUCH_NBR];
  uint32_t xDiff;
  uint32_t yDiff;
  uint32_t index;

  if(Instance >=TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Get each touch coordinates */
  else if(Ts_Drv->GetMultiTouchState(Ts_CompObj, TS_State) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }/* Check and update the number of touches active detected */
  else if(TS_State->TouchDetected != 0U)
  {
    for(index = 0; index < TS_State->TouchDetected; index++)
    {
      Raw_x[index] = TS_State->TouchX[index];
      Raw_y[index] = TS_State->TouchY[index];

      if((Ts_Ctx[Instance].Orientation & TS_SWAP_XY) == TS_SWAP_XY)
      {
        tmp = Raw_x[index];
        Raw_x[index] = Raw_y[index];
        Raw_y[index] = tmp;
      }

      if((Ts_Ctx[Instance].Orientation & TS_SWAP_X) == TS_SWAP_X)
      {
        Raw_x[index] = Ts_Ctx[Instance].MaxXl - 1UL - Raw_x[index];
      }

      if((Ts_Ctx[Instance].Orientation & TS_SWAP_Y) == TS_SWAP_Y)
      {
        Raw_y[index] = Ts_Ctx[Instance].MaxYl - 1UL - Raw_y[index];
      }

      xDiff = (Raw_x[index] > _x[index])? (Raw_x[index] - _x[index]): (_x[index] - Raw_x[index]);
      yDiff = (Raw_y[index] > _y[index])? (Raw_y[index] - _y[index]): (_y[index] - Raw_y[index]);

      if (xDiff > Ts_Ctx[Instance].Accuracy)
      {
        _x[index] = Raw_x[index];
      }

      if (yDiff > Ts_Ctx[Instance].Accuracy)
      {
        _y[index] = Raw_y[index];
      }

      TS_State->TouchX[index] = _x[index];
      TS_State->TouchY[index] = _y[index];
    }
  }
  else
  {
    ret = BSP_ERROR_TS_TOUCH_NOT_DETECTED;
  }

  return ret;
}
#endif /* USE_TS_MULTI_TOUCH > 0 */

#if (USE_TS_GESTURE > 0)
/**
  * @brief  Update gesture Id following a touch detected.
  * @param  Instance      TS instance. Could be only 0.
  * @param  GestureConfig Pointer to gesture configuration structure
  * @retval BSP status
  */
int32_t BSP_TS_GestureConfig(uint32_t Instance, TS_Gesture_Config_t *GestureConfig)
{
  int32_t ret;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Ts_Drv->GestureConfig(Ts_CompObj, GestureConfig) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Update gesture Id following a touch detected.
  * @param  Instance   TS instance. Could be only 0.
  * @param  GestureId  Pointer to gesture ID
  * @retval BSP status
  */
int32_t BSP_TS_GetGestureId(uint32_t Instance, uint32_t *GestureId)
{
  uint8_t tmp = 0;
  int32_t ret;

  if(Instance >= TS_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Get gesture Id */
    Ts_Drv->GetGesture(Ts_CompObj, &tmp);

    /* Remap gesture Id to a TS_Gesture_Id_t value */
    switch(tmp)
    {
    case FT6X06_GEST_ID_NO_GESTURE :
      *GestureId = GESTURE_ID_NO_GESTURE;
      break;
    case FT6X06_GEST_ID_MOVE_UP :
      *GestureId = GESTURE_ID_MOVE_UP;
      break;
    case FT6X06_GEST_ID_MOVE_RIGHT :
      *GestureId = GESTURE_ID_MOVE_RIGHT;
      break;
    case FT6X06_GEST_ID_MOVE_DOWN :
      *GestureId = GESTURE_ID_MOVE_DOWN;
      break;
    case FT6X06_GEST_ID_MOVE_LEFT :
      *GestureId = GESTURE_ID_MOVE_LEFT;
      break;
    case FT6X06_GEST_ID_ZOOM_IN :
      *GestureId = GESTURE_ID_ZOOM_IN;
      break;
    case FT6X06_GEST_ID_ZOOM_OUT :
      *GestureId = GESTURE_ID_ZOOM_OUT;
      break;
    default :
      *GestureId = GESTURE_ID_NO_GESTURE;
      break;
    }

    ret = BSP_ERROR_NONE;
  }

  return ret;
}
#endif /* USE_TS_GESTURE > 0 */

/**
  * @brief  Set TS orientation
  * @param  Instance TS instance. Could be only 0.
  * @param  Orientation Orientation to be set
  * @retval BSP status
  */
int32_t BSP_TS_Set_Orientation(uint32_t Instance, uint32_t Orientation)
{
  Ts_Ctx[Instance].Orientation = Orientation;
  return BSP_ERROR_NONE;
}

/**
  * @brief  Get TS orientation
  * @param  Instance TS instance. Could be only 0.
  * @param  Orientation Current Orientation to be returned
  * @retval BSP status
  */
int32_t BSP_TS_Get_Orientation(uint32_t Instance, uint32_t *Orientation)
{
  *Orientation = Ts_Ctx[Instance].Orientation;
  return BSP_ERROR_NONE;
}

/**
  * @brief  This function handles TS interrupt request.
  * @retval None
  */
void BSP_TS_IRQHandler(uint32_t Instance)
{
  HAL_EXTI_IRQHandler(&hts_exti[Instance]);
}

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_TS_Private_Functions TS Private Functions
  * @{
  */

/**
  * @brief  Register Bus IOs if component ID is OK
  * @retval BSP status
  */
static int32_t FT6X06_Probe(void)
{
  int32_t ret = BSP_ERROR_NONE;
  FT6X06_IO_t              IOCtx;
  static FT6X06_Object_t   FT6X06Obj;
  FT6X06_Capabilities_t    Cap;
  uint32_t id;

  /* Configure the touch screen driver */
  IOCtx.Init        = BSP_FMPI2C1_Init;
  IOCtx.DeInit      = BSP_FMPI2C1_DeInit;
  IOCtx.ReadReg     = BSP_FMPI2C1_ReadReg;
  IOCtx.WriteReg    = BSP_FMPI2C1_WriteReg;
  IOCtx.GetTick     = BSP_GetTick;
  IOCtx.Address     = TS_I2C_ADDRESS;

  if(FT6x06_RegisterBusIO (&FT6X06Obj, &IOCtx) != FT6X06_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  else if(FT6x06_ReadID(&FT6X06Obj, &id) != FT6X06_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if(id != FT6X36_ID)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    (void)FT6X06_GetCapabilities(&FT6X06Obj, &Cap);
    Ts_Ctx[0].MultiTouch = Cap.MultiTouch;
    Ts_Ctx[0].Gesture    = Cap.Gesture;
    Ts_Ctx[0].MaxTouch   = Cap.MaxTouch;
    Ts_Ctx[0].MaxXl      = Cap.MaxXl;
    Ts_Ctx[0].MaxYl      = Cap.MaxYl;

    Ts_CompObj = &FT6X06Obj;
    Ts_Drv = (TS_Drv_t *) &FT6X06_TS_Driver;

    if(Ts_Drv->Init(Ts_CompObj) != FT6X06_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  TS EXTI touch detection callbacks.
  * @retval None
  */
static void TS_EXTI_Callback(void)
{
  BSP_TS_Callback(0);
}

/**
  * @brief  Initializes the TS_INT pin MSP.
  * @retval None
  */
static void TS_RESET_MspInit(void)
{
  GPIO_InitTypeDef  gpio_init_structure;

  TS_RESET_GPIO_CLK_ENABLE();

  /* GPIO configuration in output for TouchScreen reset signal on TS_RESET pin */
  gpio_init_structure.Pin = TS_RESET_PIN;
  gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TS_RESET_GPIO_PORT, &gpio_init_structure);
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
