/**
  ******************************************************************************
  * @file    sfu_boot.c
  * @author  MCD Application Team
  * @brief   SFU BOOT module
  *          This file provides firmware functions to manage the following
  *          functionalities of the Secure Boot:
  *           + Initialization and de-initialization functions
  *           + Secure Boot Control functions
  *           + Secure Boot State functions
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

#define SFU_BOOT_C

/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "sfu_boot.h"
#include "sfu_loader.h"
#include "sfu_low_level_security.h"
#include "sfu_low_level_flash.h"
#include "sfu_low_level.h"
#include "sfu_fsm_states.h"
#include "sfu_error.h"
#include "stm32l4xx_it.h" /* required for the HAL Cube callbacks */


/*
 * The sfu_com init is provided by the sfu_com_trace module by default.
 * If not, then it is taken from the sfu_com_loader module.
 */
#if defined(SFU_DEBUG_MODE)
#include "sfu_trace.h"
#else
#include "sfu_trace.h"      /* needed anyhow even if the defines will be empty */
#include "sfu_com_loader.h" /* needed only for the COM init/de-init */
#endif /* SFU_DEBUG_MODE */
#include "se_def.h"
#include "se_bootinfo.h"
#include "se_interface_bootloader.h"  /* sfu_boot is the bootloader core part */
#include "sfu_new_image.h"            /* the local loader is a kind of "application" running in SB_SFU so it needs the services to install a FW image */
#include "sfu_fwimg_services.h"       /* sfu_boot uses the services of the FWIMG module */
#include "sfu_test.h"                 /* auto tests */

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup SFU_BOOT SB Secure Boot
  * @brief This file provides a set of firmware functions to manage the Secure Boot state machine.
  *      It will go through all the Secure Boot and the Secure Fw Update states without compromising
  *        the Root of Trust.
  * @{
  */


/** @defgroup SFU_BOOT_Private_Defines Private Defines
  * @{
  */

/**
  * @defgroup SFU_BOOT_Private_Defines_Execution_Id Execution ID
  * @brief Identifier for Running Code (Secure Boot or User Application)
  * @{
  */
#define EXEC_ID_SECURE_BOOT     0U       /*!< ID for Secure Boot */
#define EXEC_ID_USER_APP        1U       /*!< ID for User App */
#define IS_VALID_EXEC_ID(EXEC_ID)        (((EXEC_ID) == EXEC_ID_SECURE_BOOT) || \
                                          ((EXEC_ID) == EXEC_ID_USER_APP)) /*!< Check for valid ID */
/**
  * @}
  */

#define RESERVED_VALUE (0xFE) /*!< Reserved value. The reserved field used inside the LastExecStatus of the BootInfo is maintained for future customization/expansion of the field itself */
#define AES_BLOCK_SIZE (16U)  /*!< Size of an AES block to check FW image alignment */

/**
  * @}
  */

/** @defgroup SFU_BOOT_Private_Macros Private Macros
  * @{
  */
#define SFU_SET_SM_IF_CURR_STATE(Status, SM_STATE_OK, SM_STATE_FAILURE) \
do{                                                                     \
  m_StateMachineContext.PrevState = m_StateMachineContext.CurrState;    \
    if (Status){                                                        \
      m_StateMachineContext.CurrState = SM_STATE_OK;                    \
    }                                                                   \
    else {                                                              \
      m_StateMachineContext.CurrState = SM_STATE_FAILURE;               \
    }                                                                   \
}while(0) /*!< Set a State Machine state according to the 'Status' value*/


#define SFU_SET_SM_CURR_STATE(NewState)                               \
do{                                                                   \
  m_StateMachineContext.PrevState = m_StateMachineContext.CurrState;  \
    m_StateMachineContext.CurrState = NewState;                       \
}while(0) /*!< Set a State Machine state*/


/**
  * @defgroup SFU_BOOT_Private_Macros_Execution_Status Execution Status
  * @brief Execution State helper functions for SB State Machine
  * @{
  */
#define SFU_GET_LAST_EXEC_STATE(STATUS)         (((STATUS)>>8U) & 0xFFU)          /*!< Get last State Machine state*/
#define SFU_GET_LAST_EXEC_IMAGE_ID(STATUS)      (((STATUS)>>16U) & 0xFFU)         /*!< Get image ID*/
#define SFU_GET_LAST_EXEC_ID(STATUS)            (((STATUS)>>24U)  & 0xFFU)        /*!< Get Execution ID*/
#define SFU_SET_LAST_EXEC_STATUS(STATE,IMAGE_ID,EXEC_ID)  (RESERVED_VALUE | \
                                                          ((STATE) << 8U) | ((IMAGE_ID) << 16U) | ((EXEC_ID) << 24U))    /*!< Set a State Machine state*/
/**
  * @}
  */


/**
  * @}
  */

/** @defgroup SFU_BOOT_Private_Variables Private Variables
  * @brief variables used by sfu_boot.c only (static).
  * @{
  */

#define SFU_STATE_INITIAL     SFU_STATE_CHECK_STATUS_ON_RESET  /*!< Define the initial state*/

typedef struct
{
  SFU_BOOT_StateMachineTypeDef  PrevState;      /*!< The previous state of the State Machine */
  SFU_BOOT_StateMachineTypeDef  CurrState;      /*!< The current state of the State Machine */
} SFU_BOOT_StateMachineContextTypeDef;  /*!< Specifies a structure containing the State Machine context information using during the SM evolution. */



static volatile SFU_BOOT_StateMachineContextTypeDef m_StateMachineContext = {SFU_STATE_INITIAL,
                                                                             SFU_STATE_INITIAL
                                                                            }; /*!< Static member variables representing the StateMachine context used during the StateMachine evolution. */
/**
  * @}
  */

/** @defgroup SFU_BOOT_Private_Functions Private Functions
  * @brief Functions used internally by sfu_boot.c
  *        All these functions should be declared as static.
  * @{
  */

/** @defgroup SFU_BOOT_Initialization_Functions Initialization Functions
  * @brief Bootloader Init/Deinit functions and BSP configuration.
  * @{
  */

static SFU_ErrorStatus SFU_BOOT_Init(void);
static SFU_ErrorStatus SFU_BOOT_DeInit(void);
static void SFU_BOOT_BspConfiguration(void);

/**
  * @}
  */

/** @defgroup SFU_BOOT_SM_Functions State Machine Functions
  * @brief The bootloader is implemented as a Finite State Machine.
  * @{
  */
static SFU_ErrorStatus SFU_BOOT_SM_Run(void);
static void SFU_BOOT_SM_CheckStatusOnReset(void);
#if defined(SECBOOT_USE_LOCAL_LOADER)
static void SFU_BOOT_SM_CheckNewFwToDownload(void);
static void SFU_BOOT_SM_DownloadNewUserFw(void);
#endif /* SECBOOT_USE_LOCAL_LOADER */
static void SFU_BOOT_SM_CheckUserFwStatus(void);
static void SFU_BOOT_SM_VerifyUserFwSignature(void);
static void SFU_BOOT_SM_ExecuteUserFw(void);
static void SFU_BOOT_SM_HandleCriticalFailure(void);
static void SFU_BOOT_SM_RebootStateMachine(void);
static void SFU_BOOT_SM_InstallNewUserFw(void);
static void SFU_BOOT_SM_RollbackPrevUserFw(void);


