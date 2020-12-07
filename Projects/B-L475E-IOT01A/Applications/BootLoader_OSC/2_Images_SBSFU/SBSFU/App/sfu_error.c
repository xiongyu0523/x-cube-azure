/**
  ******************************************************************************
  * @file    sfu_error.c
  * @author  MCD Application Team
  * @brief   SFU ERROR
  *          This file provides set of firmware functions for SB_SFU errors handling.
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
#include "sfu_fsm_states.h"
#include "sfu_error.h"
#include "sfu_low_level_security.h"
#include "sfu_trace.h"
#include "se_interface_bootloader.h"
#include "sfu_fwimg_services.h"
#include "sfu_boot.h"
#include "sfu_test.h"


/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup  SFU_ERROR SFU Errors
  * @brief This file provides the functions for Errors Management.
  * @{
  */

/** @defgroup SFU_ERROR_Imported_Variables Imported Variables
  * @{
  */

#if defined(SFU_VERBOSE_DEBUG_MODE)
extern char *m_aErrorStrings[];
#endif /* SFU_VERBOSE_DEBUG_MODE */

/**
  * @}
  */


/** @defgroup SFU_ERROR_Private_Variables Private Variables
  * @{
  */
SFU_EXCPT_IdTypeDef eLastDetectedException = SFU_EXCPT_NONE;   /*!< Last detected Exception*/
/**
  * @}
  */

/** @defgroup SFU_ERROR_Private_Functions Private Functions
  * @{
  */

/** @addtogroup SFU_EXCPT_Control_Functions
  * @{
  */
static void SFU_EXCPT_RuntimeExceptionHandler(SFU_EXCPT_IdTypeDef eExceptionId);

/**
  * @}
  */

/**
  * @}
  */


/** @defgroup SFU_ERROR_Exported_Functions Exported Functions
  * @{
  */


/** @defgroup SFU_ERROR_ERRORS_Functions High Level Error Management Functions
 *  @brief Functions for Error Management.
 *         This is the first level of functions for errors management.
 *         In a second step, exceptions are handled (see @ref SFU_EXCPT_Control_Functions).
 * @{
 */

/**
  * @brief  Set the last execution error. The state of the FSM that generated the error is stored
  * @param  uLastExecError: the last error to store into the BootInfo structure.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_BOOT_SetLastExecError(uint32_t uLastExecError)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef        e_se_status;
  SE_BootInfoTypeDef      x_boot_info;

  if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
  {
    /* Set the last execution status as a 32-bit data */
    x_boot_info.LastExecError = uLastExecError;

    /* Update the BootInfo shared area according to the modifications above */
    if (SE_INFO_WriteBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
    {
      e_ret_status = SFU_SUCCESS;
    }
  }

  return e_ret_status;
}


/**
  * @brief  Manage the Exception generated by an IRQ
  * @param  eExceptionId : Exception ID.
  *         This parameter can be a value of @ref SFU_EXCPT_ID_Structure_definition.
  * @note   Because of the interruption of the State Machine execution,
  *         it's not possible to continue without compromising the stability or
  *         the security of the solution. A System Reset is forced at the end.
  * @retval None
  */
void SFU_BOOT_IrqExceptionHandler(SFU_EXCPT_IdTypeDef eExceptionId)
{
  /* Check the value */
  if (!IS_SFU_EXCPT(eExceptionId))
  {
    eExceptionId = SFU_EXCPT_UNKNOWN;
  }

  /* At this point and according to the detected critical exceptions, the execution
     itself may be compromised and could not be possible to execute the following instructions.
     It's worth trying anyway */

  /* Try to take an action */
  SFU_EXCPT_RuntimeExceptionHandler(eExceptionId);

  /* Try to store the detected error */
  (void)SFU_BOOT_SetLastExecError((uint32_t) eExceptionId);

  /* It's not possible to continue without compromising the stability or the security of the solution.
  The State Machine needs to be aborted and a Reset must be triggered */
  SFU_BOOT_ForceReboot();
}

/**
  * @brief  Manage the Error generated during the StateMachine execution
  *
  *         If a specific error has been logged in the BootInfo area it is used.
  *         Otherwise, a generic exception is derived from the previous FSM state (minimal info).
  *         Nevertheless, the max.consecutive boot on error counter has priority over all other errors.
  * @note   This function is called when processing a critical failure (called from @ref SFU_BOOT_SM_HandleCriticalFailure in state @ref SFU_STATE_HANDLE_CRITICAL_FAILURE)
  * @param  eStateMachineState: This parameter can be a value of @ref SFU_BOOT_State_Machine_Structure_definition
  * @retval None
  */
