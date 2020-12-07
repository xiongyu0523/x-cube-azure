/**
  ******************************************************************************
  * @file    stm32f413h_discovery_bus.c
  * @author  MCD Application Team
  * @brief   This file provides a set of firmware functions Bus operations to
  *          link external devices available on STM32F413H-DISCOVERY board
  *          (MB1209) from STMicroelectronics.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#include "stm32f413h_discovery_bus.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY_BUS
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Private_Macros BUS Private Macros
  * @{
  */

#define DIV_ROUND_CLOSEST(x, d)  (((x) + ((d) / 2U)) / (d))
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Private_Constants BUS Private Constants
  * @{
  */
#define I2C_ANALOG_FILTER_ENABLE        1U
#define I2C_ANALOG_FILTER_DELAY_MIN     50U     /* ns */
#define I2C_ANALOG_FILTER_DELAY_MAX     260U    /* ns */
#define I2C_ANALOG_FILTER_DELAY_DEFAULT	2U      /* ns */

#define VALID_PRESC_NBR         100U
#define PRESC_MAX               16U
#define SCLDEL_MAX              16U
#define SDADEL_MAX              16U
#define SCLH_MAX                256U
#define SCLL_MAX                256U
#define I2C_DNF_MAX             16U
#define NSEC_PER_SEC            1000000000UL

#define FMPI2C_ANALOG_FILTER_DELAY_DEFAULT 2U
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Exported_Variables BUS Exported Variables
  * @{
  */
FMPI2C_HandleTypeDef hbus_fmpi2c1;
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Private_Variables BUS Private Variables
  * @{
  */
#if (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 1)
static uint32_t IsFmpi2c1MspCbValid = 0;
#endif
static uint32_t FMPI2C1InitCounter = 0;

/**
 * @brief i2c_charac - private i2c specification timing
 *  rate: I2C bus speed (Hz)
 *  rate_min: 80% of I2C bus speed (Hz)
 *  rate_max: 100% of I2C bus speed (Hz)
 *  fall_max: Max fall time of both SDA and SCL signals (ns)
 *  rise_max: Max rise time of both SDA and SCL signals (ns)
 *  hddat_min: Min data hold time (ns)
 *  vddat_max: Max data valid time (ns)
 *  sudat_min: Min data setup time (ns)
 *  l_min: Min low period of the SCL clock (ns)
 *  h_min: Min high period of the SCL clock (ns)
 */
struct i2c_specs
{
  uint32_t rate;
  uint32_t rate_min;
  uint32_t rate_max;
  uint32_t fall_max;
  uint32_t rise_max;
  uint32_t hddat_min;
  uint32_t vddat_max;
  uint32_t sudat_min;
  uint32_t l_min;
  uint32_t h_min;
};

enum i2c_speed
{
  I2C_SPEED_STANDARD  = 0U, /* 100 kHz */
  I2C_SPEED_FAST      = 1U, /* 400 kHz */
  I2C_SPEED_FAST_PLUS = 2U, /* 1 MHz */
};

/**
 * @brief i2c_setup - private I2C timing setup parameters
 *  rise_time: Rise time (ns)
 *  fall_time: Fall time (ns)
 *  dnf: Digital filter coefficient (0-16)
 *  analog_filter: Analog filter delay (On/Off)
 */
struct i2c_setup
{
  uint32_t rise_time;
  uint32_t fall_time;
  uint32_t dnf;
  uint32_t analog_filter;
};


/**
 * @brief i2c_timings - private I2C output parameters
 *  node: List entry
 *  presc: Prescaler value
 *  scldel: Data setup time
 *  sdadel: Data hold time
 *  sclh: SCL high period (master mode)
 *  scll: SCL low period (master mode)
 */
struct i2c_timings
{
  uint32_t presc;
  uint32_t scldel;
  uint32_t sdadel;
  uint32_t sclh;
  uint32_t scll;
};

static const struct i2c_specs i2c_specs[] =
{
  [I2C_SPEED_STANDARD] =
  {
    .rate = 100000,
    .rate_min = 100000,
    .rate_max = 120000,
    .fall_max = 300,
    .rise_max = 1000,
    .hddat_min = 0,
    .vddat_max = 3450,
    .sudat_min = 250,
    .l_min = 4700,
    .h_min = 4000,
  },
  [I2C_SPEED_FAST] =
  {
    .rate = 400000,
    .rate_min = 320000,
    .rate_max = 400000,
    .fall_max = 300,
    .rise_max = 300,
    .hddat_min = 0,
    .vddat_max = 900,
    .sudat_min = 100,
    .l_min = 1300,
    .h_min = 600,
  },
  [I2C_SPEED_FAST_PLUS] =
  {
    .rate = 1000000,
    .rate_min = 800000,
    .rate_max = 1000000,
    .fall_max = 120,
    .rise_max = 120,
    .hddat_min = 0,
    .vddat_max = 450,
    .sudat_min = 50,
    .l_min = 500,
    .h_min = 260,
  },
};

static const struct i2c_setup i2c_user_setup[] =
{
  [I2C_SPEED_STANDARD] =
  {
    .rise_time = 400,
    .fall_time = 100,
    .dnf = I2C_ANALOG_FILTER_DELAY_DEFAULT,
    .analog_filter = 1,
  },
  [I2C_SPEED_FAST] =
  {
    .rise_time = 250,
    .fall_time = 100,
    .dnf = I2C_ANALOG_FILTER_DELAY_DEFAULT,
    .analog_filter = 1,
  },
  [I2C_SPEED_FAST_PLUS] =
  {
    .rise_time = 60,
    .fall_time = 100,
    .dnf = I2C_ANALOG_FILTER_DELAY_DEFAULT,
    .analog_filter = 1,
  },
};
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Private_FunctionPrototypes Bus Private Functions Prototypes
  * @{
  */
static void FMPI2C1_MspInit(FMPI2C_HandleTypeDef *hfmpi2c);
static void FMPI2C1_MspDeInit(FMPI2C_HandleTypeDef *hfmpi2c);
static int32_t FMPI2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length);
static int32_t FMPI2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length);
static uint32_t I2C_GetTiming(uint32_t clock_src_hz, uint32_t i2cfreq_hz);
/**
  * @}
  */

/** @addtogroup STM32F413H_DISCOVERY_BUS_Exported_Functions
  * @{
  */

/**
  * @brief  Initializes I2C HAL.
  * @retval None
  */
int32_t BSP_FMPI2C1_Init(void)
{
  int32_t ret = BSP_ERROR_NONE;

  hbus_fmpi2c1.Instance  = BUS_FMPI2C1;

  if(FMPI2C1InitCounter++ == 0U)
  {
    if(HAL_FMPI2C_GetState(&hbus_fmpi2c1) == HAL_FMPI2C_STATE_RESET)
    {
#if (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 0)
      /* Init the I2C Msp */
      FMPI2C1_MspInit(&hbus_fmpi2c1);
#else
      if(IsFmpi2c1MspCbValid == 0U)
      {
        if(BSP_FMPI2C1_RegisterDefaultMspCallbacks() != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_MSP_FAILURE;
        }
      }
#endif

      if(ret == BSP_ERROR_NONE)
      {
        /* Init the I2C */
        if (MX_FMPI2C1_Init(&hbus_fmpi2c1, I2C_GetTiming (HAL_RCC_GetPCLK1Freq(), BUS_FMPI2C1_FREQUENCY)) != HAL_OK)
        {
          ret = BSP_ERROR_BUS_FAILURE;
        }
        else
        {
          if( HAL_FMPI2CEx_ConfigAnalogFilter(&hbus_fmpi2c1, FMPI2C_ANALOGFILTER_ENABLE) != HAL_OK)
          {
            ret = BSP_ERROR_BUS_FAILURE;
          }
        }
      }
    }
  }

  return ret;
}

/**
  * @brief  DeInitializes I2C HAL.
  * @retval None
  */
int32_t BSP_FMPI2C1_DeInit(void)
{
  int32_t ret  = BSP_ERROR_NONE;

  if (FMPI2C1InitCounter > 0U)
  {
    if (--FMPI2C1InitCounter == 0U)
    {
#if (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 0)
      FMPI2C1_MspDeInit(&hbus_fmpi2c1);
#endif /* (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 0) */

      /* Init the I2C */
      if (HAL_FMPI2C_DeInit(&hbus_fmpi2c1) != HAL_OK)
      {
        ret = BSP_ERROR_BUS_FAILURE;
      }
    }
  }

  return ret;
}

/**
  * @brief  MX FMPI2C1 initialization.
  * @param  phfmpi2c FMPI2C handle
  * @param  timing : I2C timings as described in the I2C peripheral V2 and V3.
  * @retval None
  */
__weak HAL_StatusTypeDef MX_FMPI2C1_Init(FMPI2C_HandleTypeDef *phfmpi2c, uint32_t timing)
{
  HAL_StatusTypeDef ret = HAL_OK;

  phfmpi2c->Init.Timing           = timing;
  phfmpi2c->Init.OwnAddress1      = 0;
  phfmpi2c->Init.AddressingMode   = FMPI2C_ADDRESSINGMODE_7BIT;
  phfmpi2c->Init.DualAddressMode  = FMPI2C_DUALADDRESS_DISABLE;
  phfmpi2c->Init.OwnAddress2      = 0;
  phfmpi2c->Init.OwnAddress2Masks = FMPI2C_OA2_NOMASK;
  phfmpi2c->Init.GeneralCallMode  = FMPI2C_GENERALCALL_DISABLE;
  phfmpi2c->Init.NoStretchMode    = FMPI2C_NOSTRETCH_DISABLE;

  if(HAL_FMPI2C_Init(phfmpi2c) != HAL_OK)
  {
    ret = HAL_ERROR;
  }
  else if (HAL_FMPI2CEx_ConfigAnalogFilter(phfmpi2c, FMPI2C_ANALOGFILTER_DISABLE) != HAL_OK)
  {
    ret = HAL_ERROR;
  }
  else
  {
    if (HAL_FMPI2CEx_ConfigDigitalFilter(phfmpi2c, FMPI2C_ANALOG_FILTER_DELAY_DEFAULT) != HAL_OK)
    {
      ret = HAL_ERROR;
    }
  }

  return ret;
}


/**
  * @brief  Write a 8bit value in a register of the device through BUS.
  * @param  DevAddr   Device address on Bus.
  * @param  Reg    The target register address to write
  * @param  pData  The target register value to be written
  * @param  Length buffer size to be written
  * @retval HAL status
  */
int32_t BSP_FMPI2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  if(FMPI2C1_WriteReg(DevAddr, Reg, FMPI2C_MEMADD_SIZE_8BIT, pData, Length) != HAL_OK)
  {
    if( HAL_FMPI2C_GetError(&hbus_fmpi2c1) == HAL_FMPI2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret =  BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Read a 8bit register of the device through BUS
  * @param  DevAddr Device address on BUSFMPI2C_MEMADD_SIZE_16BIT
  * @param  Reg     The target register address to read
  * @param  pData   Pointer to data buffer
  * @param  Length  Length of the data
  * @retval HAL status
  */
int32_t BSP_FMPI2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  if(FMPI2C1_ReadReg(DevAddr, Reg, FMPI2C_MEMADD_SIZE_8BIT, pData, Length) != HAL_OK)
  {
    if( HAL_FMPI2C_GetError(&hbus_fmpi2c1) == HAL_FMPI2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret =  BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Write a 16bit value in a register of the device through BUS.
  * @param  DevAddr Device address on Bus.
  * @param  Reg    The target register address to write
  * @param  pData  The target register value to be written
  * @param  Length buffer size to be written
  * @retval HAL status
  */
int32_t BSP_FMPI2C1_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  if(FMPI2C1_WriteReg(DevAddr, Reg, FMPI2C_MEMADD_SIZE_16BIT, pData, Length) != HAL_OK)
  {
    if( HAL_FMPI2C_GetError(&hbus_fmpi2c1) == HAL_FMPI2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret =  BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Read a 16bit register of the device through BUS
  * @param  DevAddr Device address on BUS
  * @param  Reg     The target register address to read
  * @param  pData   Pointer to data buffer
  * @param  Length  Length of the data
  * @retval HAL status
  */
int32_t BSP_FMPI2C1_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  if(FMPI2C1_ReadReg(DevAddr, Reg, FMPI2C_MEMADD_SIZE_16BIT, pData, Length) != HAL_OK)
  {
    if( HAL_FMPI2C_GetError(&hbus_fmpi2c1) == HAL_FMPI2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret =  BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Checks if target device is ready for communication.
  * @note   This function is used with Memory devices
  * @param  DevAddr  Target device address
  * @param  Trials   Number of trials
  * @retval HAL status
  */
int32_t BSP_FMPI2C1_IsReady(uint16_t DevAddr, uint32_t Trials)
{
  int32_t ret = BSP_ERROR_NONE;

  if(HAL_FMPI2C_IsDeviceReady(&hbus_fmpi2c1, DevAddr, Trials, BUS_FMPI2C1_POLL_TIMEOUT) != HAL_OK)
  {
    ret = BSP_ERROR_BUSY;
  }

  return ret;
}

/**
  * @brief  Delay function
  * @retval Tick value
  */
int32_t BSP_GetTick(void)
{
  return (int32_t)HAL_GetTick();
}

#if (USE_HAL_FMPI2C_REGISTER_CALLBACKS == 1)
/**
  * @brief Register Default FMPI2C1 Bus Msp Callbacks
  * @retval BSP status
  */
int32_t BSP_FMPI2C1_RegisterDefaultMspCallbacks(void)
{
  int32_t ret = BSP_ERROR_NONE;

  __HAL_FMPI2C_RESET_HANDLE_STATE(&hbus_fmpi2c1);

  /* Register default MspInit/MspDeInit Callback */
  if(HAL_FMPI2C_RegisterCallback(&hbus_fmpi2c1, HAL_FMPI2C_MSPINIT_CB_ID, FMPI2C1_MspInit) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(HAL_FMPI2C_RegisterCallback(&hbus_fmpi2c1, HAL_FMPI2C_MSPDEINIT_CB_ID, FMPI2C1_MspDeInit) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    IsFmpi2c1MspCbValid = 1U;
  }

  /* BSP status */
  return ret;
}

/**
  * @brief Register FMPI2C1 Bus Msp Callback registering
  * @param Callbacks     pointer to FMPI2C1 MspInit/MspDeInit callback functions
  * @retval BSP status
  */
int32_t BSP_FMPI2C1_RegisterMspCallbacks(BSP_FMPI2C_Cb_t *Callback)
{
  int32_t ret = BSP_ERROR_NONE;

  __HAL_FMPI2C_RESET_HANDLE_STATE(&hbus_fmpi2c1);

  /* Register MspInit/MspDeInit Callbacks */
  if(HAL_FMPI2C_RegisterCallback(&hbus_fmpi2c1, HAL_FMPI2C_MSPINIT_CB_ID, Callback->pMspI2cInitCb) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(HAL_FMPI2C_RegisterCallback(&hbus_fmpi2c1, HAL_FMPI2C_MSPDEINIT_CB_ID, Callback->pMspI2cDeInitCb) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    IsFmpi2c1MspCbValid = 1U;
  }

  /* BSP status */
  return ret;
}
#endif /* USE_HAL_FMPI2C_REGISTER_CALLBACKS */

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_BUS_Private_Functions BUS Private Functions
  * @{
  */

/**
* @brief  Convert the I2C Frequency into I2C timing.
* @param  clock_src_hz : I2C source clock in HZ.
* @param  i2cfreq_hz : I2C frequency in Hz.
* @retval Prescaler dividor
*/
static uint32_t I2C_GetTiming(uint32_t clock_src_hz, uint32_t i2cfreq_hz)
{
  uint32_t ret = 0;
  uint32_t speed = 0;
  uint32_t is_valid_speed = 0;
  uint32_t p_prev = PRESC_MAX;
  uint32_t i2cclk;
  uint32_t i2cspeed;
  uint32_t clk_error_prev;
  uint32_t tsync;
  uint32_t af_delay_min, af_delay_max;
  uint32_t dnf_delay;
  uint32_t clk_min, clk_max;
  int32_t sdadel_min, sdadel_max;
  int32_t scldel_min;
  struct i2c_timings *s;
  struct i2c_timings valid_timing[VALID_PRESC_NBR];
  uint32_t p, l, a, h;
  uint32_t valid_timing_nbr = 0;

  if((clock_src_hz == 0U) || (i2cfreq_hz == 0U))
  {
    ret = 0;
  }
  else
  {
    for (uint32_t i =0 ; i <=  (uint32_t)I2C_SPEED_FAST_PLUS ; i++)
    {
      if ((i2cfreq_hz >= i2c_specs[i].rate_min) &&
          (i2cfreq_hz <= i2c_specs[i].rate_max))
      {
        is_valid_speed = 1;
        speed = i;
        break;
      }
    }

    if(is_valid_speed != 0U)
    {
      i2cclk = DIV_ROUND_CLOSEST(NSEC_PER_SEC, clock_src_hz);
      i2cspeed = DIV_ROUND_CLOSEST(NSEC_PER_SEC, i2cfreq_hz);

      clk_error_prev = i2cspeed;

      /*  Analog and Digital Filters */
      af_delay_min =((i2c_user_setup[speed].analog_filter == 1U)? I2C_ANALOG_FILTER_DELAY_MIN : 0U);
      af_delay_max =((i2c_user_setup[speed].analog_filter == 1U)? I2C_ANALOG_FILTER_DELAY_MAX : 0U);

      dnf_delay = i2c_user_setup[speed].dnf * i2cclk;

      sdadel_min = (int32_t)i2c_user_setup[speed].fall_time - (int32_t)i2c_specs[speed].hddat_min -
                   (int32_t)af_delay_min - (int32_t)(((int32_t)i2c_user_setup[speed].dnf + 3) * (int32_t)i2cclk);

      sdadel_max = (int32_t)i2c_specs[speed].vddat_max - (int32_t)i2c_user_setup[speed].rise_time -
                   (int32_t)af_delay_max - (int32_t)(((int32_t)i2c_user_setup[speed].dnf + 4) * (int32_t)i2cclk);

      scldel_min = (int32_t)i2c_user_setup[speed].rise_time + (int32_t)i2c_specs[speed].sudat_min;

      if (sdadel_min <= 0)
      {
        sdadel_min = 0;
      }

      if (sdadel_max <= 0)
      {
        sdadel_max = 0;
      }

      /* Compute possible values for PRESC, SCLDEL and SDADEL */
      for (p = 0; p < PRESC_MAX; p++)
      {
        for (l = 0; l < SCLDEL_MAX; l++)
        {
          uint32_t scldel = (l + 1U) * (p + 1U) * i2cclk;

          if (scldel < (uint32_t)scldel_min)
          {
            continue;
          }

          for (a = 0; a < SDADEL_MAX; a++)
          {
            uint32_t sdadel = ((a * (p + 1U)) + 1U) * i2cclk;

            if (((sdadel >= (uint32_t)sdadel_min) && (sdadel <= (uint32_t)sdadel_max))&& (p != p_prev))
            {
              valid_timing[valid_timing_nbr].presc = p;
              valid_timing[valid_timing_nbr].scldel = l;
              valid_timing[valid_timing_nbr].sdadel = a;
              p_prev = p;
              valid_timing_nbr ++;

              if(valid_timing_nbr >= VALID_PRESC_NBR)
              {
                /* max valid timing buffer is full, use only these values*/
                goto  Compute_scll_sclh;
              }
            }
          }
        }
      }

      if (valid_timing_nbr == 0U)
      {
        return 0;
      }

    Compute_scll_sclh:
      tsync = af_delay_min + dnf_delay + (2U * i2cclk);
      s = NULL;
      clk_max = NSEC_PER_SEC / i2c_specs[speed].rate_min;
      clk_min = NSEC_PER_SEC / i2c_specs[speed].rate_max;

      /*
      * Among Prescaler possibilities discovered above figures out SCL Low
      * and High Period. Provided:
      * - SCL Low Period has to be higher than Low Period of tehs SCL Clock
      *   defined by I2C Specification. I2C Clock has to be lower than
      *   (SCL Low Period - Analog/Digital filters) / 4.
      * - SCL High Period has to be lower than High Period of the SCL Clock
      *   defined by I2C Specification
      * - I2C Clock has to be lower than SCL High Period
      */
      for (uint32_t count = 0; count < valid_timing_nbr; count++)
      {
        uint32_t prescaler = (valid_timing[count].presc + 1U) * i2cclk;

        for (l = 0; l < SCLL_MAX; l++)
        {
          uint32_t tscl_l = ((l + 1U) * prescaler) + tsync;

          if ((tscl_l < i2c_specs[speed].l_min) || (i2cclk >= ((tscl_l - af_delay_min - dnf_delay) / 4U)))
          {
            continue;
          }

          for (h = 0; h < SCLH_MAX; h++)
          {
            uint32_t tscl_h = ((h + 1U) * prescaler) + tsync;
            uint32_t tscl = tscl_l + tscl_h + i2c_user_setup[speed].rise_time + i2c_user_setup[speed].fall_time;

            if ((tscl >= clk_min) && (tscl <= clk_max) && (tscl_h >= i2c_specs[speed].h_min) && (i2cclk < tscl_h))
            {
              int32_t clk_error = (int32_t)tscl - (int32_t)i2cspeed;

              if (clk_error < 0)
              {
                clk_error = -clk_error;
              }

              /* save the solution with the lowest clock error */
              if ((uint32_t)clk_error < clk_error_prev)
              {
                clk_error_prev = (uint32_t)clk_error;
                valid_timing[count].scll = l;
                valid_timing[count].sclh = h;
                s = &valid_timing[count];
              }
            }
          }
        }
      }

      if (s == NULL)
      {
        return 0;
      }

      ret = ((s->presc  & 0x0FU) << 28) |
            ((s->scldel & 0x0FU) << 20) |
            ((s->sdadel & 0x0FU) << 16) |
            ((s->sclh & 0xFFU) << 8) |
            ((s->scll & 0xFFU) << 0);
    }
  }

  return ret;
}

/**
  * @brief  Initializes I2C MSP.
  * @param  hfmpi2c FMPI2C handler
  * @retval None
  */
static void FMPI2C1_MspInit(FMPI2C_HandleTypeDef *hfmpi2c)
{
  GPIO_InitTypeDef  gpio_init_structure;

  if(HAL_FMPI2C_GetState(hfmpi2c) == HAL_FMPI2C_STATE_RESET)
  {
  /*** Configure the GPIOs ***/
  /* Enable GPIO clock */
  BUS_FMPI2C1_SCL_GPIO_CLK_ENABLE();
  BUS_FMPI2C1_SDA_GPIO_CLK_ENABLE();

  /* Configure I2C Tx as alternate function */
  gpio_init_structure.Pin       = BUS_FMPI2C1_SCL_PIN;
  gpio_init_structure.Mode      = GPIO_MODE_AF_OD;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio_init_structure.Alternate = BUS_FMPI2C1_SCL_AF;
  HAL_GPIO_Init(BUS_FMPI2C1_SCL_GPIO_PORT, &gpio_init_structure);

  /* Configure I2C Rx as alternate function */
  gpio_init_structure.Pin       = BUS_FMPI2C1_SDA_PIN;
  gpio_init_structure.Alternate = BUS_FMPI2C1_SDA_AF;
  HAL_GPIO_Init(BUS_FMPI2C1_SDA_GPIO_PORT, &gpio_init_structure);

  /*** Configure the I2C peripheral ***/
  /* Enable I2C clock */
  BUS_FMPI2C1_CLK_ENABLE();

  /* Force the I2C peripheral clock reset */
  BUS_FMPI2C1_FORCE_RESET();

  /* Release the I2C peripheral clock reset */
  BUS_FMPI2C1_RELEASE_RESET();
  }
}

/**
  * @brief  DeInitializes I2C MSP.
  * @param  hfmpi2c FMPI2C handler
  * @retval None
  */
static void FMPI2C1_MspDeInit(FMPI2C_HandleTypeDef *hfmpi2c)
{
  GPIO_InitTypeDef  gpio_init_structure;

  if(HAL_FMPI2C_GetState(hfmpi2c) == HAL_FMPI2C_STATE_RESET)
  {
  /* Configure I2C Tx, Rx as alternate function */
  gpio_init_structure.Pin = BUS_FMPI2C1_SCL_PIN;
  HAL_GPIO_DeInit(BUS_FMPI2C1_SCL_GPIO_PORT, gpio_init_structure.Pin);

  gpio_init_structure.Pin = BUS_FMPI2C1_SDA_PIN;
  HAL_GPIO_DeInit(BUS_FMPI2C1_SDA_GPIO_PORT, gpio_init_structure.Pin);

  /* Disable I2C clock */
  BUS_FMPI2C1_CLK_DISABLE();
  }
}

/**
  * @brief  Write a value in a register of the device through BUS.
  * @param  DevAddr    Device address on Bus.
  * @param  Reg        The target register address to write
  * @param  MemAddSize Size of internal memory address  
  * @param  pData      The target register value to be written
  * @param  Length     data length in bytes
  * @retval HAL status
  */
static int32_t FMPI2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length)
{
  if(HAL_FMPI2C_Mem_Write(&hbus_fmpi2c1, DevAddr, Reg, MemAddSize, pData, Length, BUS_FMPI2C1_POLL_TIMEOUT) == HAL_OK)
  {
    return BSP_ERROR_NONE;
  }

  return BSP_ERROR_BUS_FAILURE;
}

/**
  * @brief  Read a register of the device through BUS
  * @param  DevAddr    Device address on BUS
  * @param  Reg        The target register address to read
  * @param  MemAddSize Size of internal memory address   
  * @param  pData      The target register value to be read
  * @param  Length     data length in bytes  
  * @retval HAL status
  */
static int32_t FMPI2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length)
{
  if (HAL_FMPI2C_Mem_Read(&hbus_fmpi2c1, DevAddr, Reg, MemAddSize, pData, Length, BUS_FMPI2C1_POLL_TIMEOUT) == HAL_OK)
  {
    return BSP_ERROR_NONE;
  }

  return BSP_ERROR_BUS_FAILURE;
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