/**
  * @brief  Initialize the State Machine:
  * @note: Functions listed in the array and @ref SFU_BOOT_State_Machine_Structure_definition must be in sync!
  */
void (* fnStateMachineTable[])(void) = {SFU_BOOT_SM_CheckStatusOnReset,
#if defined(SECBOOT_USE_LOCAL_LOADER)
                                        SFU_BOOT_SM_CheckNewFwToDownload,
                                        SFU_BOOT_SM_DownloadNewUserFw,
#endif /* SECBOOT_USE_LOCAL_LOADER */
                                        SFU_BOOT_SM_CheckUserFwStatus,
                                        SFU_BOOT_SM_InstallNewUserFw,
                                        SFU_BOOT_SM_VerifyUserFwSignature,
                                        SFU_BOOT_SM_ExecuteUserFw,
                                        SFU_BOOT_SM_RollbackPrevUserFw,
                                        SFU_BOOT_SM_HandleCriticalFailure,
                                        SFU_BOOT_SM_RebootStateMachine
                                       };


/**
  * @}
  */

/** @defgroup SFU_BOOT_Security_Functions Security Functions
  * @brief Functions handling the security protections.
  * @{
  */
static int SFU_BOOT_SystemSecurity_Config(void);
static SFU_ErrorStatus SFU_BOOT_CheckApplySecurityProtections(void);
static SFU_ErrorStatus SFU_BOOT_SecuritySafetyCheck(void);
/**
  * @}
  */

/** @defgroup SFU_BOOT_ExecStatus_Functions Execution Status Functions
  * @brief Functions dealing with the execution status.
  * @{
  */