void SFU_BOOT_StateExceptionHandler(SFU_BOOT_StateMachineTypeDef eStateMachineState)
{
  SFU_EXCPT_IdTypeDef eExceptionId;
  SE_BootInfoTypeDef  x_boot_info; /* to retrieve the counter of consecutive errors */
  SE_StatusTypeDef    e_se_status;
  uint32_t u_specific_LastExecError = (uint32_t)SFU_EXCPT_NONE;

  /* Check the parameters */
  assert_param(IS_SFU_SM_STATE(eStateMachineState));

  /*
   * First we retrieve the specific error set when the error was detected if any.
   * This gives a chance to set a specific error cause, more detailed than the exceptions computed from the FSM state.
   */
  if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
  {
    u_specific_LastExecError = x_boot_info.LastExecError;
  } /* else keep u_specific_LastExecError as SFU_EXCPT_NONE */

  switch (eStateMachineState)
  {

    case SFU_STATE_CHECK_STATUS_ON_RESET:
      eExceptionId = SFU_EXCPT_CHECK_STATUS_ON_RESET_ERR;
      /*
       * In this FSM state, we consider that the consecutive boot on error counter error handling has priority over any other error
       * So we check it systematically even if another error occurred.
       */
      if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
      {
        if (x_boot_info.ConsecutiveBootOnErrorCounter >= SFU_MAX_CONSEC_COUNTER)
        {
          /* The priority is to handle the max. consecutive boot on error case */
          eExceptionId = SFU_EXCPT_MAX_CONSEC_ERRORS_REACHED;

          /* Sanity check to make sure the max.consecutive counters error has priority over all errors */
          if ((SFU_EXCPT_NONE != u_specific_LastExecError) &&
              (eExceptionId != u_specific_LastExecError))
          {
#if defined(SFU_VERBOSE_DEBUG_MODE)
            TRACE("\r\n= [SBOOT] Last memorized error was %s but the max.consecutive errors value has been reached.", m_aErrorStrings[u_specific_LastExecError]);
#endif /* SFU_VERBOSE_DEBUG_MODE */
            /* The priority must be the max.consecutive counters handling */
            u_specific_LastExecError = eExceptionId;
          }
        } /* else go on with SFU_EXCPT_CHECK_STATUS_ON_RESET_ERR */
      } /* else go on with the ReadBootInfo related error, this is similar to an exception in SFU_STATE_CHECK_STATUS_ON_RESET anyway */
      break;

#if defined(SECBOOT_USE_LOCAL_LOADER)
    case SFU_STATE_CHECK_NEW_FW_TO_DOWNLOAD:
      eExceptionId = SFU_EXCPT_CHECK_NEW_FW_TO_DOWNLOAD_ERR;
      break;

    case SFU_STATE_DOWNLOAD_NEW_USER_FW:
      eExceptionId = SFU_EXCPT_DOWNLOAD_NEW_USER_FW_ERR;
      break;
#endif /* SECBOOT_USE_LOCAL_LOADER */

    case SFU_STATE_VERIFY_USER_FW_STATUS:
      eExceptionId = SFU_EXCPT_VERIFY_USER_FW_STATUS_ERR;
      break;

    case SFU_STATE_INSTALL_NEW_USER_FW:
      eExceptionId = SFU_EXCPT_INSTALL_NEW_USER_FW_ERR;
      break;

    case SFU_STATE_VERIFY_USER_FW_SIGNATURE:
      eExceptionId = SFU_EXCPT_VERIFY_USER_FW_SIGNATURE_ERR;
      break;

    case SFU_STATE_ROLLBACK_PREV_USER_FW:
      eExceptionId = SFU_EXCPT_ROLLBACK_PREV_USER_FW_ERR;
      break;

    case SFU_STATE_EXECUTE_USER_FW:
      eExceptionId = SFU_EXCPT_EXECUTE_USER_FW_ERR;
      break;

    default:
      /* Set an unknown error */
      eExceptionId = SFU_EXCPT_UNKNOWN;
      break;
  }

  if (SFU_EXCPT_NONE != u_specific_LastExecError)
  {
    /* The specific error cause set at error detection stage has priority over the exception computed from the FSM state */
    eExceptionId = (SFU_EXCPT_IdTypeDef)u_specific_LastExecError;
  } /* else go ahead with the exception derived from the FSM state */

  /* Take an immediate action */
  SFU_EXCPT_RuntimeExceptionHandler(eExceptionId);

  /* Try to store the error inside the BootInfo structure */
  (void)SFU_BOOT_SetLastExecError((uint32_t) eExceptionId);

}

