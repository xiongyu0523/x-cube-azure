/**
  ******************************************************************************
  * @file    sfu_fwimg_services.c
  * @author  MCD Application Team
  * @brief   This file provides set of firmware functions to manage the Firmware Images.
  *          This file contains the services the bootloader (sfu_boot) can use to deal
  *          with the FW images handling.
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

#define SFU_FWIMG_SERVICES_C

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "sfu_fsm_states.h" /* needed for sfu_error.h */
#include "sfu_error.h"
#include "sfu_low_level_flash.h"
#include "sfu_low_level_security.h"
#include "sfu_low_level.h"
#include "sfu_fwimg_services.h"
#include "sfu_fwimg_internal.h"
#include "sfu_trace.h"



/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_IMG SFU Firmware Image
  * @{
  */

/** @defgroup SFU_IMG_SERVICES SFU Firmware Image Services
  * @brief FW handling services the bootloader can call.
  * @{
  */

/** @defgroup SFU_IMG_Exported_Functions FW Images Handling Services
  * @brief Services the bootloader can call to handle the FW images.
  * @{
  */

/** @defgroup SFU_IMG_Initialization_Functions FW Images Handling Init. Functions
  * @brief FWIMG initialization functions.
  * @{
  */

/**
  * @brief FW Image Handling (FWIMG) initialization function.
  *        Checks the validity of the settings related to image handling (slots size and alignments...).
  * @note  The system initialization must have been performed before calling this function (flash driver ready to be used...etc...).
  *        Must be called first (and once) before calling the other Image handling services.
  * @param  None.
  * @retval SFU_IMG_InitStatusTypeDef SFU_IMG_INIT_OK if successful, an error code otherwise.
  */
SFU_IMG_InitStatusTypeDef SFU_IMG_InitImageHandling(void)
{
  /*
   * At the moment there is nothing more to do than initializing the fwimg_core part.
   */
  return (SFU_IMG_CoreInit());
}

/**
  * @brief FW Image Handling (FWIMG) de-init function.
  *        Terminates the images handling service.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_ShutdownImageHandling(void)
{
  /*
   * At the moment there is nothing more to do than shutting down the fwimg_core part.
   */
  return (SFU_IMG_CoreDeInit());
}

/**
  * @}
  */

/** @defgroup SFU_IMG_Service_Functions FW Images Handling Services Functions
  * @brief FWIMG services functions.
  * @{
  */

/** @defgroup SFU_IMG_Service_Functions_NewImg New image installation services
  * @brief New image installation functions.
  * @{
  */

/**
  * @brief Checks if there is a pending firmware installation.
  *        3 situations can occur:
  *        A. Pending firmware installation: a firmware is ready to be installed but the installation has never been triggered so far.
  *        B. Pending firmware recovery: a firmware installation has been interrupted and a recovery procedure is required to come back to a steady state (valid user application).
  *        C. No firmware installation pending
  * @note  The anti-rollback check is not taken into account at this stage.
  *        But, if the firmware to be installed already carries some metadata (VALID tag) a newly downloaded firmware should not have then the installation is not considered as pending.
  * @note This function populates the FWIMG module variables: fw_desc_to_recover, fw_image_header_to_test, fw_header_to_test
  * @param  None.
  * @retval SFU_IMG_ImgInstallStateTypeDef Pending Installation status (peding install, pending recovery, no pending action)
  */
SFU_IMG_ImgInstallStateTypeDef SFU_IMG_CheckPendingInstallation(void)
{
  SFU_IMG_ImgInstallStateTypeDef e_ret_state = SFU_IMG_NO_FWUPDATE;
  /*
   * The order of the checks is important:
   *
   * A. Check if a FW update has been interrupted first.
   *    This check is based on the content of the trailer area and the validity of the backed up image.
   *
   * B. Check if a Firmware image is waiting to be installed.
   */
  if (SFU_SUCCESS == SFU_IMG_CheckTrailerValid(&fw_desc_to_recover))
  {
    /* A Firmware Update has been stopped: the FW to recover is now described in 'fw_desc_to_recover' */
    e_ret_state = SFU_IMG_FWUPDATE_STOPPED;
  }
  else if (SFU_SUCCESS == SFU_IMG_FirmwareToInstall())
  {
    /*
     * A Firmware is available for installation:
     * fw_image_header_to_test and fw_header_to_test have been populated
     */
    e_ret_state = SFU_IMG_FWIMAGE_TO_INSTALL;
  }
  else
  {
    /* No interrupted FW update and no pending update */
    e_ret_state = SFU_IMG_NO_FWUPDATE;
  }

  return e_ret_state;
}