static SFU_ErrorStatus SFU_BOOT_SetLastExecStatus(uint8_t uExecID, uint8_t uLastExecState);
static SFU_ErrorStatus SFU_BOOT_ManageLastExecStatus(uint32_t uLastExecStatus);
static SFU_ErrorStatus SFU_BOOT_ManageResetSources(void);

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup SFU_BOOT_EntryPoint_Function Bootloader Entry Point
  * @brief This function is the entry point of the secure bootloader service.
  * @{
  */

/**
 * @brief This function starts the secure boot service and returns only if a configuration issue occurs.
 *        In the nominal case, the bootloader service runs until the user application is launched.
 *        When no valid user application can be run (after installing a new image or not),
 *        if the local loader feature is not enabled then the execution stops,
 *        otherwise a local download will be awaited.
 *        If the state machine encounters a major issue then a reboot is triggered.
 * @param None.
 * @note Please note that this service initializes all the required sub-services and rely on them to perform its tasks.
 * @note Constraints
 *       1. The system initialization must be completed (HAL, clocks, peripherals...) before calling this function.
 *       2. This function also takes care of BSP initialization after enabling the secure mode.
 *          The BSP init code can be added in @ref SFU_BOOT_BspConfiguration().
 *       3. No other entity should handle the initialization of the Secure Engine.
 *       4. The other SB_SFU services should NOT be configured by other entities if this service is used (the previous configurations will be overwritten).
 *       5. The other SB_SFU services should NOT be used by any other entity if this service is running.
 *       6. When returning from this function a reboot should be triggered (NVIC_SystemReset) after processing the error cause.
 *       7. The caller must be prepared to never get the hand back after calling this function (jumping in user application by default or entering local loader state if local loader is enabled or rebooting to install a new image).
 * @note Settings are handled at compilation time:
 *       1. See compiler switches in main.h for secure IPs settings
 *       2. Errors handling settings are not passed as parameters but tuned in main.h (@ref SFU_MAX_CONSEC_COUNTER ...)
 *       3. The trace system is configured in the sfu_trace.h file
 * @retval SFU_BOOT_InitErrorTypeDef error code as the function returns only if a critical failure occurs at init stage.
 */
SFU_BOOT_InitErrorTypeDef SFU_BOOT_RunSecureBootService()
{
  SFU_BOOT_InitErrorTypeDef e_ret_code = SFU_BOOT_INIT_ERROR;

  /* initialize Secure Engine variable as secure Engine is managed as a completely separate binary - not "automatically" managed by SBSFU compiler command */
  if (SE_Startup() == SE_SUCCESS)
  {
    /* Security Configuration---------------------------------------------------*/
    if (SFU_BOOT_SystemSecurity_Config())
    {
      /* Board BSP  Configuration-------------------------------------------------*/
      SFU_BOOT_BspConfiguration();

      /* Configure the Secure Boot and start the State machine--------------------*/
      /* Initialize the Secure Boot */
      if (SFU_BOOT_Init() == SFU_SUCCESS)
      {
        /* Start the Secure Boot State Machine */
        SFU_BOOT_SM_Run();
      }
      else
      {
        /* failure when initializing the secure boot service  */
        e_ret_code = SFU_BOOT_INIT_FAIL;
      }
    }
    else
    {
      /* failure when configuring the security IPs  */
      e_ret_code = SFU_BOOT_SECIPS_CFG_FAIL;
    }
  }
  else
  {
    /* failure at secure engine initialization stage */
    e_ret_code = SFU_BOOT_SECENG_INIT_FAIL;
  }

  /*
   * This point should not be reached unless a critical init failure occurred
   * Return the error code
   */
  return (e_ret_code);
}

/**
  * @}
  */


/** @defgroup SFU_BOOT_ShutdownOnError_Function Shutdown On Error
  * @brief Shutting down the bootloader and the system when a critical error occurs during the bootloader execution.
  * @{
  */

/**
  * @brief  Force System Reboot
  * @param  None
  * @retval None
  */
void SFU_BOOT_ForceReboot(void)
{
  /* WARNING: The follow TRACEs are for debug only. This function could be called
  inside an IRQ so the below printf could not be executed or could generate a fault! */
  TRACE("\r\n========= End of Execution ==========");
  TRACE("\r\n\r\n\r\n");

  /* This is the last operation executed. Force a System Reset. */
  NVIC_SystemReset();
}


/**
  * @}
  */

/** @addtogroup SFU_BOOT_Private_Functions
  * @{
  */

/** @addtogroup SFU_BOOT_Initialization_Functions Initialization Functions
  * @{
  */

/**
  * @brief  Initialize the Secure Boot State machine.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_BOOT_Init(void)
{
  SFU_ErrorStatus  e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status;

  /* We start the execution at boot-up (display all messages in teraterm console, check the trigger to force a local download)*/
  initialDeviceStatusCheck = 1U;

  /* Call the Hardware Abstraction Layer Init implemented for the specific MCU */
  if (SFU_LL_Init() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }

  /* The COM modules is required only if the trace or the local download is enabled */
#if defined(SECBOOT_USE_LOCAL_LOADER) || defined(SFU_DEBUG_MODE)
  /* Call the COM module Init (already handled in SFU_BOOT_SystemSecurity_Config) */
  if (SFU_COM_Init() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
#endif /* SECBOOT_USE_LOCAL_LOADER || SFU_DEBUG_MODE */

#if defined(SECBOOT_USE_LOCAL_LOADER)
  /* Call the SFU_LOADER module Init */
  if (SFU_LOADER_Init() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
#endif /* SECBOOT_USE_LOCAL_LOADER */

  /* Call the Exception module Init */
  if (SFU_EXCPT_Init() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }

  /* Call the 2 images handling Init  */
  if (SFU_IMG_INIT_OK != SFU_IMG_InitImageHandling())
  {
    return SFU_ERROR;
  } /* else continue */

#ifdef SFU_TEST_PROTECTION
  SFU_TEST_Init();
#endif /* SFU_TEST_PROTECTION */

  TRACE("\r\n\r\n");
  TRACE("\r\n======================================================================");
  TRACE("\r\n=              (C) COPYRIGHT 2017 STMicroelectronics                 =");
  TRACE("\r\n=                                                                    =");
  TRACE("\r\n=              Secure Boot and Secure Firmware Update                =");
  TRACE("\r\n======================================================================");
  TRACE("\r\n\r\n");

  /* Initialize the Secure Engine that will be used for all the most critical operations */
  if (SE_Init(&e_se_status, SystemCoreClock) != SE_SUCCESS)
  {
    TRACE("\r\n= [SBOOT] SECURE ENGINE INITIALIZATION CRITICAL FAILURE!");
  }
  else
  {
    /* Check the Secure Engine Status in order to get more information */
    if (e_se_status == SE_BOOT_INFO_ERR_FACTORY_RESET)
    {
      TRACE("\r\n= [SBOOT] STATE: WARNING: SECURE ENGINE INITIALIZATION WITH FACTORY DEFAULT VALUES!");
      /* Please add your custom action here */
      /* ... */
    }
    else
    {
      e_ret_status = SFU_SUCCESS;
      TRACE("\r\n= [SBOOT] SECURE ENGINE INITIALIZATION SUCCESSFUL");
    }
    /*
     * The BootInfo consecutive boot on error counter will be handled in SFU_BOOT_ManageResetSources() to decide if it is incremented or reset.
     * This counter is used to count the number of consecutive resets triggered by an error (in the User Application or in SB_SFU when dealing with the user application management).
     * So this counter is reset as soon as a normal power off/power on boot is performed.
     */
  }

  return e_ret_status;
}

/**
  * @brief  DeInitialize the Secure Boot State machine.
  * @param  None
  * @note   Please note that in this example the de-init function is used only once to avoid a compiler warning.
  *         The bootloader can terminate:
  *         1. with an init failure : no de-init needed
  *         2. with a critical failure leading to a reboot: no de-init needed as long as no persistent info is stored by this function.
  *         3. when launching the user app: de-init may be called here if required as long as it does not disengage the required security mechanisms.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_BOOT_DeInit(void)
{
  if (SFU_EXCPT_DeInit() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }

#if defined(SECBOOT_USE_LOCAL_LOADER)
  if (SFU_LOADER_DeInit() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
#endif /* SECBOOT_USE_LOCAL_LOADER */

#if defined(SECBOOT_USE_LOCAL_LOADER) || defined(SFU_DEBUG_MODE)
  if (SFU_COM_DeInit() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
#endif /* SECBOOT_USE_LOCAL_LOADER || SFU_DEBUG_MODE */

  if (SFU_LL_DeInit() != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }

  /* Call the 2 images handling De-Init  */
  if (SFU_SUCCESS != SFU_IMG_ShutdownImageHandling())
  {
    return SFU_ERROR;
  } /* else continue */

  return SFU_SUCCESS;
}

/**
  * @brief  BSP Initialization.
  *         Called when the secure mode is enabled.
  * @note   The BSP configuration should be handled only in this function.
  * @param  None
  * @retval None
  */
static void SFU_BOOT_BspConfiguration()
{
  /* LED Init*/
  BSP_LED_Init(SFU_STATUS_LED);

#if defined(SECBOOT_USE_LOCAL_LOADER)
  /* User Button */
  BUTTON_INIT();
#endif /* SECBOOT_USE_LOCAL_LOADER */
}


/**
  * @}
  */



/** @addtogroup SFU_BOOT_SM_Functions
  * @{
  */

/**
  * @brief  Execute the Secure Boot state machine.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_BOOT_SM_Run(void)
{
  SFU_ErrorStatus  e_ret_status = SFU_SUCCESS;
  void (*fnStateMachineFunction)(void);


  /* Start the State Machine loop/evolution */
  while (e_ret_status == SFU_SUCCESS)
  {
    /* Always execute a security/safety check before moving to the next state */
    if (SFU_BOOT_SecuritySafetyCheck() == SFU_SUCCESS)
    {
      /* Except for the 1st state, used to check the last saved execution status,
        let's save the current Execution Status in order to be retrieved and analyzed
        in case a reboot will be triggered by an error, bug, power-off, Hw reset, etc. */
      if (m_StateMachineContext.CurrState != SFU_STATE_CHECK_STATUS_ON_RESET)
      {
        SFU_BOOT_SetLastExecStatus(EXEC_ID_SECURE_BOOT, m_StateMachineContext.CurrState);
      }

      /* Get the right StateMachine function according to the current state */
      fnStateMachineFunction = fnStateMachineTable[m_StateMachineContext.CurrState];

      /* Call the State Machine function associated to the current state */
      fnStateMachineFunction();
    }
    else
    {
      e_ret_status = SFU_ERROR;
    }
  }
  /* If the State Machine cannot evolve anymore, reboot is the only option */
  if (e_ret_status != SFU_SUCCESS)
  {
    /* Set the error before forcing a reboot */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_UNKNOWN);

    /* This is the last operation executed. Force a System Reset */
    SFU_BOOT_ForceReboot();
  }

  return e_ret_status;
}