/**
  * @brief  Cehck exception code among the known list
  * @param  eExceptionId : Exception ID.
  *         This parameter can be a value of @ref SFU_EXCPT_ID_Structure_definition.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */

SFU_ErrorStatus SFU_EXCPT_Check_Code(SFU_EXCPT_IdTypeDef eExceptionId)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;

  switch (eExceptionId)
  {
    case SFU_EXCPT_FIREWALL_RESET :
      break;
    case SFU_EXCPT_WATCHDOG_RESET :
      break;
    case SFU_EXCPT_MEMORY_FAULT :
      break;
    case SFU_EXCPT_HARD_FAULT :
      break;
    case SFU_EXCPT_TAMPERING_FAULT :
      break;
    case SFU_EXCPT_CHECK_PROTECTIONS_ERR :
      break;
    case SFU_EXCPT_CHECK_STATUS_ON_RESET_ERR :
      break;
#ifdef SECBOOT_USE_LOCAL_LOADER
    case SFU_EXCPT_CHECK_NEW_FW_TO_DOWNLOAD_ERR :
      break;
    case SFU_EXCPT_DOWNLOAD_NEW_USER_FW_ERR :
      break;
#endif /* SECBOOT_USE_LOCAL_LOADER */
    case SFU_EXCPT_VERIFY_USER_FW_STATUS_ERR :
      break;
    case SFU_EXCPT_DECRYPT_NEW_USER_FW_ERR :
      break;
    case SFU_EXCPT_INSTALL_NEW_USER_FW_ERR :
      break;
    case SFU_EXCPT_VERIFY_USER_FW_SIGNATURE_ERR :
      break;
    case SFU_EXCPT_ROLLBACK_PREV_USER_FW_ERR :
      break;
    case SFU_EXCPT_EXECUTE_USER_FW_ERR :
      break;
    case SFU_EXCPT_MAX_CONSEC_ERRORS_REACHED :
      break;
    case SFU_EXCPT_LOCK_SE_SERVICES_ERR :
      break;
#ifdef SECBOOT_USE_LOCAL_LOADER
    case SFU_EXCPT_INCONSISTENT_FW_SIZE :
      break;
    case SFU_EXCPT_FW_TOO_BIG :
      break;
    case SFU_EXCPT_COM_ERROR :
      break;
    case SFU_EXCPT_DOWNLOAD_ERROR :
      break;
    case SFU_EXCPT_HEADER_AUTH_FAILED :
      break;
#endif /* SECBOOT_USE_LOCAL_LOADER */
    case SFU_EXCPT_DECRYPT_FAILURE :
      break;
    case SFU_EXCPT_SIGNATURE_FAILURE :
      break;
    case SFU_EXCPT_INCORRECT_BINARY :
      break;
    case SFU_EXCPT_FLASH_ERROR :
      break;
    case SFU_EXCPT_FWIMG_MAGIC :
      break;
    case SFU_EXCPT_FWIMG_SWAP :
      break;
    case SFU_EXCPT_INCORRECT_VERSION :
      break;
    case SFU_EXCPT_UNKNOWN :
      break;
    default :
      break;
  }

  return e_ret_status;
}

/**
  * @}
  */


/** @defgroup SFU_EXCPT_Initialization_Functions Exceptions Initialization Functions
 *  @brief Initialize the part of the error handling dealing with exceptions.
 *         These exceptions are the second level of errors handling (to take immediate actions).
* @{
*/
/**
* @brief  SFU Exception Initialization.
* @param  None.
* @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
*/
SFU_ErrorStatus SFU_EXCPT_Init(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* ADD SRC CODE HERE
       ...
  */
  e_ret_status = SFU_SUCCESS;

  return e_ret_status;
}

/**
  * @brief  SFU Exception DeInitialization.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_EXCPT_DeInit(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  /* ADD SRC CODE HERE
      ...
  */
  e_ret_status = SFU_SUCCESS;

  return e_ret_status;
}
/**
  * @}
  */