/**
  * @brief Verifies the validity of the metadata associated to a candidate image.
  *        The anti-rollback check is performed.
  *        Errors can be memorized in the bootinfo area but no action is taken in this procedure.
  * @note It is up to the caller to make sure the conditions to call this primitive are met (no check performed before running the procedure):
  *       SFU_IMG_CheckPendingInstallation should be called first.
  * @note If this check fails the candidate Firmware is altered in FLASH to invalidate it.
  * @note For the local loader (@ref SECBOOT_USE_LOCAL_LOADER), the candidate metadata is verified by: @ref SFU_APP_VerifyFwHeader.
  *       But, SFU_IMG_CheckCandidateMetadata will be called when the installation of the new image is triggered.
  * @param None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, error code otherwise
  */
SFU_ErrorStatus SFU_IMG_CheckCandidateMetadata(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SFU_FLASH_StatusTypeDef flash_if_status;

  /*
   * ##0 - FW Header Authentication
   *       An authentication check should be performed.
   *
   *       In this example, we bypass this check because SFU_IMG_CheckPendingInstallation() has been called first.
   *       Therefore we know the header is fine.
   *
   *       As a consequence: fw_image_header_to_test must have been populated first (by SFU_IMG_CheckPendingInstallation)
   */
  SFU_ErrorStatus authCheck = SFU_SUCCESS;

  if (SFU_SUCCESS == authCheck)
  {
    /*
     * ##1 - Check version :
     *      It is not allowed to install a Firmware with a lower version than the active firmware.
     *      But we authorize the re-installation of the current firmware version.
     *      We also check (in case there is no active FW) that the candidate version is at least the min. allowed version for this device.
     *
     *      SFU_IMG_GetActiveFwVersion() returns -1 if there is no active firmware but can return a value if a bricked FW is present with a valid header.
     */
    int32_t curVer = SFU_IMG_GetActiveFwVersion();
    SFU_ErrorStatus hasActiveFw = SFU_IMG_HasValidActiveFirmware();

    if (!(((((int32_t)fw_image_header_to_test.FwVersion) >= curVer) || (SFU_ERROR == hasActiveFw)) &&  /* candidate >= current version if any */
          (fw_image_header_to_test.FwVersion >= SFU_FW_VERSION_START_NUM)))                            /* && candidate >= min.version number  */
    {
      /* The installation is forbidden */
#if defined(SFU_VERBOSE_DEBUG_MODE)
      if (SFU_SUCCESS == hasActiveFw)
      {
        /* a FW with a lower version cannot be installed */
        TRACE("\r\n          Anti-rollback: candidate version(%d) rejected | current version(%d) , min.version(%d) !", fw_image_header_to_test.FwVersion, curVer, SFU_FW_VERSION_START_NUM);
      }
      else
      {
        /* There is no FW at all or a bricked FW in slot #0 so its version is Not Applicable (N.A.) */
        TRACE("\r\n          Anti-rollback: candidate version(%d) rejected | current version(N.A.) , min.version(%d) !", fw_image_header_to_test.FwVersion, SFU_FW_VERSION_START_NUM);
      }
#endif /* SFU_VERBOSE_DEBUG_MODE */

      /* Memorize this error as this will be handled as a critical failure */
      (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_INCORRECT_VERSION);

      /* We would enter an infinite loop of installation attempts if we do not clean-up the swap sector */
      e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)SFU_IMG_SWAP_REGION_BEGIN, SFU_IMG_IMAGE_OFFSET) ;
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);

      /* leave now to handle the critical failure in the appropriate FSM state */
      e_ret_status = SFU_ERROR;
    }
    else
    {
      /* the anti-rollback check succeeds: the version is fine */
      e_ret_status = SFU_SUCCESS;
    }
  }
  else
  {
    /* authentication failure: in this example, this code is unreachable */
    e_ret_status = SFU_ERROR; /* unreachable */
  }

  /* Return the result */
  return (e_ret_status);
}