/** @brief  Check the Reset status in order to understand the last cause of Reset
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_CheckStatusOnReset(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef         e_se_status;
  SE_BootInfoTypeDef       x_boot_info;
  uint32_t                 u_last_exec_status;

  TRACE("\r\n= [SBOOT] STATE: CHECK STATUS ON RESET");

  /* First of all, locally get the last execution status we'll work on, before setting the new state */
  if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
  {
    u_last_exec_status = x_boot_info.LastExecStatus;

    /* Try to save here the current Execution Status in order to be retrieved and analyzed in case a reboot will be triggered by an error, bug, power-off, Hw reset, etc. */

    if (SFU_BOOT_SetLastExecStatus(EXEC_ID_SECURE_BOOT, m_StateMachineContext.CurrState) == SFU_SUCCESS)
    {
      /* Check the wakeup sources */
      if (SFU_BOOT_ManageResetSources() == SFU_SUCCESS)
      {
        /* Check the last execution status and take the opportune actions */
        if (SFU_BOOT_ManageLastExecStatus(u_last_exec_status) == SFU_SUCCESS)
        {
          if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
          {
            /*
             * Check if counters limits have been reached and in this case take an action
             */
            if (x_boot_info.ConsecutiveBootOnErrorCounter >= SFU_MAX_CONSEC_COUNTER)
            {
#ifdef SFU_DEBUG_MODE
              TRACE("\r\n\t  CRITICAL: Max number of consecutive errors reached:#%d/%d. Current FW will be invalidated!", x_boot_info.ConsecutiveBootOnErrorCounter, SFU_MAX_CONSEC_COUNTER);
#endif /* SFU_DEBUG_MODE */
              /* Memorize the error here */
              (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_MAX_CONSEC_ERRORS_REACHED);
              e_ret_status = SFU_ERROR; /* enter SFU_STATE_HANDLE_CRITICAL_FAILURE to process this error fully */
            }
            else
            {
              e_ret_status = SFU_SUCCESS;
            }
          } /* else SE_INFO_ReadBootInfo fails: go forward as we end up with a critical failure anyway */
        } /* else SFU_BOOT_ManageLastExecStatus fails: go forward as we end up with a critical failure anyway */
      } /* else SFU_BOOT_ManageResetSources fails: go forward as we end up with a critical failure anyway */
    } /* else SFU_BOOT_SetLastExecStatus fails: go forward as we end up with a critical failure anyway */
  } /* else SE_INFO_ReadBootInfo fails: go forward as we end up with a critical failure anyway */

  /* Set the next State Machine state according to the success of the failure of e_ret_status */
#if defined(SECBOOT_USE_LOCAL_LOADER)
  /* When the local loader feature is supported we need to check if a local download is requested */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_CHECK_NEW_FW_TO_DOWNLOAD, SFU_STATE_HANDLE_CRITICAL_FAILURE);
#else
  /* When the local loader feature is disabled go directly to the check of the FW status */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_VERIFY_USER_FW_STATUS, SFU_STATE_HANDLE_CRITICAL_FAILURE);
#endif /* SECBOOT_USE_LOCAL_LOADER */
}


#if defined(SECBOOT_USE_LOCAL_LOADER)
/**
  * @brief  Check if a new UserApp firmware is available for downloading.
  *         When entering this state from SFU_STATE_CHECK_STATUS_ON_RESET (initialDeviceStatusCheck=1) it is required to press the user button to force the local download.
  *         When entering this state from SFU_STATE_VERIFY_USER_FW_STATUS the local download is awaited automatically (because there is no other action to do).
  * @param  None
  * @note   This function must set the next State Machine State.
  * @retval None
  */
static void SFU_BOOT_SM_CheckNewFwToDownload(void)
{
  SFU_ErrorStatus  e_ret_status = SFU_ERROR;

  if (initialDeviceStatusCheck == 1)
  {
    /* At boot-up, before checking the FW status, a local download can be forced thanks to the user button */
    TRACE("\r\n= [SBOOT] STATE: CHECK NEW FIRMWARE TO DOWNLOAD");
    /* Change this code to implement another local download trigger */
    if (0 != SFU_LOADER_CheckLocalDownloadReq())
    {
      /* Download requested */
      e_ret_status = SFU_SUCCESS;
    }
  }
  else
  {
    /* The FW status has already been checked and no FW can be launched: no need to check the trigger, wait for a local download to start */
    e_ret_status = SFU_SUCCESS;
  }

  /* Set the next State Machine state according to the success of the failure of e_ret_status */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_DOWNLOAD_NEW_USER_FW, SFU_STATE_VERIFY_USER_FW_STATUS);
}
#endif /* SECBOOT_USE_LOCAL_LOADER */

/**
  * @brief  Check the Status of the Fw Image to work on  in order to set the next
  *         State Machine state accordingly
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_CheckUserFwStatus(void)
{
  SFU_IMG_ImgInstallStateTypeDef e_PendingInstallStatus;
  SFU_ErrorStatus e_HasValidFw;

  if (initialDeviceStatusCheck == 1U)
  {
    TRACE("\r\n= [SBOOT] STATE: CHECK USER FW STATUS");
  }

  /*
   * 2 FW images implementation: the status is computed directly from the FLASH state.
   */

  /* Check if there is a pending action related to a FW update procedure */
  e_PendingInstallStatus = SFU_IMG_CheckPendingInstallation();

  switch (e_PendingInstallStatus)
  {
    case SFU_IMG_FWUPDATE_STOPPED:
      /* The installation of a downloaded FW has been interrupted */
      /* We perform a recovery: recovery of the previous version without preserving the downloaded FW */
      TRACE("\r\n\t  Downloaded Fw installation Failed, Rollback available: recovery procedure initiated");
      SFU_SET_SM_CURR_STATE(SFU_STATE_ROLLBACK_PREV_USER_FW);
      break;

    case SFU_IMG_FWIMAGE_TO_INSTALL:
#if (SFU_IMAGE_PROGRAMMING_TYPE == SFU_ENCRYPTED_IMAGE)
      /* A new encrypted FW is present in the slot #1 and ready to be installed: to be decrypted and swapped with the active FW */
      TRACE("\r\n\t  New Fw Encrypted, to be decrypted");
#elif (SFU_IMAGE_PROGRAMMING_TYPE == SFU_CLEAR_IMAGE)
      /* A new clear FW is present in the slot #1 and ready to be installed: to be reorganized in flash and swapped with the active FW */
      TRACE("\r\n\t  New Clear Fw, to be re-ordered in FLASH as expected by the swap procedure");
#else
#error "The current example does not support the selected Firmware Image Programming type."
#endif
      SFU_SET_SM_CURR_STATE(SFU_STATE_INSTALL_NEW_USER_FW);
      break;

    case SFU_IMG_NO_FWUPDATE:
      /*
       * No FW image installation pending:
       * check if there is a valid active Firmware already installed
       */
      e_HasValidFw = SFU_IMG_HasValidActiveFirmware();

      if (SFU_SUCCESS == e_HasValidFw)
      {
        TRACE("\r\n\t  A valid FW is installed in the active slot - version: %d", SFU_IMG_GetActiveFwVersion());
        /* Even if already verified in the past, the fw has to be verified before being executed */
        SFU_SET_SM_CURR_STATE(SFU_STATE_VERIFY_USER_FW_SIGNATURE);
      }
      else
      {
        /*
         * No valid FW is present in the active slot
         * and there is no FW to be installed in UserApp download area: local download (when possible)
         */
        if (1U == initialDeviceStatusCheck)
        {
          TRACE("\r\n\t  No valid FW found in the active slot nor new encrypted FW found in the UserApp download area");
#if defined(SECBOOT_USE_LOCAL_LOADER)
          /* Waiting for a local download is automatic, no trigger required. */
          TRACE("\r\n\t  Waiting for the local download to start... ");
#endif /* SECBOOT_USE_LOCAL_LOADER */
          initialDeviceStatusCheck = 0U;
#ifdef SFU_TEST_PROTECTION
          SFU_TEST_Reset();
#endif /* SFU_TEST_PROTECTION */
        }
#if defined(SFU_VERBOSE_DEBUG_MODE)
        else
        {
          /*
           * No ELSE branch (except for verbose debug), because the FW status is checked only once per boot:
           *   If a FW is present in the active slot => it is checked then launched.
           *   If there is a FW to install => the installation procedure starts.
           *   If no FW is present and no installation is pending:
           *      - if the local loader feature is enabled we enter the local download state
           *      - if the local loader feature is disabled, the execution is stopped.
           */
          TRACE("Abnormal case: SFU_STATE_VERIFY_USER_FW_STATUS is not supposed to be entered more than once per boot.");
        }
#endif /* SFU_VERBOSE_DEBUG_MODE */
#if defined(SECBOOT_USE_LOCAL_LOADER)
        SFU_SET_SM_CURR_STATE(SFU_STATE_CHECK_NEW_FW_TO_DOWNLOAD);
        /*
         * In this example, if there is no active FW, we do not try to re-install a backed-up FW from slot #1
         * even if it is available. This could be done to improve the handling of the corrupted FW case.
         * With the current implementation, when the active FW is corrupted we end up in local download state.
         */
#else
        /*
         * When the local loader feature is disabled it is not possible to enter the local download state.
         * Rebooting automatically or keeping on checking the FW status would not necessarily be better.
         * So we end up waiting for the user to reboot (or the IWDG to expire).
         */
        TRACE("No valid FW and no local loader: execution stopped.\r\n");
        while (1)
        {
          BSP_LED_Toggle(SFU_STATUS_LED);
          HAL_Delay(SFU_STOP_NO_FW_BLINK_DELAY);
        }
#endif /* SECBOOT_USE_LOCAL_LOADER */
      }
      break;

    default:
      TRACE("\r\n\t  Flash State Unknown, Critical failure");
      /* If not in one of the previous state, something bad occurred */
      SFU_SET_SM_CURR_STATE(SFU_STATE_HANDLE_CRITICAL_FAILURE);
      break;
  }
}


