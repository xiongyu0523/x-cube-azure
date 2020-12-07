/**
  ******************************************************************************
  * @file    sfu_mpu_isolation.c
  * @author  MCD Application Team
  * @brief   SFU MPU isolation primitives
  *          This file provides functions to manage the MPU isolation of the Secure Engine.
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
#include "sfu_mpu_isolation.h"
#include "sfu_low_level_security.h"
#if defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#elif defined(__CC_ARM)
#include "mapping_sbsfu.h"
#endif

/* Private variables ---------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup SFU_IMG_MPU_ISOLATION MPU Isolation of the Secure Engine
  * @brief Elements to handle the MPU isolation of the Secure Engine.
  * @{
  */

/** @defgroup SFU_BOOT_MPUIsolation_Functions MPU Isolation Functions
  * @brief Functions dealing with the MPU isolation.
  * @{
  */
/**
 * @brief This is the Supervisor calls handler.
 * @param args SVC arguments
 * @retval void
 * @note Installed in startup_stm32xxxx.s
 *
 * This handler handles 2 requests:
 * \li Secure Engine SVC: run a Secure Engine privileged operation provided as a parameter (see @ref SE_FunctionIDTypeDef)
 * \li Internal SB_SFU SVC: run a SB_SFU privileged operation provided as a parameter (see @ref SFU_MPU_PrivilegedOpId)
 */
void MPU_SVC_Handler(uint32_t *args)
{
  /* in PSP (args) conatains caller context as follow *
  * args[0] :R0
  * args[1] :R1
  * args[2] :R2
  * args[3] :R3
  * args[4] :R12
  * args[5] :LR
  * args[6] :PC to return after expception
  * args[7] :xPSR
  */
  /*  read code OP of instruction that generate SVC interrupt
   *  bottom 8-bits of the SVC instruction code OP are SVC value  */
  uint8_t code = ((char *)args[6])[-2];

  switch (code)
  {
    case 0:
      /* A Secure Engine service is called */
      SE_SVC_Handler(args);
      break;
    case 1:
      /* Internal SB_SFU privileged service */
      SFU_MPU_SVC_Handler(args);
      break;
    default:
      /* Force a reset */
      HAL_NVIC_SystemReset();
      break;
  }
}

/**
 * @brief This function triggers a SB_SFU Privileged Operation requested with SB_SysCall
 * @param args arguments
 *             The first argument is the identifier of the requested operation.
 * @retval void
 */
void SFU_MPU_SVC_Handler(uint32_t *args)
{
  switch (args[0])
  {
    case SB_SYSCALL_LAUNCH_APP:
#ifdef SFU_MPU_PROTECT_ENABLE
      SFU_LL_SECU_SetProtectionMPU_UserApp();
#endif /* SFU_MPU_PROTECT_ENABLE */
      /* make sure to return into application in privileged mode */
      __set_CONTROL(__get_CONTROL() & ~0x3); /* clear process stack & unprivileged bit */
      launch_application(args[1], (uint32_t)jump_to_function, 0, 0);
      break;
    case SB_SYSCALL_RESET:
      HAL_NVIC_SystemReset();
      break;
    default:
      /* Force a reset */
      HAL_NVIC_SystemReset();
      break;
  }
}

/**
 * @brief This functions triggers a SB_SFU system call (supervisor call): request privileged operation
 * @param syscall The identifier of the operation to be called (see @ref SFU_MPU_PrivilegedOpId)
 * @param arguments arguments of the privileged operation
 * @retval void
 */
void SFU_MPU_SysCall(uint32_t syscall, ...)
{
  /*
   * You cannot directly change to privileged mode from unprivileged mode without going through an exception, for example an SVC.
   * Handled by @ref MPU_SVC_Handler() and finally @ref SFU_MPU_SVC_Handler()
   */
  __ASM volatile("SVC #1");  /* 1 is the hard-coded value to indicate a SB_SFU syscall */
}

/**
 * @brief This is a helper function to determine if we are currently running in non-privileged mode or not
 * @param void
 * @retval 0 if we are in privileged mode, 1 if we are in non-privileged mode
 */
uint32_t SFU_MPU_IsUnprivileged(void)
{
  return ((__get_IPSR() == 0) && ((__get_CONTROL() & 1) == 1));
}


/**
 * @brief This is a helper function to enter the unprivileged level for software execution
 * @param void
 * @retval void
 */
void SFU_MPU_EnterUnprivilegedMode(void)
{
  __set_PSP(__get_MSP()); /* set up Process Stack Pointer to current stack pointer */
  __set_MSP(SE_REGION_SRAM1_STACK_TOP); /* change main stack to point on privileged stack */
  __set_CONTROL(__get_CONTROL() | 3); /* bit 0 = 1: unpriviledged      bit 1=1: stack=PSP */
  __ISB();
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