/**
  * @brief Verifies the validity of a candidate image.
  *        This function does not decrypt the candidate firmware in flash (so does not modify the flash content).
  *        Errors can be memorized in the bootinfo area but no action is taken in this procedure.
  *        Cryptographic operations are used to make sure the firmware has not already been decrypted.
  * @note It is up to the caller to make sure the conditions to call this primitive are met (no check performed before running the procedure):
  *       SFU_IMG_CheckPendingInstallation should be called first.
  * @note If this check fails for a major cause then the SWAP sector in FLASH is altered to prevent the Candidate firmware from being installed.
  * @param None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, error code otherwise
  */
SFU_ErrorStatus SFU_IMG_CheckCandidateImage(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
#if (SFU_IMAGE_PROGRAMMING_TYPE == SFU_ENCRYPTED_IMAGE)
  SE_StatusTypeDef e_se_status;
  SFU_FLASH_StatusTypeDef flash_if_status;
#endif /* SFU_IMAGE_PROGRAMMING_TYPE */

  /* ##0 - All pre-requisites have been checked by SFU_IMG_CheckPendingInstallation:
   *       A. Making sure this Candidate Image matches the installation request from the User Application (thanks to header comparison)
   *       B. Making sure this candidate image is "fresh" (not already tagged as valid by the bootloader)
   *       C. Making sure the FW image is not too big
   * Nothing more to do.
   */

#if (SFU_IMAGE_PROGRAMMING_TYPE == SFU_ENCRYPTED_IMAGE)

  /*  ##1 - check no ECC double error on this image, and image not already decrypted */
  /*  signature encrypted is not the same as decrypted */
  e_ret_status = SFU_IMG_VerifyFwSignature(&e_se_status, &fw_image_header_to_test, 1);
  if ((e_ret_status == SFU_SUCCESS) || (e_se_status == SE_ERR_FLASH_READ))
  {
    /* e_ret_status == SFU_SUCCESS: the signature check succeeded so this means that slot#1 contains a decrypted FW, this is abnormal.
    * No need to go further as the first step of the installation is a decrypt: this would fail.
    * e_se_status == SE_ERR_FLASH_READ: double ECC error
    */
    if (SFU_SUCCESS == e_ret_status)
    {
      (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_INCORRECT_BINARY);
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n= [FWIMG] The image to be installed is not encrypted!");
#endif /* SFU_VERBOSE_DEBUG_MODE */

      /* We would enter an infinite loop of installation attempts if we do not clean-up the swap sector */
      e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)SFU_IMG_SWAP_REGION_BEGIN, SFU_IMG_IMAGE_OFFSET) ;
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);
    }
    else
    {
      /* e_se_status == SE_ERR_FLASH_READ */
      (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FLASH_ERROR);
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n= [FWIMG] Flash error!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    }

    /* in both cases this is an error, the candidate image is rejected  */
    e_ret_status = SFU_ERROR;
  }
  else
  {
    /* The candidate image is fine */
    e_ret_status = SFU_SUCCESS;
  }
#elif (SFU_IMAGE_PROGRAMMING_TYPE == SFU_CLEAR_IMAGE)
  /* No check is performed at this stage - see ##2 below */
  e_ret_status = SFU_SUCCESS;
#else
#error "The current example does not support the selected Firmware Image Programming type."
#endif /* SFU_IMAGE_PROGRAMMING_TYPE */

  /*
   * ##2 - We should authenticate the candidate image
   * In this example, to authenticate the candidate image we need to decrypt it.
   * Therefore this check is bypassed and will be performed when installing the new image (see SFU_IMG_TriggerImageInstallation).
   *
   * Note: we do the same even if the image is in clear format to avoid introducing a difference in the sequence.
   */

  /* Return the result */
  return (e_ret_status);
}