#if defined(SECBOOT_USE_LOCAL_LOADER)
/**
  * @brief  A new UserApp Fw was available. Start to download it
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_DownloadNewUserFw(void)
{
  SFU_ErrorStatus           e_ret_status = SFU_ERROR;
  SFU_LOADER_StatusTypeDef  e_ret_status_app = SFU_LOADER_ERR;
  SE_FwRawHeaderTypeDef     x_fw_raw_header;
  SFU_FwImageFlashTypeDef    x_fw_image_flash_data;
  uint32_t u_size = 0;

  TRACE("\r\n= [SBOOT] STATE: DOWNLOAD NEW USER FIRMWARE");

  /*
   * Get the download area parameters.
   * We use the same generic function as the UserApp to avoid maintaining equivalent information.
   * In the current example with 2 images:
   *     x_fw_image_flash_data.DownloadAddr = SFU_IMG_SLOT_1_REGION_BEGIN_VALUE;
   *     x_fw_image_flash_data.MaxSizeInBytes = (uint32_t)SFU_IMG_SLOT_1_REGION_SIZE;
   *     x_fw_image_flash_data.ImageOffsetInBytes = SFU_IMG_IMAGE_OFFSET;
   */
  if (SFU_IMG_GetDownloadAreaInfo(&x_fw_image_flash_data) != SFU_ERROR)
  {
    /*
     * Download of the new firmware image : We erase the entire slot 1 to be able to do a local download at any point
     * in time of the device's life.
     */
    e_ret_status = SFU_LOADER_DownloadNewUserFw(&e_ret_status_app, &x_fw_image_flash_data, &u_size);
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Read header in slot 1 */
      SFU_LL_FLASH_Read((void *) &x_fw_raw_header, (uint32_t *) x_fw_image_flash_data.DownloadAddr, sizeof(x_fw_raw_header));

#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  FwSize=%d | %d bytes received", x_fw_raw_header.FwSize, u_size);
#endif /* SFU_VERBOSE_DEBUG_MODE */
      /*
       * Notify the Secure Boot that a new image has been downloaded
       * by calling the SE interface function to trigger the installation procedure at next reboot.
       */
      if (SFU_ERROR == SFU_IMG_InstallAtNextReset((uint8_t *) &x_fw_raw_header))
      {
        /* no specific error cause set */
#if defined(SFU_VERBOSE_DEBUG_MODE)
        TRACE("\r\n\t  Cannot memorize that a new image has been downloaded.");
#endif /* SFU_VERBOSE_DEBUG_MODE */
      } /* else continue */
    }
    else
    {
      /* Memorize the specific error cause if any before handling this critical failure */
      switch (e_ret_status_app)
      {
        case SFU_LOADER_ERR_COM:
          (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_COM_ERROR);
          break;
        case SFU_LOADER_ERR_DOWNLOAD:
          (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_DOWNLOAD_ERROR);
          break;
        case SFU_LOADER_ERR_CRYPTO:
          (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_DECRYPT_FAILURE);
          break;
        default:
          /* no specific error cause */
          break;
      }
    }
  } /* else error with no specific error cause */

  /* Set the next State Machine state according to the success of the failure of e_ret_status */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_REBOOT_STATE_MACHINE, SFU_STATE_HANDLE_CRITICAL_FAILURE);
}
#endif /* SECBOOT_USE_LOCAL_LOADER */

/**
  * @brief  Install the new UserApp Fw
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_InstallNewUserFw(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  TRACE("\r\n= [SBOOT] STATE: INSTALL NEW USER FIRMWARE ");

  /* Check the Firmware Header (metadata) */
  e_ret_status = SFU_IMG_CheckCandidateMetadata();

  if (SFU_SUCCESS == e_ret_status)
  {
    /* Check the Firmware to be installed */
    e_ret_status = SFU_IMG_CheckCandidateImage();
  }
  /* else no installation: handle this as a critical failure */
  if (SFU_SUCCESS == e_ret_status)
  {
    /* Launch the Firmware Installation procedure */
    e_ret_status = SFU_IMG_TriggerImageInstallation();
  }
  /* else no installation: handle this as a critical failure */

  if (SFU_SUCCESS == e_ret_status)
  {
    /*
     * The FW installation succeeded: the previous FW is now backed up in the slot #1.
     * Nothing more to do, in the next FSM state we are going to verify the FW again and execute it if possible.
     */
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] FW installation succeeded.");
#endif /* SFU_VERBOSE_DEBUG_MODE */
  }
  else
  {
    /*
     * The FW installation failed: we need to reboot and the recovery procedure will be triggered at next boot.
     * Nothing more to do, the next FSM state (HANDLE_CRITICAL_FAILURE) will deal with it.
     */
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] FW installation failed!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
  }

  /* Set the next State Machine state according to the success or the failure of e_ret_status */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_VERIFY_USER_FW_SIGNATURE, SFU_STATE_HANDLE_CRITICAL_FAILURE);
}