/** @defgroup SFU_EXCPT_ExportedCtrl_Functions Exported Exceptions Control Functions
 *  @brief The reset exceptions are also handled at boot time when the last execution status is checked.
 * @{
 */

/**
  * @brief  Manage the Exception/Errors/Fault at reset,
  *         after reading the stored values related to the last execution errors/status
  * @param  eExceptionId : Exception ID.
  *         This parameter can be a value of @ref SFU_EXCPT_ID_Structure_definition.
  * @note   this function is exported because the reset exceptions are also handled at boot time
  *         when the last execution status is checked.
  * @retval None
  */
void SFU_EXCPT_ResetExceptionHandler(SFU_EXCPT_IdTypeDef eExceptionId)
{

  switch (eExceptionId)
  {
    case SFU_EXCPT_FIREWALL_RESET:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] FIREWALL RESET FAULT!");
      break;

    case SFU_EXCPT_WATCHDOG_RESET:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] WATCHDOG RESET FAULT!");
      break;

    default:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system.
         ...
         ...
      */
      /* As an example all the other errors/exceptions/fault retrieved at reset
         and related to the previous execution can be managed at the same way of the
         errors/exceptions/fault detected at runtime: See SFU_EXCPT_RuntimeExceptionHandler
      */
      break;
  }
}


/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup SFU_ERROR_Private_Functions
  * @{
  */

/** @defgroup SFU_EXCPT_Control_Functions Exceptions Control Functions
  * @brief Control functions for the Exceptions processing (taking immediate actions if required).
  * @{
  */

/**
  * @brief  Manage the Exception/Errors/Fault at runtime, when detected
  * @param  eExceptionId : Exception ID.
  *         This parameter can be a value of @ref SFU_EXCPT_ID_Structure_definition.
  * @note   A System Reset is forced at the end by the caller of this function.
  *         WARNING: Be aware that this function can be called by an IRQ while
  *         using the printf, since the printf is not a reentrant function, and using
  *         the printf inside this function as well, a fault will likely occur.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static void SFU_EXCPT_RuntimeExceptionHandler(SFU_EXCPT_IdTypeDef eExceptionId)
{
  switch (eExceptionId)
  {
    case SFU_EXCPT_TAMPERING_FAULT:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      /* WARNING: Please consider that at runt-time,  this "case" execution is
      inside an ISR */
      TRACE_IRQ((uint8_t *)"\r\n= [EXCPT] TAMPERING FAULT!");
      break;

    case SFU_EXCPT_MEMORY_FAULT:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      /* WARNING: Please consider that at run-time,  this "case" execution is
      inside an ISR */
      TRACE_IRQ((uint8_t *)"\r\n= [EXCPT] MEMORY FAULT!");
      break;

    case SFU_EXCPT_HARD_FAULT:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      /* WARNING: Please consider that at run-time,  this "case" execution is
      inside an ISR */
      TRACE_IRQ((uint8_t *)"\r\n= [EXCPT] HARD FAULT!");
#ifdef SFU_TEST_PROTECTION
      SFU_TEST_Error();
#endif /* SFU_TEST_PROTECTION */
      break;

    case SFU_EXCPT_CHECK_STATUS_ON_RESET_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
        Add your code here in order to implement a custom action for this event,
        e.g. trigger a mass erase or take any other action in order to
        protect your system.
        ...
        ...
      */
      TRACE("\r\n= [EXCPT] CHECK ON-RESET FAILED!");
      break;

#if defined(SECBOOT_USE_LOCAL_LOADER)
    case SFU_EXCPT_CHECK_NEW_FW_TO_DOWNLOAD_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] CHECK FOR NEW FW DOWNLOAD FAILED!");
      break;

    case SFU_EXCPT_DOWNLOAD_NEW_USER_FW_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] FW DOWNLOAD FAILED!");
      break;