/**
  * @brief Installs a new firmware, performs the post-installation checks and sets the metadata to tag this firmware as valid if the checks are successful.
  *        The anti-rollback check is performed and errors are memorized in the bootinfo area, but no action is taken in this procedure.
  *        Cryptographic operations are used (if enabled): the firmware is decrypted and its authenticity is checked afterwards if the cryptographic scheme allows this (signature check for instance).
  *        The detailed errors are memorized in bootinfo area to be processed as critical errors if needed.
  *        This function modifies the FLASH content.
  *        If this procedure is interrupted before its completion (e.g.: switch off) it cannot be resumed .
  * @note  It is up to the caller to make sure the conditions to call this primitive are met (no check performed before running the procedure):
  *        SFU_IMG_CheckPendingInstallation, SFU_IMG_CheckCandidateMetadata, SFU_IMG_CheckCandidateImage should be called first.
  *        fw_image_header_to_test to be populated before calling this function.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_TriggerImageInstallation(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /*
   * In this example, the preparation stage consists in decrypting the candidate firmware image.
   *
   */
  e_ret_status = SFU_IMG_PrepareCandidateImageForInstall();

  if (SFU_SUCCESS == e_ret_status)
  {
    /*
     * In this example, installing the new firmware image consists in swapping the 2 FW images.
     */
    e_ret_status = SFU_IMG_InstallNewVersion();
  } /* else no need to try the installation as the preparation stage failed */

  /* return the installation result */
  return (e_ret_status);
}

/**
  * @}
  */

/** @defgroup SFU_IMG_Service_Functions_Reinstall Image re-installation services
  * @brief Image re-installation functions.
  * @{
  */

/**
  * @brief Re-installs a previous valid firmware after a firmware installation failure.
  *        If this procedure is interrupted before its completion (e.g.: switch off) it can be resumed.
  *        This function is dedicated (only) to the recovery of the previous firmware when a new firmware installation is stopped before its completion.
  *        This function does not handle explicitly the metadata tagging the firmware as valid: these metadata must have been preserved during the installation procedure.
  *        This function modifies the FLASH content.
  * @note  It is up to the caller to make sure the conditions to call this primitive are met (no check performed before running the procedure):
  *        need to make sure an installation has been interrupted.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_TriggerRecoveryProcedure(void)
{
  /*
   * Nothing more to do than calling the core procedure
   * The core procedure is kept because we do not want code dealing with the internals (like trailers) in this file.
   */
  return (SFU_IMG_Recover());
}

/**
  * @brief Checks if a rollback procedure to re-install a previous valid firmware can be triggered.
  *        This function is dedicated (only) to the re-installation of a previous valid firmware when an active firmware has been deemed as invalid.
  *        This function modifies the FLASH content (so it cannot be called several times in a row for instance).
  *        If this procedure is interrupted before its completion (e.g.: switch off) it cannot be resumed.
  * @note In this example, the function uses the firmware version available in the FW metadata so this piece of information must be available.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_CheckRollbackConditions(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint16_t minAcceptableVersion = SFU_FW_VERSION_START_NUM;
  uint16_t maxAcceptableVersion = SFU_FW_VERSION_START_NUM;
  int32_t curFwVersion = SFU_IMG_GetActiveFwVersion();

  /*
   * Compute the range of acceptable versions for the rollback.
   * This code is here only to illustrate how to use the APIs, this must be fully re-written
   * to implement a suitable processing for the targeted system.
   *
   * We do not check any upper bound here as we assume a FW with an incorrect version number would not have been installed.
   * This needs to be revisited for the targeted system.
   */
  if (curFwVersion > SFU_FW_VERSION_START_NUM)
  {
    /* In this example we consider that we can accept a backed-up FW which is :
     * exactly 1 version behind the invalidated FW
     */
    minAcceptableVersion = (uint16_t)(curFwVersion - 1); /* min = version N-1 */
    maxAcceptableVersion = (uint16_t)(curFwVersion - 1); /* max = version N-1 too */

    /* We do not check the lower and upper bounds in this example as we select version N-1
     * and curFwVersion is greater than SFU_FW_VERSION_START_NUM,
     * but these checks should be implemented if the chosen strategy differs from this.
     */
#ifdef SFU_DEBUG_MODE
    TRACE("\r\n= [FWIMG] RollBack range is: [%d;%d]", minAcceptableVersion, maxAcceptableVersion);
#endif /* SFU_DEBUG_MODE */
  }