/**
  * @brief  Execute a UserApp Fw Rollback
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_RollbackPrevUserFw(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  TRACE("\r\n= [SBOOT] STATE: ROLLBACK FROM PREV USER FIRMWARE");

  /*
   * With the 2 images implementation, 2 "rollbacks" are possible:
   *       A. A recovery to the previous FW when a new FW installation is stopped before its completion (e.g.: switch off during a swap)
   *       B. A rollback to the previous FW when the active FW is invalidated
   */

  /* [A] Recovery after an installation failure */
  /* ------------------------------------------ */
  /*
   * This recovery procedure re-installs the previous FW in the active slot
   * BUT does not preserve the downloaded FW that was under installation.
   * The user application will have to download it again to try again the installation procedure.
   */
  e_ret_status = SFU_IMG_TriggerRecoveryProcedure();

  /*
   * Set the next State Machine state according to the success or the failure of e_ret_status.
   * No specific error cause managed here because the FSM state already provides the information.
   */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_VERIFY_USER_FW_SIGNATURE, SFU_STATE_HANDLE_CRITICAL_FAILURE);

  /* [B] Rollback when the active FW is deemed as invalid        */
  /* ----------------------------------------------------------- */
  /* This case is considered as a critical failure and the rollback is handled in SFU_STATE_HANDLE_CRITICAL_FAILURE  */

}

/**
  * @brief  Verify the UserApp Fw signature before executing it
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_VerifyUserFwSignature(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  TRACE("\r\n= [SBOOT] STATE: VERIFY USER FW SIGNATURE");

  /*
   * With the 2 images handling:
   *       1. the active FW is always in slot 0
   *       2. the signature is verified when determining if a FW is installed (see SFU_BOOT_SM_CheckUserFwStatus)
   *       3. the signature is checked when installing a new firmware
   * So this check is redundant and should never fail.
   */

  /* read the FW header from FLASH and fill the appropriate structure (if the header is authenticated successfully) */
  e_ret_status = SFU_IMG_VerifyActiveImgMetadata();

  if (SFU_SUCCESS == e_ret_status)
  {
    /* Check the signature */
    e_ret_status = SFU_IMG_VerifyActiveImg();
  }
  else
  {
    /*
     * This should have already been caught by:
     *     1. the check already done in SFU_BOOT_SM_CheckUserFwStatus() at boot-up
     *     2. the check already done after installing a new FW
     * No action is taken in this example but a possibility could be to consider this as a rollback cause.
     */
    TRACE("\r\n=         Cannot retrieve any FW header from the active FW slot!");
  }

  /* Set the next State Machine state according to the success of the failure of e_ret_status */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_EXECUTE_USER_FW, SFU_STATE_HANDLE_CRITICAL_FAILURE);

}


/**
  * @brief  Exit from the SB/SFU State Machine and try to execute the UserApp Fw
  * @param  None
  * @note   This function must set the next State Machine State
  * @retval None
  */
static void SFU_BOOT_SM_ExecuteUserFw(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status = SE_KO;

  TRACE("\r\n= [SBOOT] STATE: EXECUTE USER FIRMWARE");

  /* Try to set the last execution status and continue: we assume the UserApp will start or a failure will be linked to the UserApp code so we can change the EXEC_ID now  */
  SFU_BOOT_SetLastExecStatus(EXEC_ID_USER_APP, m_StateMachineContext.CurrState);

  /*
   * You may decide to implement additional checks before running the Firmware.
   * For the time being we launch the FW present in the active slot.
   *
   * The bootloader must also take care of the security aspects:
   *   A.lock the SE services the UserApp is not allowed to call
   *   B.leave secure boot mode
   */
  /* Lock part of Secure Engine services */
  if (SE_LockRestrictServices(&e_se_status) == SE_SUCCESS)
  {
    /* Prepare for User Fw Execution */
    (void)SFU_LL_SECU_ExitSecureBootExecution(); /* with the current code checking the return value is useless */

    /* De-initialize the SB_SFU bootloader before launching the UserApp */
    (void)SFU_BOOT_DeInit(); /* the return value is not checked, we will always try launching the UserApp */

    /* This function should not return */
    e_ret_status = SFU_IMG_LaunchActiveImg();

    /* This point should not be reached */
#if defined(SFU_VERBOSE_DEBUG_MODE)
    /* We do not memorize any specific error, the FSM state is already providing the info */
    TRACE("\r\n=         SFU_IMG_LaunchActiveImg() failure!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    while (1);
  }
  else
  {
    TRACE("\r\n= [FWIMG] SECURE ENGINE CRITICAL FAILURE!");

    /* Set the error before forcing a reboot, don't care of return value as followed by reboot */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_LOCK_SE_SERVICES_ERR);

    /* This is the last operation executed. Force a System Reset */
    SFU_BOOT_ForceReboot();
  }

  /* This is unreachable code (dead code) in principle...
  At this point we should not be able to reach the following instructions.
  If we can execute them a critical issue has occurred.. So set the next State Machine accordingly */
  SFU_SET_SM_IF_CURR_STATE(e_ret_status, SFU_STATE_HANDLE_CRITICAL_FAILURE, SFU_STATE_HANDLE_CRITICAL_FAILURE);

}

/**
  * @brief  Manage a Critical Failure occurred during the evolution of the State Machine
  * @param  None
  * @note   After a Critical Failure a Reboot will be called.
  * @retval None
  */
static void SFU_BOOT_SM_HandleCriticalFailure(void)
{
  TRACE("\r\n= [SBOOT] STATE: HANDLE CRITICAL FAILURE");

  /* Any Critical Failure will be served and a SystemReset will be triggered */
  SFU_BOOT_StateExceptionHandler(m_StateMachineContext.PrevState);

  /* It's not possible to continue without compromising the stability or the security of the solution.
     The State Machine needs to be aborted and a Reset must be triggered */
  SFU_SET_SM_IF_CURR_STATE(0U, SFU_STATE_REBOOT_STATE_MACHINE, SFU_STATE_REBOOT_STATE_MACHINE);
}

/**
  * @brief  The state machine is aborted and a Reset is triggered
  * @param  None
  * @note   You are in this condition because it's not possible to continue without
            compromising the stability or the security of the solution.
  * @retval None
  */
static void SFU_BOOT_SM_RebootStateMachine(void)
{
  TRACE("\r\n= [SBOOT] STATE: REBOOT STATE MACHINE");

  /*
   * In case some clean-up must be done before resetting.
   * Please note that at the moment this function does not clean-up the RAM used by SB_SFU.
   */
  (void)SFU_BOOT_DeInit();

  /* This is the last operation executed. Force a System Reset */
  SFU_BOOT_ForceReboot();
}

/**
  * @}
  */





/** @addtogroup SFU_BOOT_Security_Functions
  * @{
  */

/**
  * @brief  Check (and Apply when possible) the security/safety/integrity protections.
  *         The "Apply" part depends on @ref SECBOOT_OB_DEV_MODE and @ref SFU_PROTECT_RDP_LEVEL.
  * @param  None
  * @note   This operation should be done as soon as possible after a reboot.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_BOOT_CheckApplySecurityProtections(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;

  /* Apply Static protections involving Option Bytes */
  if (SFU_LL_SECU_CheckApplyStaticProtections() == SFU_SUCCESS)
  {
    /* Apply runtime protections needed to be enabled after each Reset */
    e_ret_status = SFU_LL_SECU_CheckApplyRuntimeProtections();
  }
  else
  {
    e_ret_status = SFU_ERROR;
  }

  return e_ret_status;
}