#endif /* SECBOOT_USE_LOCAL_LOADER */

    case SFU_EXCPT_VERIFY_USER_FW_STATUS_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] NOT POSSIBLE TO CHECK THE FW STATUS. FAILURE!");
      break;

    case SFU_EXCPT_DECRYPT_NEW_USER_FW_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] USER FW DECRYPTION FAILED!");
      break;

    case SFU_EXCPT_INSTALL_NEW_USER_FW_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] USER FW INSTALLATION FAILED!");
      break;

    case SFU_EXCPT_VERIFY_USER_FW_SIGNATURE_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] USER FW CANNOT BE VERIFIED. FAILURE!");
      break;

    case SFU_EXCPT_ROLLBACK_PREV_USER_FW_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] ROLLBACK FROM PREV FW CANNOT BE EXECUTED. FAILURE!");
      break;

    case SFU_EXCPT_EXECUTE_USER_FW_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] NOT POSSIBLE TO EXECUTE THE NEW FW. FAILURE!");
      break;

    case SFU_EXCPT_UNKNOWN:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] UNKNOWN FAILURE!");
      break;

    case SFU_EXCPT_MAX_CONSEC_ERRORS_REACHED:
    {
      /*
       * In this example we decide to trigger a rollback to a previous firmware version if one is available in slot #1.
       * Otherwise the current Firmware in slot #0 must at least be invalidated.
       * But this is only as example and this error handling must be revisited
       * or replaced by your code in order to implement a custom action for this event.
       */
      TRACE("\r\n= [EXCPT] MAX. NB. CONSECUTIVE ERRORS!");


      /* The checks below must be adapted to match the chosen strategy */
      if (SFU_SUCCESS == SFU_IMG_CheckRollbackConditions())
      {
        /* There is a possibility to trigger a rollback to a backed-up FW version */
        TRACE("\r\n= [EXCPT] RollBack triggered!");
        SFU_LL_SECU_IWDG_Refresh();
        /*
         * Please note that if the procedure below is interrupted (by a switch-off or a reset...)
         * then it is not resumed:
         *   - either we end-up in local download state if the rollback had started,
         *   - or we restart with the active FW and the consecutive counter set to 0.
         */
        if (SFU_SUCCESS == SFU_IMG_TriggerRollbackProcedure())
        {
          TRACE("\r\n= [EXCPT] RollBack succeeded!");
        }
        else
        {
          TRACE("\r\n= [EXCPT] RollBack failure!");
        }
      }
      else
      {
        /*
         * No valid FW version available in the backup slot so we can only invalidate the current firmware.
         * This means that the device ends up in local download state.
         */
        TRACE("\r\n= [EXCPT] No rollback possible, invalidating the faulty FW!");
        (void)SFU_IMG_InvalidateCurrentFirmware(); /* If this fails we continue anyhow */
      }
    }
    break;

    case SFU_EXCPT_LOCK_SE_SERVICES_ERR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] CANNOT CONFIGURE SECURE ENGINE TO RUN THE FIRMWARE!");
      break;

#if defined(SECBOOT_USE_LOCAL_LOADER)
    case SFU_EXCPT_INCONSISTENT_FW_SIZE:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] INCONSISTENT FIRMWARE SIZE!");
      break;

    case SFU_EXCPT_FW_TOO_BIG:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] BINARY IMAGE TOO BIG TO BE STORED IN SLOT#1!");
      break;

    case SFU_EXCPT_COM_ERROR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] COM ERROR DURING DOWNLOAD !");
      break;

    case SFU_EXCPT_DOWNLOAD_ERROR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] DOWNLOAD ERROR!");
      break;

    case SFU_EXCPT_HEADER_AUTH_FAILED:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] HEADER AUTHENTICATION FAILURE!");
      break;
#endif /* SECBOOT_USE_LOCAL_LOADER */

    case SFU_EXCPT_DECRYPT_FAILURE:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] DECRYPT FAILURE!");
      break;

    case SFU_EXCPT_SIGNATURE_FAILURE:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] SIGNATURE CHECK FAILED!");
      break;

    case SFU_EXCPT_INCORRECT_BINARY:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] INCORRECT BINARY IMAGE!");
      break;

    case SFU_EXCPT_FLASH_ERROR:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] FLASH ERROR!");
      break;

    case SFU_EXCPT_FWIMG_MAGIC:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] INTERNAL FWIMG ISSUE!");
      break;

    case SFU_EXCPT_FWIMG_SWAP:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] NOT POSSIBLE TO SWAP THE IMAGES!");
      break;

    case SFU_EXCPT_INCORRECT_VERSION:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] INSTALLATION CANCELLED: FORBIDDEN VERSION!");
      break;

    default:
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system.
         ...
         ...
      */
      TRACE("\r\n= [EXCPT] UNKNOWN FAILURE!");
      break;
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

/**
  * @}
  */

#if defined (SFU_ERROR_C)
#undef SFU_ERROR_C
#endif /* SFU_ERROR_C */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