#ifdef SFU_DEBUG_MODE
  else
  {
    /* else no rollback to version N-1 is possible: we will invalidate the current FW at the next step */
    TRACE("\r\n= [FWIMG] The current FW version (%d) does not allow us to try a rollback!", curFwVersion);
  }
#endif /* SFU_DEBUG_MODE */

  /* The checks below must be adapted to match the chosen strategy */
  if ((curFwVersion > SFU_FW_VERSION_START_NUM) &&
      (SFU_SUCCESS == SFU_IMG_ValidFwInSlot1(minAcceptableVersion, maxAcceptableVersion)))
  {
    /* rollback is possible */
    e_ret_status = SFU_SUCCESS;
  }
  else
  {
    /* no rollback is possible */
    e_ret_status = SFU_ERROR;
  }

  /* Return the result */
  return (e_ret_status);
}

/**
  * @brief Triggers a rollback procedure to re-install a previous valid firmware.
  *        This function re-installs a backed up firmware and sets the metadata to tag this firmware as valid.
  *        This function is dedicated (only) to the re-installation of a previous valid firmware when an active firmware has been deemed as invalid.
  *        This function modifies the FLASH content (so it cannot be called several times in a row for instance).
  *        If this procedure is interrupted before its completion (e.g.: switch off) it cannot be resumed.
  * @note  It is up to the caller to make sure the conditions to call this primitive are met (no check performed before running the procedure).
  *        Tagging the re-installed firmware as valid must be the last operation of this procedure (to detect if the procedure completed or not).
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_TriggerRollbackProcedure(void)
{
  /*
   * Nothing more to do than calling the core procedure.
   * The core procedure is kept because we do not want code dealing with the internals (like updating valid tags) in this file.
   */
  return (SFU_IMG_RollBack());
}

/**
  * @}
  */

/** @defgroup SFU_IMG_Service_Functions_CurrentImage Active Firmware services (current image)
  * @brief Active Firmware Image functions.
  * @{
  */

/**
  * @brief This function makes sure the current active firmware will not be considered as valid any more.
  *        This function alters the FLASH content.
  * @note It is up to the caller to make sure the conditions to call this primitive are met (no check performed before running the procedure).
  * @param  None.
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_InvalidateCurrentFirmware(void)
{
  SFU_LL_SECU_IWDG_Refresh();
  SFU_FLASH_StatusTypeDef x_flash_info;

  /* Invalidates the FW in the active slot (slot #0) */
  return (SFU_LL_FLASH_Erase_Size(&x_flash_info, SFU_IMG_SLOT_0_REGION_BEGIN,
                                  (uint32_t)FW_HEADER_TOT_LEN)); /* altering the header bytes */
}

/**
  * @brief Verifies the validity of the active firmware image metadata.
  * @note This function relies on cryptographic procedures and it is up to the caller to make sure the required elements have been configured.
  * @note This function populates the FWIMG module variable: fw_image_header_validated
  * @param None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, error code otherwise
  */
SFU_ErrorStatus SFU_IMG_VerifyActiveImgMetadata(void)
{
  /*
   * The active FW slot is slot #0.
   * If the metadata is valid then 'fw_image_header_validated' is filled with the metadata.
   */
  return (SFU_IMG_GetFWInfoMAC(&fw_image_header_validated, 0));
}

/**
  * @brief Verifies the validity of the active firmware image.
  * @note This function relies on cryptographic procedures and it is up to the caller to make sure the required elements have been configured.
  *       Typically, SFU_IMG_VerifyActiveImgMetadata() must have been called first to populate fw_image_header_validated.
  * @param None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, error code otherwise
  */
SFU_ErrorStatus SFU_IMG_VerifyActiveImg(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status = SE_KO;

  /*
   * fw_image_header_validated MUST have been populated with valid metadata first,
   * slot #0 is the active FW image
   */
  e_ret_status = SFU_IMG_VerifyFwSignature(&e_se_status, &fw_image_header_validated, 0U);
#if defined(SFU_VERBOSE_DEBUG_MODE)
  if (SFU_ERROR == e_ret_status)
  {
    /* We do not memorize any specific error, the FSM state is already providing the info */
    TRACE("\r\n=         SFU_IMG_VerifyActiveImg failure with se_status=%d!", e_se_status);
  }
#endif /* SFU_VERBOSE_DEBUG_MODE */

  return (e_ret_status);
}