/**
  * @brief  System security configuration
  * @param  None
  * @note   Check and apply the security protections. This has to be done as soon
  *         as possible after a reset
  * @retval None
  */
static int SFU_BOOT_SystemSecurity_Config(void)
{
  int ret = 0;
  /* WARNING: The following CheckApplySecurityProtection function must be called
     as soon as possible after a Reset in order to be sure the system is secured
     before starting any other operation. The drawback is that the ErrorManagement
     is not initialized yet, so in case of failure of this function, there will not be
     any error stored into the BootInfo or other visible effects.  */

  /* Very few things are already initialized at this stage. Need additional initialization
     to show a message that is added as below only in Debug/Test mode */
#ifdef SFU_DEBUG_MODE
  SFU_COM_Init();
#endif

  if (SFU_BOOT_CheckApplySecurityProtections() != SFU_SUCCESS)
  {
    /* WARNING: This might be generated by an attempted attack or a bug or your code!
       Add your code here in order to implement a custom action for this event,
       e.g. trigger a mass erase or take any other action in order to
       protect your system, or simply discard it if this is expected.
       ...
       ...
    */
    TRACE("\r\n= [SBOOT] System Security Check failed! Rebooting...");
  }
  else
  {
    TRACE("\r\n= [SBOOT] System Security Check successfully passed. Starting...");
    ret = 1;
  }

  return ret;
}

/**
  * @brief  Periodic verification of applied protection mechanisms, in order to prevent
  *         a malicious code removing some of the applied security/integrity features.
  *         The IWDG is also refreshed in this function.
  * @param  None
  * @note   This function must be called with a frequency greater than 0.25Hz!
  *         Otherwise a Reset will occur. Once enabled the IWDG cannot be disabled
  *         So the User App should continue to refresh the IWDG counter.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_BOOT_SecuritySafetyCheck(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /* Refresh the IWDG */
  e_ret_status = SFU_LL_SECU_IWDG_Refresh();

#ifdef SFU_FWALL_PROTECT_ENABLE
  /* Make sure the code isolation is properly set */
  if (SFU_SUCCESS == e_ret_status)
  {
    if (__HAL_FIREWALL_IS_ENABLED() != RESET)
    {
      /* Firewall enabled as expected */
      e_ret_status = SFU_SUCCESS;
    }
    else
    {
      /* Firewall not enabled: abnormal situation */
      e_ret_status = SFU_ERROR;
    }
  }
#endif /* SFU_FWALL_PROTECT_ENABLE */

  /* Add your code here for customization:
     e.g. for additional security or safety periodic check.
     ...
     ...
  */
  return e_ret_status;
}

/**
  * @}
  */

/** @addtogroup SFU_BOOT_ExecStatus_Functions
  * @{
  */

/**
  * @brief  Set the last execution status. For future customization the parameter
  *         list can be extended with an additional uint8_t, that at the moment is
  *         stored with a reserved default value.
  * @param  uExecID: This parameter can be a value of @ref SFU_BOOT_Private_Defines_Execution_Id
  * @param  uLastExecState: the last SM state to store into the BootInfo structure.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_BOOT_SetLastExecStatus(uint8_t uExecID, uint8_t uLastExecState)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef        e_se_status;
  SE_BootInfoTypeDef      x_boot_info;
  uint8_t uFwImageIdx = 0U; /* the active image is always in slot #0 */

  if ((IS_VALID_EXEC_ID(uExecID)) && (IS_SFU_SM_STATE(uLastExecState)))
  {
    if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
    {
      /* Set the last execution status as a 32-bit data */
      x_boot_info.LastExecStatus = RESERVED_VALUE | (uLastExecState << 8U) | (uFwImageIdx << 16U) | (uExecID << 24U);

      /* Update the BootInfo shared area according to the modifications above */
      if (SE_INFO_WriteBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
      {
        e_ret_status = SFU_SUCCESS;
      }

    }
  }
  return e_ret_status;
}

/**
  * @brief  Manage the  the last execution status, and take the right actions accordingly
  * @param  uLastExecStatus: 32-bit representing the @ref SFU_BOOT_Private_Macros_Execution_Status
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus  SFU_BOOT_ManageLastExecStatus(uint32_t uLastExecStatus)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef         e_se_status;
  SE_BootInfoTypeDef       x_boot_info;
  uint32_t                 u_last_error;

  if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
  {
#if defined(SFU_VERBOSE_DEBUG_MODE)
    /* Show the last execution state */
    TRACE("\r\n\t  INFO: Last execution status before Reboot was:");
    if (IS_SFU_SM_STATE(SFU_GET_LAST_EXEC_STATE(uLastExecStatus)))
    {
      TRACE(m_aStateMachineStrings[SFU_GET_LAST_EXEC_STATE(uLastExecStatus)]);
    }
    else
    {
      TRACE("Unknown");
    }
#endif /* SFU_VERBOSE_DEBUG_MODE */

    /* Show the last execution error */
    u_last_error = x_boot_info.LastExecError;
    TRACE("\r\n\t  INFO: Last execution detected error was:");
    if (IS_SFU_EXCPT((SFU_EXCPT_IdTypeDef)u_last_error))
    {
      TRACE(m_aErrorStrings[u_last_error]);
    }
    else
    {
      TRACE("Unknown error.");
      u_last_error = SFU_EXCPT_UNKNOWN;
    }

    /* Custom handler of the last error detected at reset */
    if ((SFU_EXCPT_IdTypeDef)u_last_error != SFU_EXCPT_NONE)
    {
      SFU_EXCPT_ResetExceptionHandler((SFU_EXCPT_IdTypeDef)u_last_error);
    }

    /* It's possible to reset the error at this point (because we are in SFU_STATE_CHECK_STATUS_ON_RESET context so no need to memorize the exception further) */
    x_boot_info.LastExecError = SFU_EXCPT_NONE;

    /* Update the BootInfo shared area according to the modifications above */
    if (SE_INFO_WriteBootInfo(&e_se_status, &x_boot_info) == SE_SUCCESS)
    {
      e_ret_status = SFU_SUCCESS;
    }
  }

  return e_ret_status;
}



/**
  * @brief  Manage the  the Reset sources, and if the case store the error for the next steps
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus  SFU_BOOT_ManageResetSources(void)
{
  SFU_ErrorStatus     e_ret_status = SFU_ERROR;
  SFU_RESET_IdTypeDef e_wakeup_source_id = SFU_RESET_UNKNOWN;
  SFU_EXCPT_IdTypeDef e_exception = SFU_EXCPT_NONE;
  SE_BootInfoTypeDef  x_boot_info; /* to retrieve and  update the counter of consecutive errors */
  SE_StatusTypeDef    e_se_status;

  /* Check the wakeup sources */
  SFU_LL_SECU_GetResetSources(&e_wakeup_source_id);
  switch (e_wakeup_source_id)
  {
      /*
       * Please note that the example of reset causes handling below is only a basic example to illustrate the way the RCC_CSR flags can be used to do so.
       * It is based on the behaviors we consider as normal and abnormal for the SB_SFU and UserApp example projects running on a Nucleo HW board.
       * Hence this piece of code must systematically be revisited and tuned for the targeted system (software and hardware expected behaviors must be assessed to tune this code).
       *
       * One may use the "uExecID" parameter to determine if the last exec status was in the SB_SFU context or UserApp context
       * to implement more clever checks in the reset cause handling below.
       */
    case SFU_RESET_FIREWALL:
      TRACE("\r\n\t  WARNING: A Reboot has been triggered by a Firewall reset!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* This event has to be considered as an error to manage */
      e_exception = SFU_EXCPT_FIREWALL_RESET;
      break;

    case SFU_RESET_WDG_RESET:
      TRACE("\r\n\t  WARNING: A Reboot has been triggered by a Watchdog reset!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* This event has to be considered as an error to manage */
      e_exception = SFU_EXCPT_WATCHDOG_RESET;
      break;

    case SFU_RESET_LOW_POWER:
      TRACE("\r\n\t  INFO: A Reboot has been triggered by a LowPower reset!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* In the current implementation this event is not considered as an error to manage.
         But this is strictly related to the final system. If needed to be managed as an error
         please add the right error code in the following src code line */
      e_exception = SFU_EXCPT_NONE;
      break;

    case SFU_RESET_HW_RESET:
      TRACE("\r\n\t  INFO: A Reboot has been triggered by a Hardware reset!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* In the current implementation this event is not considered as an error to manage.
         This is because a Nucleo board offers a RESET button triggering the HW reset.
         But this is strictly related to the final system. If needed to be managed as an error
         please add the right error code in the following src code line */
      e_exception = SFU_EXCPT_NONE;
      break;

    case SFU_RESET_BOR_RESET:
      TRACE("\r\n\t  INFO: A Reboot has been triggered by a BOR reset!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* In the current implementation this event is not considered as an error to manage.
         But this is strictly related to the final system. If needed to be managed as an error
         please add the right error code in the following src code line */
      e_exception = SFU_EXCPT_NONE;
      break;

    case SFU_RESET_SW_RESET:
      TRACE("\r\n\t  INFO: A Reboot has been triggered by a Software reset!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* In the current implementation this event is not considered as an error to manage,
         also because a sw reset is generated when the State Machine forces a Reboot.
         But this is strictly related to the final system. If needed to be managed as an error
         please add the right error code in the following src code line */
      e_exception = SFU_EXCPT_NONE;
      break;

    case SFU_RESET_OB_LOADER:
      TRACE("\r\n\t  WARNING: A Reboot has been triggered by an Option Bytes reload!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* In the current implementation this event is not considered as an error to manage,
         also because an OptionByte loader is called after applying some of the security protections (see SFU_CheckApplyStaticProtections).
         But this is strictly related to the final system. If needed to be managed as an error
         please add the right error code in the following src code line.
         Typically we may implement a more clever check where we determine if this OB reset occured only once because SB_SFU had to tune the OB initially,
         or if it occured again after these initial settings. If so, we could consider it as an attack. */
      e_exception = SFU_EXCPT_NONE;
      break;

    default:
      TRACE("\r\n\t  WARNING: A Reboot has been triggered by an Unknown reset source!");
      /* WARNING: This might be generated by an attempted attack, a bug or your code!
         Add your code here in order to implement a custom action for this event,
         e.g. trigger a mass erase or take any other  action in order to
         protect your system, or simply discard it if this is expected.
         ...
         ...
      */
      /* In the current implementation this event is not considered as an error to manage.
         But this is strictly related to the final system. If needed to be managed as an error
         please add the right error code in the following src code line */
      e_exception = SFU_EXCPT_NONE;
      break;
  }

  if (SE_INFO_ReadBootInfo(&e_se_status, &x_boot_info) != SE_SUCCESS)
  {
    /* This is not supposed to occur because ReadBootInfo is already called before SFU_BOOT_ManageResetSources() is entered... */
    TRACE("\r\n\t  BOOT INFO reading Error");
    /* Do nothing more and return an error */
  }
  else
  {
    /* In case an error has been set above, set/overwrite it into the BootInfo */
    if (e_exception != SFU_EXCPT_NONE)
    {
      /* increment number of consecutive boot errors */
      x_boot_info.ConsecutiveBootOnErrorCounter++;
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  Consecutive Boot on error counter ++ ");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    }
    else
    {
      /*
       * The reset cause is considered as normal so we reset the counter of consecutive errors
       */
      x_boot_info.ConsecutiveBootOnErrorCounter = 0;
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  Consecutive Boot on error counter reset ");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    }

    /* Log the consecutive init counter value in the console */
    TRACE("\r\n\t  Consecutive Boot on error counter = %d ", x_boot_info.ConsecutiveBootOnErrorCounter);

    /* Update BootInfo structure with  consecutive boot counter */
    if (SE_INFO_WriteBootInfo(&e_se_status, &x_boot_info) != SE_SUCCESS)
    {
      TRACE("\r\n\t  BOOT INFO Consecutive Boot on error counter writing error ");
    }
    else
    {
      e_ret_status = SFU_SUCCESS;
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  Consecutive Boot on error counter updated ");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    }
  }

  /* Update the last exec error if needed */
  if (e_exception != SFU_EXCPT_NONE)
  {
    if (SFU_BOOT_SetLastExecError((uint32_t)e_exception) != SFU_SUCCESS)
    {
      e_ret_status = SFU_ERROR;
    } /* else keep the previous e_ret_status value */
  } /* else: no need to update it */

  /* Once the reset sources has been managed and a possible error has been set, clear the reset sources */
  SFU_LL_SECU_ClearResetSources();

  return e_ret_status;
}

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup SFU_BOOT_Callback_Functions Callback_Functions
  * @brief Cube HAL callbacks implementation.
  * @{
  */

/**
  * @brief  Implement the Cube_Hal Callback generated on the Tamper IRQ.
  * @param  None
  * @retval None
  */
void SFU_CALLBACK_Antitamper(void)
{
  SFU_BOOT_IrqExceptionHandler(SFU_EXCPT_TAMPERING_FAULT);
}

/**
  * @brief  Implement the Cube_Hal Callback generated on the Memory Fault.
  * @note  After a Memory Fault could not be possible to execute additional code
  * @param  None
  * @retval None
  */
void SFU_CALLBACK_MemoryFault(void)
{
  SFU_BOOT_IrqExceptionHandler(SFU_EXCPT_MEMORY_FAULT);
}

/**
  * @brief  Implement the Cube_Hal Callback generated on Hard Fault.
  * @note   After a Hard Fault could not be possible to execute additional code
  * @param  None
  * @retval None
  */
void SFU_CALLBACK_HardFault(void)
{
  SFU_BOOT_IrqExceptionHandler(SFU_EXCPT_HARD_FAULT);
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

#undef SFU_BOOT_C


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