/**
  * @brief Launches the user application.
  *        The caller must be prepared to never get the hand back after calling this function.
  *        If a problem occurs, it must be memorized in the bootinfo area.
  *        If the caller gets the hand back then this situation must be handled as a critical error.
  * @note It is up to the caller to make sure the conditions to call this primitive are met
  *       (typically: no security check performed before launching the firmware).
  * @note This function only handles the "system" aspects.
  *       It is up to the caller to manage any security related action (enable ITs, disengage MPU, clean RAM...).
  *       Nevertheless, cleaning-up the stack and heap used by SB_SFU is part of the system actions handled by this function (as this needs to be done just before jumping into the user application).
  * @param  None.
  * @retval SFU_ErrorStatus Does not return if successful, returns SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_LaunchActiveImg(void)
{
  uint32_t jump_address ;
  typedef void (*Function_Pointer)(void);
  Function_Pointer  p_jump_to_function;

  jump_address = *(__IO uint32_t *)(((uint32_t)SFU_IMG_SLOT_0_REGION_BEGIN + SFU_IMG_IMAGE_OFFSET + 4));
  /* Jump to user application */
  p_jump_to_function = (Function_Pointer) jump_address;
  /* Initialize user application's Stack Pointer */
  __set_MSP(*(__IO uint32_t *)(SFU_IMG_SLOT_0_REGION_BEGIN + SFU_IMG_IMAGE_OFFSET));

  /* Destroy the Volatile data and CSTACK in SRAM used by Secure Boot in order to prevent any access to sensitive data from the UserApp.
     If FWall is used and a Secure Engine CStack context switch is implemented, these data are the data
     outside the protected area, so not so sensitive, but still in this way we add increase the level of security.
  */
  SFU_LL_SB_SRAM_Erase();
  /* JUMP into User App */
  p_jump_to_function();


  /* The point below should NOT be reached */
  return (SFU_ERROR);
}

/**
  * @brief Get the version of the active FW (present in slot #0)
  * @note It is up to the caller to make sure the slot #0 contains a valid active FW.
  * @note In the current implementation the header is checked (authentication) and no version is returned if this check fails.
  * @param  None.
  * @retval the FW version if it succeeds (coded on uint16_t), -1 otherwise
  */
int32_t SFU_IMG_GetActiveFwVersion(void)
{
  SE_FwRawHeaderTypeDef fw_image_header;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  int32_t version = -1;

  /*  check the header in slot #0 */
  e_ret_status = SFU_IMG_GetFWInfoMAC(&fw_image_header, 0U);
  if (e_ret_status == SFU_SUCCESS)
  {
    /* retrieve the version from the header without any further check */
    version = (int32_t)fw_image_header.FwVersion;
  }

  return (version);
}

/**
  * @brief Indicates if a valid active firmware image is installed.
  *        It performs the same checks as SFU_IMG_VerifyActiveImgMetadata and SFU_IMG_VerifyActiveImg
  *        but it also verifies if the Firmware image is internally tagged as valid.
  * @note This function modifies the FWIMG variables 'fw_image_header_validated' and 'fw_header_validated'
  * @param  None.
  * @retval SFU_SUCCESS if successful, error code otherwise
  */
SFU_ErrorStatus SFU_IMG_HasValidActiveFirmware(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;

  /* check the installed header (metadata) */
  e_ret_status = SFU_IMG_VerifyActiveImgMetadata();
  if (e_ret_status == SFU_SUCCESS)
  {
    /* check the installed FW image itself */
    e_ret_status = SFU_IMG_VerifyActiveImg();
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    /* check that this FW has been tagged as valid by the bootloader  */
    e_ret_status = SFU_IMG_CheckSlot0FwValid();
  }

  return e_ret_status;
}

/**
  * @brief  Validate the active FW image in slot #0 by installing the header and the VALID tags
  * @param  address of the header to be installed in slot #0
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_Validation(uint8_t *pHeader)
{
  return (SFU_IMG_WriteHeaderValidated(pHeader));
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

#undef SFU_FWIMG_SERVICES_C

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
