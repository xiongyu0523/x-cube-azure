/**
  ******************************************************************************
  * @file    sfu_fwimg_core.c
  * @author  MCD Application Team
  * @brief   This file provides set of firmware functions to manage the Firmware Images.
  *          This file contains the "core" functionalities of the image handling.
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

#define SFU_FWIMG_CORE_C

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "sfu_fsm_states.h" /* needed for sfu_error.h */
#include "sfu_error.h"
#include "sfu_low_level_flash.h"
#include "sfu_low_level_security.h"
#include "se_interface_bootloader.h"
#include "sfu_fwimg_regions.h"
#include "sfu_fwimg_services.h" /* to have definitions like SFU_IMG_InitStatusTypeDef (required by sfu_fwimg_internal.h) */
#include "sfu_fwimg_internal.h"
#include "sfu_trace.h"


/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup SFU_IMG SFU Firmware Image
  * @brief Firmware Images Handling  (FWIMG)
  * @{
  */

/** @defgroup SFU_IMG_CORE SFU Firmware Image Core
  * @brief Core functionalities of the FW image handling (trailers, magic, swap...).
  * @{
  */


/** @defgroup SFU_IMG_CORE_Private_Defines Private Defines
  * @brief FWIMG Defines used only by sfu_fwimg_core
  * @{
  */

/**
  * @brief RAM chunk used for decryption / comparison / swap
  * it is the size of RAM buffer allocated in stack and used for decrypting/moving images.
  * some function allocates 2 buffer of this size in stack.
  * As image are encrypted by 128 bits blocks, this value is 16 bytes aligned.
  * it can be equal to SWAP_SIZE , or SWAP_SIZE = N  * CHUNK_SIZE
  */
#define VALID_SIZE (3*MAGIC_LENGTH)
#define CHUNK_SIZE_SIGN_VERIFICATION (1024)  /*!< Signature verification chunk size*/

#define SFU_IMG_CHUNK_SIZE  (512U)

/**
  * @brief  FW slot and swap offset
  */
#define TRAILER_INDEX  ((uint32_t)(SFU_IMG_SLOT_0_REGION_SIZE) / (uint32_t)(SFU_IMG_SWAP_REGION_SIZE))
/*  position of image begin on 1st block */
#define TRAILER_HEADER  ( FW_INFO_TOT_LEN+ FW_INFO_TOT_LEN + MAGIC_LENGTH  )
#define COUNTER_HEADER  TRAILER_HEADER
/*                            SLOT1_REGION_SIZE                                                                                   */
/*  <-------------------------------------------------------------------------------------------------------------------------->  */
/*  |                    | FW_INFO_TOT_LEN bytes | FW_INFO_TOT_LEN bytes |MAGIC_LENGTH bytes  | N*sizeof(SFU_LL_FLASH_write_t) |  */
/*  |                    | header 1              | header 2              |SWAP magic          | N*CPY                          |  */
/*                                                                                                              */
/* Please note that the size of the trailer area (N*CPY) depends directly on SFU_LL_FLASH_write_t type,         */
/* so it can differ from one platform to another (this is FLASH-dependent)                                      */
#define TRAILER_SIZE (sizeof(SFU_LL_FLASH_write_t)*(TRAILER_INDEX)+(uint32_t)(TRAILER_HEADER))
#define TRAILER_BEGIN  (( uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + (uint32_t)SFU_IMG_SLOT_1_REGION_SIZE - TRAILER_SIZE))
#define TRAILER_CPY(i) ((void*)((uint32_t)TRAILER_BEGIN + (uint32_t)TRAILER_HEADER+((i)*sizeof(SFU_LL_FLASH_write_t))))
#define TRAILER_SWAP_ADDR  (( uint8_t *)((uint32_t)TRAILER_BEGIN + TRAILER_HEADER - MAGIC_LENGTH))
#define TRAILER_HDR_VALID (( uint8_t *)(TRAILER_BEGIN))
#define TRAILER_HDR_TEST  (( uint8_t *)(TRAILER_BEGIN + FW_INFO_TOT_LEN))
#define SLOT_HDR_VALID(A) ((!memcmp(&A,SWAPPED,sizeof(A)))  ? (( uint8_t *)(SFU_IMG_SLOT_1_REGION_BEGIN)):( uint8_t *)(SFU_IMG_SLOT_0_REGION_BEGIN))

/*  read of this info can cause double ECC NMI , fix me , the field must be considered , only one double ECC error is
 *  possible, if several error are observed , aging issue or bug */

#define CHECK_TRAILER_MAGIC CheckMagic(TRAILER_SWAP_ADDR, TRAILER_HDR_VALID,  TRAILER_HDR_TEST)

#define WRITE_TRAILER_MAGIC WriteMagic(TRAILER_SWAP_ADDR, TRAILER_HDR_VALID, TRAILER_HDR_TEST)
#define CLEAN_TRAILER_MAGIC CleanMagicValue(TRAILER_SWAP_ADDR)

#define CHUNK_1_ADDR(A,B) ((uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN +(SFU_IMG_SWAP_REGION_SIZE*A)+SFU_IMG_CHUNK_SIZE*B))
#define CHUNK_0_ADDR(A,B) ((uint8_t *)((uint32_t) SFU_IMG_SLOT_0_REGION_BEGIN +(SFU_IMG_SWAP_REGION_SIZE*A)+SFU_IMG_CHUNK_SIZE*B))

#define CHUNK_0_ADDR_MODIFIED(A,B) (((A==0) && (B==0))?\
      ((uint8_t*)((uint32_t) SFU_IMG_SLOT_0_REGION_BEGIN +SFU_IMG_IMAGE_OFFSET)) : ((uint8_t *)((uint32_t) SFU_IMG_SLOT_0_REGION_BEGIN +(SFU_IMG_SWAP_REGION_SIZE*A)+SFU_IMG_CHUNK_SIZE*B)))
#define CHUNK_SWAP_ADDR(B) ((uint8_t *)((uint32_t) SFU_IMG_SWAP_REGION_BEGIN+SFU_IMG_CHUNK_SIZE*B))

#define AES_BLOCK_SIZE (16U)  /*!< Size of an AES block to check padding needs for decrypting */

/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Private_Types Private Types
  * @brief FWIMG Types used only by sfu_fwimg_core
  * @{
  */

/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Private_Variable Private Variables
  * @brief FWIMG Module Variables used only by sfu_fwimg_core
  * @{
  */
static uint8_t  SFU_APP_HDR_VALID_FWInfoInputAuthenticated[FW_INFO_TOT_LEN] __attribute__((aligned(8)));
static uint8_t  SFU_APP_HDR_TEST_FWInfoInputAuthenticated[FW_INFO_TOT_LEN] __attribute__((aligned(8)));
/**
 * FW header (metadata) of the candidate FW in slot #1: raw format (access by bytes)
 */
static uint8_t fw_header_to_test[FW_INFO_TOT_LEN] __attribute__((aligned(8)));

/**
 * FW header (metadata) of the active FW in slot #0: raw format (access by bytes)
 */
static uint8_t fw_header_validated[FW_INFO_TOT_LEN] __attribute__((aligned(8)));

/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Private_Functions Private Functions
  *  @brief Private functions used by fwimg_core internally.
  *  @note All these functions should be declared as static and should NOT have the SFU_IMG prefix.
  * @{
  */

/* used the same Magic for counter and trailer*/

/**
  * @brief  Check raw header
  * @param  pFWinfoInput: pointer to raw header. This must match SE_FwRawHeaderTypeDef.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus VerifyFwRawHeaderTag(uint8_t *pFWInfoInput)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef se_status;

  if (SE_VerifyFwRawHeaderTag(&se_status, (SE_FwRawHeaderTypeDef *)pFWInfoInput) == SE_SUCCESS)
  {
    e_ret_status = SFU_SUCCESS;
  }

  return e_ret_status;
}


/**
  * @brief Secure Engine Firmware TAG verification (FW in non contiguous area).
  *        It handles Firmware TAG verification of a complete buffer by calling
  *        SE_AuthenticateFW_Init, SE_AuthenticateFW_Append and SE_AuthenticateFW_Finish inside the firewall.
  * @note: AES_GCM tag: In order to verify the TAG of a buffer, the function will re-encrypt it
  *        and at the end compare the obtained TAG with the one provided as input
  *        in pSE_GMCInit parameter.
  * @note: SHA-256 tag: a hash of the firmware is performed and compared with the digest stored in the Firmware header.
  * @param pSE_Status: Secure Engine Status.
  *        This parameter can be a value of @ref SE_Status_Structure_definition.
  * @param pSE_Metadata: Firmware metadata.
  * @param pSE_Payload: pointer to Payload Buffer descriptor.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
static SFU_ErrorStatus VerifyTagScatter(SE_StatusTypeDef *pSeStatus, SE_FwRawHeaderTypeDef *pSE_Metadata,
                                        SE_Ex_PayloadDescTypeDef  *pSE_Payload)
{
  SE_ErrorStatus se_ret_status = SE_ERROR;
  SFU_ErrorStatus sfu_ret_status = SFU_SUCCESS;
  /* Loop variables */
  uint32_t i = 0;
  uint32_t j = 0;
  /* Variables to handle the FW image chunks to be injected in the verification procedure and the result */
  int32_t fw_tag_len = 0;             /* length of the authentication tag to be verified */
  int32_t fw_verified_total_size = 0; /* number of (FW image) bytes that have been processed during the authentication check */
  int32_t fw_chunk_size;              /* size of a FW chunk to be verified */
  uint8_t fw_tag_output[SE_TAG_LEN] __attribute__((aligned(8)));    /* Authentication tag computed in this procedure (to be compared with the one stored in the FW metadata) */
  uint8_t fw_chunk[CHUNK_SIZE_SIGN_VERIFICATION] __attribute__((aligned(8)));         /* FW chunk produced by the verification procedure if any   */
  uint8_t fw_image_chunk[CHUNK_SIZE_SIGN_VERIFICATION] __attribute__((aligned(8)));   /* FW chunk provided as input to the verification procedure */
  /* Variables to handle the FW image (this will be split in chunks) */
  int32_t payloadsize;
  uint8_t *ppayload;
  uint32_t scatter_nb;

  /* Check the pointers allocation */
  if ((pSeStatus == NULL) || (pSE_Metadata == NULL) || (pSE_Payload == NULL))
  {
    return SFU_ERROR;
  }
  if ((pSE_Payload->pPayload[0] == NULL) || ((pSE_Payload->pPayload[1] == NULL) && pSE_Payload->PayloadSize[1]))
  {
    return SFU_ERROR;
  }
  if ((pSE_Payload->PayloadSize[0] + pSE_Payload->PayloadSize[1]) != pSE_Metadata->FwSize)
  {
    return SFU_ERROR;
  }
  /*  fix number of scatter block */
  if (pSE_Payload->PayloadSize[1])
  {
    scatter_nb = 2;
  }
  else
  {
    scatter_nb = 1;
  }


  /* Encryption process*/
  se_ret_status = SE_AuthenticateFW_Init(pSeStatus, pSE_Metadata);

  /* check for initialization errors */
  if ((se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK))
  {
    for (j = 0; j < scatter_nb; j++)
    {
      payloadsize = pSE_Payload->PayloadSize[j];
      ppayload = pSE_Payload->pPayload[j];
      i = 0;
      fw_chunk_size = CHUNK_SIZE_SIGN_VERIFICATION;

      while ((i < (payloadsize / CHUNK_SIZE_SIGN_VERIFICATION)) && (*pSeStatus == SE_OK) && (sfu_ret_status == SFU_SUCCESS))
      {

        sfu_ret_status = SFU_LL_FLASH_Read(fw_image_chunk, ppayload, fw_chunk_size) ;
        if (sfu_ret_status == SFU_SUCCESS)
        {
          se_ret_status = SE_AuthenticateFW_Append(pSeStatus, fw_image_chunk, fw_chunk_size,
                                                   fw_chunk, &fw_chunk_size);
        }
        else
        {
          *pSeStatus = SE_ERR_FLASH_READ;
          se_ret_status = SE_ERROR;
          sfu_ret_status = SFU_ERROR;
        }
        ppayload += fw_chunk_size;
        fw_verified_total_size += fw_chunk_size;
        i++;
      }
      /* this the last path , size can be smaller */
      fw_chunk_size = (uint32_t)pSE_Payload->pPayload[j] + pSE_Payload->PayloadSize[j] - (uint32_t)ppayload;
      if ((fw_chunk_size) && (se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK))
      {
        sfu_ret_status = SFU_LL_FLASH_Read(fw_image_chunk, ppayload, fw_chunk_size) ;
        if (sfu_ret_status == SFU_SUCCESS)
        {

          se_ret_status = SE_AuthenticateFW_Append(pSeStatus, fw_image_chunk,
                                                   payloadsize - i * CHUNK_SIZE_SIGN_VERIFICATION, fw_chunk,
                                                   &fw_chunk_size);
        }
        else
        {
          *pSeStatus = SE_ERR_FLASH_READ;
          se_ret_status = SE_ERROR;
          sfu_ret_status = SFU_ERROR;
        }
        fw_verified_total_size += fw_chunk_size;
      }
    }
  }

  if ((sfu_ret_status == SFU_SUCCESS) && (se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK))
  {
    if (fw_verified_total_size <= pSE_Metadata->FwSize)
    {
      /* Do the Finalization, check the authentication TAG*/
      fw_tag_len = sizeof(fw_tag_output);
      se_ret_status =   SE_AuthenticateFW_Finish(pSeStatus, fw_tag_output, &fw_tag_len);

      if ((se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK) && (fw_tag_len == SE_TAG_LEN))
      {
        for (i = 0; i < SE_TAG_LEN; i++)
        {
          /*
           * If the FW signature must be checked then the FW metadata structure will have a field named FwTag.
           * If the user does not want this check then this function will be empty anyway.
           *
           * This check is kept here and not moved in the crypto services on SE side
           * because we do not want the SE services to deal with the FLASH mapping.
           */
          if (fw_tag_output[i] != pSE_Metadata->FwTag[i])
          {
            *pSeStatus = SE_SIGNATURE_ERR;
            se_ret_status = SE_ERROR;
            sfu_ret_status = SFU_ERROR;
            break;
          }
        }
      }
      else
      {
        sfu_ret_status = SFU_ERROR;
      }
    }
    else
    {
      sfu_ret_status = SFU_ERROR;
    }
  }
  else
  {
    sfu_ret_status = SFU_ERROR;
  }
  return sfu_ret_status;
}


/**
  * @brief Secure Engine Firmware TAG verification (FW in non contiguous area).
  *        It handles Firmware TAG verification of a complete buffer by calling
  *        SE_AuthenticateFW_Init, SE_AuthenticateFW_Append and SE_AuthenticateFW_Finish inside the firewall.
  * @note: AES_GCM tag: In order to verify the TAG of a buffer, the function will re-encrypt it
  *        and at the end compare the obtained TAG with the one provided as input
  *        in pSE_GMCInit parameter.
  * @note: SHA-256 tag: a hash of the firmware is performed and compared with the digest stored in the Firmware header.
  * @param pSE_Status: Secure Engine Status.
  *        This parameter can be a value of @ref SE_Status_Structure_definition.
  * @param pSE_Metadata: Firmware metadata.
  * @param pPayload: pointer to Payload Buffer.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
static SFU_ErrorStatus VerifyTag(SE_StatusTypeDef *pSeStatus, SE_FwRawHeaderTypeDef *pSE_Metadata,
                                 uint8_t  *pPayload)
{
  SE_Ex_PayloadDescTypeDef  pse_payload;

  if (NULL == pSE_Metadata)
  {
    /* This should not happen */
    return SFU_ERROR;
  }

  pse_payload.pPayload[0] = pPayload;
  pse_payload.PayloadSize[0] = pSE_Metadata->FwSize;
  pse_payload.pPayload[1] = NULL;
  pse_payload.PayloadSize[1] = 0;

  return  VerifyTagScatter(pSeStatus, pSE_Metadata, &pse_payload);

}


/**
  * @brief Fill authenticated info in SE_FwImage.
  * @param SFU_APP_Status
  * @param pBuffer
  * @param BufferSize
  * @retval SFU_SUCCESS if successful, a SFU_ERROR otherwise.
  */
static SFU_ErrorStatus ParseFWInfo(SE_FwRawHeaderTypeDef *pFwHeader, uint8_t *pBuffer)
{
  /* Check the pointers allocation */
  if ((pFwHeader == NULL) || (pBuffer == NULL))
  {
    return SFU_ERROR;
  }
  memcpy(pFwHeader, pBuffer, sizeof(*pFwHeader));
  return SFU_SUCCESS;
}


/**
  * @brief  Check  that a file header is valid
  *         (Check if the header has a VALID tag)
  * @param  phdr  pointer to header to check
  * @retval SFU_ SUCCESS if valid, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus CheckHeaderValidated(uint8_t *phdr)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  if (memcmp(phdr + FW_INFO_TOT_LEN - FW_INFO_MAC_LEN, phdr + FW_INFO_TOT_LEN, MAGIC_LENGTH))
  {
    return  e_ret_status;
  }
  if (memcmp(phdr + FW_INFO_TOT_LEN - FW_INFO_MAC_LEN, phdr + FW_INFO_TOT_LEN + MAGIC_LENGTH, MAGIC_LENGTH))
  {
    return  e_ret_status;
  }
  return SFU_SUCCESS;
}


/**
  * @brief  Verify image signature of binary not contiguous in flash
  * @param  pSeStatus pointer giving the SE status result
  * @param  pFwImageHeader pointer to fw header
  * @param  pPayloadDesc description of non contiguous fw
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus VerifyFwSignatureScatter(SE_StatusTypeDef *pSeStatus, SE_FwRawHeaderTypeDef *pFwImageHeader,
                                                SE_Ex_PayloadDescTypeDef *pPayloadDesc)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  if ((pFwImageHeader == NULL) || (pPayloadDesc == NULL))
  {
    return  e_ret_status;
  }
  if ((pPayloadDesc->pPayload[0] == NULL) || (pPayloadDesc->PayloadSize[0] < SFU_IMG_IMAGE_OFFSET))
  {
    return e_ret_status;
  }

  /*
   * Adjusting the description of the way the Firmware is written in FLASH.
   */
  pPayloadDesc->pPayload[0] = (uint8_t *)((uint32_t)(pPayloadDesc->pPayload[0]) + SFU_IMG_IMAGE_OFFSET);

  /* The first part contains the execution offset so the payload size must be adjusted accordingly */
  pPayloadDesc->PayloadSize[0] = pPayloadDesc->PayloadSize[0]  - SFU_IMG_IMAGE_OFFSET;

  if (pFwImageHeader->FwSize <= pPayloadDesc->PayloadSize[0])
  {
    /* The firmware is written fully in a contiguous manner */
    pPayloadDesc->PayloadSize[0] = pFwImageHeader->FwSize;
    pPayloadDesc->PayloadSize[1] = 0;
    pPayloadDesc->pPayload[1] = NULL;
  }
  else
  {
    /*
     * The firmware is too big to be contained in the first payload slot.
     * So, the firmware is split in 2 non-contiguous parts
     */

    if ((pPayloadDesc->pPayload[1] == NULL)
        || (pPayloadDesc->PayloadSize[1] < (pFwImageHeader->FwSize - pPayloadDesc->PayloadSize[0])))
    {
      return  e_ret_status;
    }

    /* The second part contains the end of the firmware so the size is the FwSize - size already stored in the first area */
    pPayloadDesc->PayloadSize[1] = (pFwImageHeader->FwSize - pPayloadDesc->PayloadSize[0]);

  }

  /* Signature Verification */
  return VerifyTagScatter(pSeStatus, pFwImageHeader, pPayloadDesc);
}


/**
  * @brief  Check the magic from trailer or counter
  * @param  pMagicAddr: pointer to magic in Flash .
  * @param  pValidHeaderAddr: pointer to valid fw header in Flash
  * @param  pTestHeaderAddr: pointer to test fw header in Flash
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus CheckMagic(uint8_t *pMagicAddr, uint8_t *pValidHeaderAddr, uint8_t *pTestHeaderAddr)
{
  /*  as long as it is 128 bit key it is fine */
  uint8_t  magic[MAGIC_LENGTH];
  uint8_t  signature_valid[MAGIC_LENGTH / 2];
  uint8_t  signature_test[MAGIC_LENGTH / 2];

  if (SFU_LL_FLASH_Read(&magic, pMagicAddr, sizeof(magic)) == SFU_ERROR)
  {
    return SFU_ERROR;
  }
  if (SFU_LL_FLASH_Read(&signature_valid, (void *)((uint32_t)pValidHeaderAddr + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2),
                        sizeof(signature_valid)) == SFU_ERROR)
  {
    return SFU_ERROR;
  }
  if (SFU_LL_FLASH_Read(&signature_test, (void *)((uint32_t) pTestHeaderAddr + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2),
                        sizeof(signature_test)) == SFU_ERROR)
  {
    return SFU_ERROR;
  }
  if ((memcmp(magic, signature_valid, sizeof(signature_valid)))
      || (memcmp(&magic[MAGIC_LENGTH / 2], signature_test, sizeof(signature_test))))
  {
    return SFU_ERROR;
  }
  return SFU_SUCCESS;
}

/**
  * @brief  Write the minimum value possible on the flash
  * @param  pAddr: pointer to address to write .
  * @param  pValue: pointer to the value to write
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  *
  * @note This function should be FLASH dependent.
  *       We abstract this dependency thanks to the type SFU_LL_FLASH_write_t.
  *       See @ref SFU_LL_FLASH_write_t
  */
static SFU_ErrorStatus AtomicWrite(uint8_t *pAddr, SFU_LL_FLASH_write_t *pValue)
{
  SFU_FLASH_StatusTypeDef flash_if_info;

  return SFU_LL_FLASH_Write(&flash_if_info, pAddr, pValue, sizeof(SFU_LL_FLASH_write_t));
}

/**
  * @brief  Clean Magic value
  * @param  pMagicAddr: pointer to magic address to write .
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus CleanMagicValue(uint8_t *pMagicAddr)
{
  SFU_FLASH_StatusTypeDef flash_if_info;
  return SFU_LL_FLASH_Write(&flash_if_info, pMagicAddr, MAGIC_NULL, MAGIC_LENGTH);
}


/**
  * @brief  Compute Magic and Write Magic
  * @param  pMagicAddr: pointer to magic address to write .
  * @param  pHeaderValid: pointer to header of firmware validated
  * @param  pHeaderToInstall: pointer to header of firmware to install
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus WriteMagic(uint8_t *pMagicAddr, uint8_t *pHeaderValid, uint8_t *pHeaderToInstall)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  uint8_t  magic[MAGIC_LENGTH];
  int i;
  SFU_FLASH_StatusTypeDef flash_if_info;

  (void)SFU_LL_FLASH_Read(&magic[0], (void *)((uint32_t)pHeaderValid + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2), MAGIC_LENGTH / 2);
  (void)SFU_LL_FLASH_Read(&magic[MAGIC_LENGTH / 2], (void *)((uint32_t)pHeaderToInstall + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2), MAGIC_LENGTH / 2);
  for (i = 0; i < MAGIC_LENGTH ; i++)
  {
    /* Write MAGIC only if not MAGIC_NULL */
    if (magic[i] != 0)
    {
      e_ret_status = SFU_LL_FLASH_Write(&flash_if_info, pMagicAddr, magic, sizeof(magic));
      break;
    }
  }
  return  e_ret_status;
}

/**
  * @brief  Write Trailer Header
  * @param  pTestHeader: pointer in ram to header of fw to test
  * @param  pValidHeader: pointer in ram to header of valid fw to backup
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus WriteTrailerHeader(uint8_t *pTestHeader, uint8_t *pValidHeader)
{
  /* everything is in place , just compute from present data and write it */
  SFU_ErrorStatus e_ret_status;
  SFU_FLASH_StatusTypeDef flash_if_info;

  e_ret_status = SFU_LL_FLASH_Write(&flash_if_info, TRAILER_HDR_TEST, pTestHeader, FW_INFO_TOT_LEN);
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = SFU_LL_FLASH_Write(&flash_if_info, TRAILER_HDR_VALID, pValidHeader, FW_INFO_TOT_LEN);
  }
  return e_ret_status;
}

/**
  * @brief  Check CPY stucture from trailer
  * @param  pSE_Payload on success  described the positioning of the fw to restore  in 2 contiguous area.
  * @retval SFU_ SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus  GetTrailerInfo(SE_Ex_PayloadDescTypeDef  *pSE_Payload)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  /*  index = 0 , means no block shift,  */
  /*  index = 1 means last block swapped */
  /*  index = TRAILER_INDEX means all blocks swapped */
  int32_t i, index = TRAILER_INDEX;
  SFU_LL_FLASH_write_t trailer;
  for (i = TRAILER_INDEX - 1; i >= 0;  i--)
  {
    /*  this function will allow NMI to occur once and return (NMI normally is blocking)
     *  and check that NMI has occured or not */
    /*  NMI is set again as blocking after */
    e_ret_status = SFU_LL_FLASH_Read(&trailer, TRAILER_CPY(TRAILER_INDEX - 1 - i), sizeof(trailer));
    if ((e_ret_status == SFU_ERROR) || (memcmp(&trailer, SWAPPED, sizeof(trailer)) != 0))
    {
      e_ret_status = SFU_SUCCESS;
      break;
    }
    index = i;
  }
  /*  check that remaining CPY field are all at NOT_SWAPPED, the 1st not swapped is at index - 1*/
  for (i = index - 2 ; i >= 0; i--)
  {
    e_ret_status = SFU_LL_FLASH_Read(&trailer,  TRAILER_CPY(TRAILER_INDEX - 1 - i), sizeof(trailer));

    if ((e_ret_status == SFU_ERROR) || (memcmp(&trailer, NOT_SWAPPED, sizeof(trailer)) != 0))
    {
      e_ret_status = SFU_ERROR;
      break;
    }
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    /*  fill result pointer when provided */
    memset(pSE_Payload, 0, sizeof(*pSE_Payload));
    /*  Payload must have always the pPayload[0] set. */
    if (pSE_Payload)
    {

      if (index != 0)
      {
        pSE_Payload-> pPayload[0] = SFU_IMG_SLOT_0_REGION_BEGIN;
        pSE_Payload-> PayloadSize[0] = index * SFU_IMG_SWAP_REGION_SIZE;
        pSE_Payload-> pPayload[1] = (uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + index * SFU_IMG_SWAP_REGION_SIZE);
        pSE_Payload-> PayloadSize[1] = (TRAILER_INDEX - index) * SFU_IMG_SWAP_REGION_SIZE;
        if (pSE_Payload-> PayloadSize[1] >= TRAILER_SIZE)
        {
          pSE_Payload-> PayloadSize[1]  -= TRAILER_SIZE;
        }
        else
        {
          pSE_Payload-> PayloadSize[1] = 0;
        }
      }
      else
      {
        /*  all has been swap  */
        pSE_Payload-> pPayload[0] = (uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN);
        pSE_Payload-> PayloadSize[0] = TRAILER_INDEX * SFU_IMG_SWAP_REGION_SIZE - TRAILER_SIZE;
      }
    }
  }
  return e_ret_status;
}

/**
  * @brief  Recopy a flash block to flash
  * @param  dest address of flash to recopy (it must be the start of sector
  * @param  src  address of flash to copy from
  * @param  size it must be a multiple of flash sector size (and a multiple of 2)
  * @retval SFU_ SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus RecopyFlash(uint32_t dest, uint32_t src, uint32_t size)
{
  SFU_FLASH_StatusTypeDef flash_if_status;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;

  if ((dest == 0) || (src == 0))
  {
    return SFU_ERROR;
  }

  if (0 == size)
  {
    /* There is nothing to recopy so return now */
    return (SFU_SUCCESS);
  }

  SFU_LL_SECU_IWDG_Refresh();
  /* The FLASH erase is split becasue of the IWDG: this means that the size must also be a multiple of 2 (constraint) */
  e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)dest, size / 2U);
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);
  SFU_LL_SECU_IWDG_Refresh();
  e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)(dest + (size / 2U)), size / 2U);
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);
  SFU_LL_SECU_IWDG_Refresh();
  if (e_ret_status == SFU_SUCCESS)
  {
    uint8_t buffer[512];
    uint32_t number_of_chunk = (size + sizeof(buffer) - 1) / sizeof(buffer);
    uint32_t i, last_size, read_size = sizeof(buffer);
    last_size = size - (number_of_chunk - 1) * sizeof(buffer);

    /* number_of_chunk is always positive as we exit earlier when size == 0 */
    e_ret_status = SFU_ERROR;

    for (i = 0; i < number_of_chunk; i++)
    {
      SFU_LL_SECU_IWDG_Refresh();
      if (i  == (number_of_chunk - 1))
      {
        read_size = last_size;
      }
      e_ret_status = SFU_LL_FLASH_Read(buffer, (void *)src, read_size);
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
      if (e_ret_status == SFU_ERROR)
      {
        break;
      }
      SFU_LL_SECU_IWDG_Refresh();
      e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, (void *)dest, buffer, read_size);
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED)
      if (e_ret_status == SFU_ERROR)
      {
        break;
      }
      dest += read_size;
      src += read_size;
    }

  }

  return e_ret_status;
}

/**
  * @brief  Rollback by recopying the slot #1 content in slot #0 except the 3 VALID tags in header
  * @retval SFU_ SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus RollbackFlash(void)
{
  SFU_FLASH_StatusTypeDef flash_if_status;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint32_t dest = SFU_IMG_SLOT_0_REGION_BEGIN_VALUE;
  uint32_t size = SFU_IMG_SLOT_1_REGION_SIZE;
  uint32_t src = SFU_IMG_SLOT_1_REGION_BEGIN_VALUE;
  uint8_t buffer[SFU_IMG_IMAGE_OFFSET] __attribute__((aligned(8)));   /* A chunk must have the size of the offset to be able to skip the header */

  SFU_LL_SECU_IWDG_Refresh();
  e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)dest, (size / 2U));
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);
  SFU_LL_SECU_IWDG_Refresh();
  e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)(dest + (size / 2U)), (size / 2U));
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);
  SFU_LL_SECU_IWDG_Refresh();
  if (e_ret_status == SFU_SUCCESS)
  {
    uint32_t number_of_chunk = size / (sizeof(buffer));
    uint32_t i;
    if (number_of_chunk > 0)
    {
      e_ret_status = SFU_ERROR;

      /*
       * Starting from i=1 because we want to skip the header (the first chunk).
       * The header will be installed together with the VALID tags when validating the image at the
       * end of the rollback procedure.
       */
      dest += sizeof(buffer); /* skip the first chunk */
      src += sizeof(buffer);  /* skip the first chunk */

      for (i = 1; i < number_of_chunk; i++)
      {
        SFU_LL_SECU_IWDG_Refresh();
        e_ret_status = SFU_LL_FLASH_Read(buffer, (void *)src, sizeof(buffer));
        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
        if (e_ret_status == SFU_ERROR)
        {
          break;
        }
        SFU_LL_SECU_IWDG_Refresh();

        /* Copy the entire chunk */
        e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, (void *)dest, buffer, (sizeof(buffer)));

        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED)
        if (e_ret_status == SFU_ERROR)
        {
          break;
        }
        dest += sizeof(buffer);
        src += sizeof(buffer);
      }
    }
    /* else there is nothing to do so it is a success */
  }

  return e_ret_status;
}


/**
  * @brief  Erase the size of the swap area in a given slot sector.
  * @note   The erasure occurs at @: @slot + index*swap_area_size
  * @param  SlotNumber This is Slot #0 or Slot #1
  * @param  Index This is the number of "swap size" we jump from the slot start
  * @retval SFU_ SUCCESS if valid, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus EraseSlotIndex(uint32_t SlotNumber, uint32_t index)
{
  SFU_FLASH_StatusTypeDef flash_if_status;
  SFU_ErrorStatus e_ret_status;
  uint8_t *pbuffer;
  if (SlotNumber > (SFU_SLOTS - 1))
  {
    return SFU_ERROR;
  }
  pbuffer = (uint8_t *) SlotHeaderAddress[SlotNumber];
  pbuffer = pbuffer + (SFU_IMG_SWAP_REGION_SIZE * index);
  e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, pbuffer, SFU_IMG_SWAP_REGION_SIZE - 1) ;
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED)
  return e_ret_status;
}


/**
  * @brief  Recopy the slice of binary not already present in SLOT 0
  * @param  pPayloadDesc description of non contiguous fw
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus RecoverBinary(SE_Ex_PayloadDescTypeDef *payload_desc)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint32_t address_dest[2] = {(uint32_t)SFU_IMG_SLOT_0_REGION_BEGIN, (uint32_t)SFU_IMG_SLOT_0_REGION_BEGIN + SFU_IMG_SWAP_REGION_SIZE};
  uint32_t address_src[2] = { (uint32_t)SFU_IMG_SWAP_REGION_BEGIN, (uint32_t) SFU_IMG_SLOT_1_REGION_BEGIN};
  uint32_t size_to_copy[2] = { (uint32_t)SFU_IMG_SWAP_REGION_SIZE, (uint32_t)SFU_IMG_SLOT_1_REGION_SIZE - (uint32_t) SFU_IMG_SWAP_REGION_SIZE};
  uint32_t recopy_slice = 2, i;
  if (payload_desc->pPayload[0] == (uint8_t *) SFU_IMG_SLOT_0_REGION_BEGIN)
  {
    /*  install interrupted , the slot 0 does not need to be fully erased  */
    recopy_slice = 1;
    address_src[0] = (uint32_t)payload_desc->pPayload[1];
    address_dest[0] = (uint32_t)SFU_IMG_SLOT_0_REGION_BEGIN + (uint32_t) payload_desc->PayloadSize[0];
    size_to_copy[0] = (uint32_t)payload_desc->PayloadSize[1];
  }
  if (payload_desc->pPayload[0] == (uint8_t *)SFU_IMG_SLOT_1_REGION_BEGIN)
  {
    /*  installed version is not working well , re-install backed-up version */
    recopy_slice = 1;
    address_src[0] = (uint32_t)payload_desc->pPayload[0];
    size_to_copy[0] = (uint32_t)payload_desc->PayloadSize[0];
    address_dest[0] = (uint32_t)SFU_IMG_SLOT_0_REGION_BEGIN;
  }
  for (i = 0; i < recopy_slice; i++)
  {
    e_ret_status = RecopyFlash(address_dest[i], address_src[i], size_to_copy[i]);
    if (e_ret_status != SFU_SUCCESS)
    {
      break;
    }
  }

  return e_ret_status;
}

/**
  * @brief  Verify image signature of binary after decryption
  * @param  pSeStatus pointer giving the SE status result
  * @param  pFwImageHeader pointer to fw header
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus VerifyFwSignatureAfterDecrypt(SE_StatusTypeDef *pSeStatus,
                                                     SE_FwRawHeaderTypeDef *pFwImageHeader)
{
  SE_Ex_PayloadDescTypeDef payload_desc;
  /*
   * The values below are not necessarily matching the way the firmware
   * has been spread in FLASH but this is adjusted in @ref VerifyFwSignatureScatter.
   */
  payload_desc.pPayload[0] = (uint8_t *)SFU_IMG_SWAP_REGION_BEGIN;
  payload_desc.PayloadSize[0] = (uint32_t) SFU_IMG_SWAP_REGION_SIZE;
  payload_desc.pPayload[1] = (uint8_t *)SFU_IMG_SLOT_1_REGION_BEGIN;
  payload_desc.PayloadSize[1] = (uint32_t)SFU_IMG_SLOT_1_REGION_SIZE;
  return VerifyFwSignatureScatter(pSeStatus, pFwImageHeader, &payload_desc);
}

/**
 * @brief  Swap Slot 0 with decrypted FW to install
 *         With the 2 images implementation, installing a new Firmware Image means swapping Slot #0 and Slot #1.
 *         To perform this swap, the image to be installed is split in blocks of the swap region size: SFU_IMG_SLOT_1_REGION_SIZE / SFU_IMG_SWAP_REGION_SIZE blocks to be swapped .
 *         Each of these blocks is swapped using smaller chunks of SFU_IMG_CHUNK_SIZE size.
 *         The swap starts from the tail of the image and ends with the beginning of the image ("swap from tail to head").
 * @param None.
 * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
 */
static SFU_ErrorStatus SwapFirmwareImages(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SFU_FLASH_StatusTypeDef flash_if_status;
  int32_t index = 0;
  int32_t chunk;
  uint8_t  buffer[SFU_IMG_CHUNK_SIZE] __attribute__((aligned(8)));
  /* number_of_index is the number of blocks that are swapped (1 move = swap a block of SFU_IMG_SWAP_REGION_SIZE bytes) */
  uint32_t number_of_index = SFU_IMG_SLOT_1_REGION_SIZE / SFU_IMG_SWAP_REGION_SIZE;
  /* number_of_chunk is the number of chunks used to swap 1 block (moving a block of SFU_IMG_SWAP_REGION_SIZE bytes split in number_of_chunk chunks of SFU_IMG_CHUNK_SIZE bytes) */
  uint32_t number_of_chunk = SFU_IMG_SWAP_REGION_SIZE / SFU_IMG_CHUNK_SIZE;
  uint32_t write_len;

#if defined(SFU_VERBOSE_DEBUG_MODE)
  TRACE("\r\n\t  Swapping the Firmware Images (%d blocks): ", number_of_index);
#endif /* SFU_VERBOSE_DEBUG_MODE */

  /*  index is the block number from slot 0 to copy in the same block number from slot 1 (without trailer info)  */
  for (index = (number_of_index - 1) ; index >= 0; index--)
  {
    SFU_LL_SECU_IWDG_Refresh();

    TRACE(".");

    if (index != (number_of_index - 1))
    {
      /* Erase the destination address in slot #1 */
      e_ret_status = EraseSlotIndex(1, index); /* erase the size of "swap area" at @: slot #1 + index*swap_area_size*/

      if (e_ret_status ==  SFU_ERROR)
      {
        return SFU_ERROR;
      }
    }

    /* Copy the block from slot #0 to slot #1 (using "number_of_chunk - 1" chunks) */
    for (chunk = (number_of_chunk - 1); chunk >= 0 ; chunk--)
    {
      /* ignore return value,  no double ecc error is expected, area already read before */
      (void)SFU_LL_FLASH_Read(buffer, CHUNK_0_ADDR(index, chunk), sizeof(buffer));
      if (((uint32_t)CHUNK_1_ADDR(index, chunk)) < (uint32_t)TRAILER_BEGIN)
      {
        /*  write is possible length can be modified  */
        write_len = sizeof(buffer);
        if ((uint32_t)(CHUNK_1_ADDR(index, chunk) + write_len) > (uint32_t)TRAILER_BEGIN)
        {
          write_len = TRAILER_BEGIN - CHUNK_1_ADDR(index, chunk);
        }

        e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, CHUNK_1_ADDR(index, chunk), buffer,
                                          write_len);
        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
        if (e_ret_status == SFU_ERROR)
        {
          return SFU_ERROR;
        }
      }
    }

    /*
     * The block of the active firmware has been backed up.
     * The trailer is updated to memorize this: the CPY bytes at the appropriate index are set to SWAPPED.
     */
    e_ret_status  = AtomicWrite(TRAILER_CPY((TRAILER_INDEX - 1 - index)), (SFU_LL_FLASH_write_t *) SWAPPED);

    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
    if (e_ret_status == SFU_ERROR)
    {
      return e_ret_status;
    }

    /* The source address in slot #0 is erased */
    e_ret_status = EraseSlotIndex(0, index); /* erase the size of "swap area" at @: slot #0 + index*swap_area_size*/

    if (e_ret_status ==  SFU_ERROR)
    {
      return SFU_ERROR;
    }

    /* The appropriate block of slot #1 is copied at the address in slot #0 (installing the block of the new firmware image) */
    for (chunk = (number_of_chunk - 1); chunk >= 0 ; chunk--)
    {
      if (index == 0)
      {
        /* ignore return value,  no double ecc error is expected, area already read before */
        (void)SFU_LL_FLASH_Read(buffer, CHUNK_SWAP_ADDR(chunk), sizeof(buffer));
      }
      else
      {
        /* ignore return value,  no double ecc error is expected, area already read before */
        (void)SFU_LL_FLASH_Read(buffer, CHUNK_1_ADDR((index - 1), chunk), sizeof(buffer));
      }
      if (chunk == (number_of_chunk - 1) && (index == number_of_index - 1))
      {
        write_len = SFU_IMG_CHUNK_SIZE - TRAILER_INDEX * sizeof(SFU_LL_FLASH_write_t);
      }
      else
      {
        write_len = SFU_IMG_CHUNK_SIZE;
      }
      /*  don't copy header on slot 0, fix me chunk minimum size is SFU_IMG_IMAGE_OFFSET  */
      if ((index == 0) && (chunk == 0))
      {
        write_len = write_len - SFU_IMG_IMAGE_OFFSET;
      }
      e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, CHUNK_0_ADDR_MODIFIED(index, chunk), buffer,
                                        write_len);
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
      if (e_ret_status == SFU_ERROR)
      {
        return SFU_ERROR;
      }
    }
  }
  return e_ret_status;
}

/**
  * @brief Decrypt Image in slot #1
  * @ note Decrypt is done from slot 1 to slot 1 + swap with 2 images (swap contains 1st sector)
  * @param  pFwImageHeader
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus DecryptImageInSlot1(SE_FwRawHeaderTypeDef *pFwImageHeader)
{
  SFU_ErrorStatus  e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status;
  SE_ErrorStatus   se_ret_status;
  uint32_t NumberOfChunkPerSwap = SFU_IMG_SWAP_REGION_SIZE / SFU_IMG_CHUNK_SIZE;
  SFU_FLASH_StatusTypeDef flash_if_status;
  /*  chunk size is the maximum , the 1st block can be smaller */
  /*  the chunk is static to avoid  large stack */
  uint8_t fw_decrypted_chunk[SFU_IMG_CHUNK_SIZE] __attribute__((aligned(8)));
  uint8_t fw_encrypted_chunk[SFU_IMG_CHUNK_SIZE] __attribute__((aligned(8)));
  uint8_t *pfw_source_address;
  uint32_t fw_dest_address_write = 0U;
  uint32_t fw_dest_erase_address = 0U;
  int32_t fw_decrypted_total_size = 0;
  int32_t size;
  int32_t fw_decrypted_chunk_size;
  int32_t fw_tag_len = 0;
  uint8_t fw_tag_output[SE_TAG_LEN];
  uint32_t pass_index = 0;
  uint32_t erase_index = 0;

  if ((pFwImageHeader == NULL))
  {
    return e_ret_status;
  }

  pass_index = 0;

  /* Decryption process*/
  se_ret_status = SE_Decrypt_Init(&e_se_status, pFwImageHeader);
  if ((se_ret_status == SE_SUCCESS) && (e_se_status == SE_OK))
  {
    e_ret_status = SFU_SUCCESS;
    size = SFU_IMG_CHUNK_SIZE;

    /* Decryption loop*/
    while ((e_ret_status == SFU_SUCCESS) && (fw_decrypted_total_size < (pFwImageHeader->FwSize)) && (e_se_status == SE_OK))
    {
      if (pass_index == NumberOfChunkPerSwap)
      {
        fw_dest_address_write = (uint32_t) SFU_IMG_SLOT_1_REGION_BEGIN;
        fw_dest_erase_address =  fw_dest_address_write;
        erase_index = NumberOfChunkPerSwap;
      }
      if (pass_index == 0)
      {
        pfw_source_address = (uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + SFU_IMG_IMAGE_OFFSET);
        fw_dest_erase_address = (uint32_t)SFU_IMG_SWAP_REGION_BEGIN;
        fw_dest_address_write = fw_dest_erase_address + SFU_IMG_IMAGE_OFFSET;
        fw_decrypted_chunk_size = sizeof(fw_decrypted_chunk) - SFU_IMG_IMAGE_OFFSET;
      }
      else
      {
        fw_decrypted_chunk_size = sizeof(fw_decrypted_chunk);

        /* For the last 2 pass, divide by 2 remaining buffer to ensure that :
         *     - chunk size greater than 16 bytes : minimum size of a block to be decrypted
         *     - except last one chunk size is 16 bytes aligned
         *
         * Pass n - 1 : remaining bytes / 2 with (16 bytes alignment for crypto AND sizeof(SFU_LL_FLASH_write_t) for flash programming)
         * Pass n : remaining bytes
         */

        /* Last pass : n */
        if ((pFwImageHeader->FwSize - fw_decrypted_total_size) < fw_decrypted_chunk_size)
        {
          fw_decrypted_chunk_size = pFwImageHeader->FwSize - fw_decrypted_total_size;
        }
        /* Previous pass : n - 1 */
        else if ((pFwImageHeader->FwSize - fw_decrypted_total_size) < ((2 * fw_decrypted_chunk_size) - 16U))
        {
          fw_decrypted_chunk_size = ((pFwImageHeader->FwSize - fw_decrypted_total_size) / 32U) * 16U;

          /* Set dimension to the appropriate length for FLASH programming.
           * Example: 64-bit length for L4.
           */
          if ((fw_decrypted_chunk_size & ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - 1U)) != 0)
          {
            fw_decrypted_chunk_size = fw_decrypted_chunk_size + ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - (fw_decrypted_chunk_size % (uint32_t)sizeof(SFU_LL_FLASH_write_t)));
          }
        }
        /* All others pass */
        else
        {
          /* nothing */
        }
      }

      size = fw_decrypted_chunk_size;

      /* Decrypt Append*/
      e_ret_status = SFU_LL_FLASH_Read(fw_encrypted_chunk, pfw_source_address, size);
      if (e_ret_status == SFU_ERROR)
      {
        break;
      }
      if (size != 0)
      {
        se_ret_status = SE_Decrypt_Append(&e_se_status, (uint8_t *)fw_encrypted_chunk, size, fw_decrypted_chunk, &fw_decrypted_chunk_size);
      }
      else
      {
        e_ret_status = SFU_SUCCESS;
        fw_decrypted_chunk_size = 0;
      }
      if ((se_ret_status == SE_SUCCESS) && (e_se_status == SE_OK) && (fw_decrypted_chunk_size == size))
      {
        /* Erase Page */
        if (pass_index == erase_index)
        {
          SFU_LL_SECU_IWDG_Refresh();
          e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)fw_dest_erase_address, SFU_IMG_SWAP_REGION_SIZE - 1) ;
          erase_index += NumberOfChunkPerSwap;
          fw_dest_erase_address += SFU_IMG_SWAP_REGION_SIZE;
        }
        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);

        if (e_ret_status == SFU_SUCCESS)
        {
          /* For last pass with remaining size not aligned on 16 bytes : Set dimension AFTER decrypt to the appropriate length for FLASH programming.
           * Example: 64-bit length for L4.
           */
          if ((size & ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - 1U)) != 0)
          {
            /* By construction, SFU_IMG_CHUNK_SIZE is a multiple of sizeof(SFU_LL_FLASH_write_t) so there is no risk to read out of the buffer */
            size = size + ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - (size % (uint32_t)sizeof(SFU_LL_FLASH_write_t)));
          }

          /* Write Decrypted Data in Flash - size has to be 32-bit aligned */
          e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, (void *)fw_dest_address_write,  fw_decrypted_chunk, size);
          StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);

          if (e_ret_status == SFU_SUCCESS)
          {
            /* Update flash pointer */
            fw_dest_address_write  += (size);

            /* Update source pointer */
            pfw_source_address += size;
            fw_decrypted_total_size += size;
            memset(fw_decrypted_chunk, 0xff, sizeof(fw_decrypted_chunk));
            pass_index += 1;

          }
        }
      }
    }
  }

#if (SFU_IMAGE_PROGRAMMING_TYPE == SFU_ENCRYPTED_IMAGE)
#if defined(SFU_VERBOSE_DEBUG_MODE)
  TRACE("\r\n\t  %d bytes of ciphertext decrypted.", fw_decrypted_total_size);
#endif /* SFU_VERBOSE_DEBUG_MODE */
#endif /* SFU_ENCRYPTED_IMAGE */

  if ((se_ret_status == SE_SUCCESS) && (e_ret_status == SFU_SUCCESS) && (e_se_status == SE_OK))
  {
    /* Do the Finalization, check the authentication TAG*/
    fw_tag_len = sizeof(fw_tag_output);
    se_ret_status = SE_Decrypt_Finish(&e_se_status, fw_tag_output, &fw_tag_len);
    if ((se_ret_status != SE_SUCCESS) || (e_se_status != SE_OK))
    {
      e_ret_status = SFU_ERROR;
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  Decrypt fails at Finalization stage.");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    }
  }
  else
  {
    e_ret_status = SFU_ERROR;
  }
  return e_ret_status;
}


/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Exported_Functions Exported Functions
  * @brief Functions used by fwimg_services
  * @note All these functions are also listed in the Common services (High Level and Low Level Services).
  * @{
  */

/**
  * @brief  Initialize the SFU APP.
  * @param  None
  * @note   Not used in Alpha version -
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_IMG_InitStatusTypeDef SFU_IMG_CoreInit(void)
{
  SFU_IMG_InitStatusTypeDef e_ret_status = SFU_IMG_INIT_OK;

  /*
   * When there is no valid FW in slot 0, the fw_header_validated array is filled with 0s.
   * When installing a first FW (after local download) this means that WRITE_TRAILER_MAGIC will write a SWAP magic starting with 0s.
   * This causes an issue when calling CLEAN_TRAILER_MAGIC (because of this we added an erase that generated side-effects).
   * To avoid all these problems we can initialize fw_header_validated with a non-0  value.
   */
  memset(fw_header_validated, 0xFE, sizeof(fw_header_validated));

  /*
   * Sanity check: let's make sure the slot sizes are correct
   * to avoid discrepancies between linker definitions and constants in sfu_fwimg_regions.h
   */
  if (!(SFU_IMG_REGION_IS_SAME_SIZE(SFU_IMG_SLOT_0_REGION_SIZE, SFU_IMG_SLOT_1_REGION_SIZE)))
  {
    TRACE("\r\n= [FWIMG] SLOT size issue SLOT0 %d != SLOT1 %d\r\n", SFU_IMG_SLOT_0_REGION_SIZE, SFU_IMG_SLOT_1_REGION_SIZE);
    e_ret_status = SFU_IMG_INIT_SLOTS_SIZE_ERROR;
  }
  else if (!(SFU_IMG_REGION_IS_MULTIPLE(SFU_IMG_SLOT_0_REGION_SIZE, SFU_IMG_SWAP_REGION_SIZE)))
  {
    TRACE("\r\n= [FWIMG] The image slot size (%d) must be a multiple of the swap region size (%d)\r\n", SFU_IMG_SLOT_0_REGION_SIZE, SFU_IMG_SWAP_REGION_SIZE);
    e_ret_status = SFU_IMG_INIT_SLOTS_SIZE_ERROR;
  }
  else
  {
    e_ret_status = SFU_IMG_INIT_OK;
    TRACE("\r\n= [FWIMG] Slot #0 @: %x / Slot #1 @: %x / Swap @: %x", SFU_IMG_SLOT_0_REGION_BEGIN_VALUE, SFU_IMG_SLOT_1_REGION_BEGIN_VALUE, SFU_IMG_SWAP_REGION_BEGIN_VALUE);
  }

  /*
   * Sanity check: let's make sure the swap region size and the slot size is correct with regards to the swap procedure constraints.
   * The swap procedure cannot succeed if the trailer info size is bigger than what a chunk used for swapping can carry.
   */
  if (((int32_t)(SFU_IMG_CHUNK_SIZE - (TRAILER_INDEX * sizeof(SFU_LL_FLASH_write_t)))) < 0)
  {
    e_ret_status = SFU_IMG_INIT_SWAP_SETTINGS_ERROR;
    TRACE("\r\n= [FWIMG] %d bytes required for the swap metadata is too much, please tune your settings", (TRAILER_INDEX * sizeof(SFU_LL_FLASH_write_t)));
  } /* else the swap settings are fine from a metadata size perspective */

  /*
   * Sanity check: let's make sure the swap region size and the slot size is correct with regards to the swap procedure constraints.
   * The swap region size must be a multiple of the chunks size used to do the swap.
   */
#if defined(__GNUC__)
  __IO uint32_t swap_size = SFU_IMG_SWAP_REGION_SIZE, swap_chunk = SFU_IMG_CHUNK_SIZE;
  if (0 != ((uint32_t)(swap_size % swap_chunk)))
#else
  if (0 != ((uint32_t)(SFU_IMG_SWAP_REGION_SIZE % SFU_IMG_CHUNK_SIZE)))
#endif /* __GNUC__ */
  {
    e_ret_status = SFU_IMG_INIT_SWAP_SETTINGS_ERROR;
    TRACE("\r\n= [FWIMG] The swap procedure uses chunks of %d bytes but the swap region size (%d) is not a multiple of this: please tune your settings", SFU_IMG_CHUNK_SIZE, SFU_IMG_SWAP_REGION_SIZE);
  } /* else the swap settings are fine from a chunk size perspective */

  /*
   * Sanity check: let's make sure the chunks used for the swap procedure are big enough with regards to the offset giving the start @ of the firmware
   */
  if (((int32_t)(SFU_IMG_CHUNK_SIZE - SFU_IMG_IMAGE_OFFSET)) < 0)
  {
    e_ret_status = SFU_IMG_INIT_SWAP_SETTINGS_ERROR;
    TRACE("\r\n= [FWIMG] The swap procedure uses chunks of %d bytes but the firmware start offset is %d bytes: please tune your settings", SFU_IMG_CHUNK_SIZE, SFU_IMG_IMAGE_OFFSET);
  } /* else the swap settings are fine from a firmware start offset perspective */
  /*
   * Sanity check: let's make sure all slots are properly aligned with regards to flash constraints
   */
  if (!IS_ALIGNED(SFU_IMG_SLOT_0_REGION_BEGIN_VALUE))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] slot 0 (%x) is not properly aligned: please tune your settings", SFU_IMG_SLOT_0_REGION_BEGIN_VALUE);
  } /* else slot 0 is properly aligned */

  if (!IS_ALIGNED(SFU_IMG_SLOT_1_REGION_BEGIN_VALUE))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] slot 1 (%x) is not properly aligned: please tune your settings", SFU_IMG_SLOT_1_REGION_BEGIN_VALUE);
  } /* else slot 1 is properly aligned */
  if (!IS_ALIGNED(SFU_IMG_SWAP_REGION_BEGIN_VALUE))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] swap region (%x) is not properly aligned: please tune your settings", SFU_IMG_SWAP_REGION_BEGIN_VALUE);
  } /* else swap region is properly aligned */

  /*
   * Sanity check: let's make sure the MAGIC patterns used by the internal algorithms match the FLASH constraints.
   */
  if (0 != (uint32_t)(MAGIC_LENGTH % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] magic size (%d) is not matching the FLASH constraints", MAGIC_LENGTH);
  } /* else the MAGIC patterns size is fine with regards to FLASH constraints */

  /*
   * Sanity check: let's make sure the Firmware Header Length is fine with regards to FLASH constraints
   */
  if (0 != (uint32_t)(SE_FW_HEADER_TOT_LEN % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    /* The code writing the FW header in FLASH requires the FW Header length to match the FLASH constraints */
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] FW Header size (%d) is not matching the FLASH constraints", SE_FW_HEADER_TOT_LEN);
  } /* else the FW Header Length is fine with regards to FLASH constraints */

  /*
   * Sanity check: let's make sure the chunks used for decrypt match the FLASH constraints
   */
  if (0 != (uint32_t)(SFU_IMG_CHUNK_SIZE % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    /* The size of the chunks used to store the decrypted data must be a multiple of the FLASH write length */
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] Decrypt chunk size (%d) is not matching the FLASH constraints", SFU_IMG_CHUNK_SIZE);
  } /* else the decrypt chunk size is fine with regards to FLASH constraints */

  /*
   * Sanity check: let's make sure the chunk size used for decrypt is fine with regards to AES CBC constraints.
   *               This block alignment constraint does not exist for AES GCM but we do not want to specialize the code for a specific crypto scheme.
   */
  if (0 != (uint32_t)((uint32_t)SFU_IMG_CHUNK_SIZE % (uint32_t)AES_BLOCK_SIZE))
  {
    /* For AES CBC block encryption/decryption the chunks must be aligned on the AES block size */
    e_ret_status = SFU_IMG_INIT_CRYPTO_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] Chunk size (%d) is not matching the AES CBC constraints", SFU_IMG_CHUNK_SIZE);
  }
  /*
   * Sanity check: let's make sure the slot 0 does not overlap SB code area protected by WRP)
   */
  if (((SFU_IMG_SLOT_0_REGION_BEGIN_VALUE - FLASH_BASE) / FLASH_PAGE_SIZE) <= SFU_PROTECT_WRP_PAGE_END_1)
  {
    TRACE("\r\n= [FWIMG] SLOT 0 overlaps SBSFU code area protected by WRP\r\n");
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
  }

  /*
   * Sanity check: let's make sure the slot 1 does not overlap SB code area protected by WRP)
   */
  if (((SFU_IMG_SLOT_1_REGION_BEGIN_VALUE - FLASH_BASE) / FLASH_PAGE_SIZE) <= SFU_PROTECT_WRP_PAGE_END_1)
  {
    TRACE("\r\n= [FWIMG] SLOT 1 overlaps SBSFU code area protected by WRP\r\n");
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
  }

  /*
   * Sanity check: let's make sure the swap area does not overlap SB code area protected by WRP)
   */
  if (((SFU_IMG_SWAP_REGION_BEGIN_VALUE - FLASH_BASE) / FLASH_PAGE_SIZE) <= SFU_PROTECT_WRP_PAGE_END_1)
  {
    TRACE("\r\n= [FWIMG] SWAP overlaps SBSFU code area protected by WRP\r\n");
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
  }

  /* FWIMG core initialization completed */
  return e_ret_status;
}


/**
  * @brief  DeInitialize the SFU APP.
  * @param  None
  * @note   Not used in Alpha version.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_CoreDeInit(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  /*
   *  To Be Reviewed after Alpha.
  */
  return e_ret_status;
}


/**
  * @brief  Check that the FW in slot #0 has been tagged as valid by the bootloader.
  *         This function populates the FWIMG module variable: 'fw_header_validated'
  * @param  None.
  * @note It is up to the caller to make sure that slot#0 contains a valid firmware first.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus  SFU_IMG_CheckSlot0FwValid()
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint8_t fw_header_slot[FW_INFO_TOT_LEN + VALID_SIZE];

  /*
   * It is up to the caller to make sure that slot#0 contains a valid firmware before calling this function.
   * Extract the header.
   */
  e_ret_status = SFU_LL_FLASH_Read(fw_header_slot, (uint8_t *) SFU_IMG_SLOT_0_REGION_BEGIN, sizeof(fw_header_slot));

  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Check the header is tagged as VALID */
    e_ret_status = CheckHeaderValidated(fw_header_slot);
  }

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Populating the FWIMG module variable with the header */
    e_ret_status = SFU_LL_FLASH_Read(fw_header_validated, (uint8_t *) SFU_IMG_SLOT_0_REGION_BEGIN, sizeof(fw_header_validated));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  }

  return (e_ret_status);
}


/**
  * @brief  Verify Image Header in the slot given as a parameter
  * @param  pFwImageHeader pointer to a structure to handle the header info (filled by this function)
  * @param  SlotNumber slot #0 = active Firmware , slot #1 = downloaded image or backed-up image, slot #2 = swap region
  * @note   Not used in Alpha version -
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_GetFWInfoMAC(SE_FwRawHeaderTypeDef *pFwImageHeader, uint32_t SlotNumber)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint8_t *pbuffer;
  uint8_t  buffer[FW_INFO_TOT_LEN];
  if ((pFwImageHeader == NULL) || (SlotNumber > (SFU_SLOTS)))
  {
    return SFU_ERROR;
  }
  pbuffer = (uint8_t *) SlotHeaderAddress[SlotNumber];

  /* use api read to detect possible ECC error */
  e_ret_status = SFU_LL_FLASH_Read(buffer, pbuffer, sizeof(buffer));
  if (e_ret_status != SFU_ERROR)
  {

    e_ret_status = VerifyFwRawHeaderTag(buffer);

    if (e_ret_status != SFU_ERROR)
    {
      e_ret_status = ParseFWInfo(pFwImageHeader, buffer);
    }
  }
  /*  cleaning */
  memset(buffer, 0, FW_INFO_TOT_LEN);
  return e_ret_status;
}


/**
  * @brief  Verify image signature of binary contiguous in flash
  * @param  pSeStatus pointer giving the SE status result
  * @param  pFwImageHeader pointer to fw header
  * @param  SlotNumber flash slot to check
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_VerifyFwSignature(SE_StatusTypeDef  *pSeStatus, SE_FwRawHeaderTypeDef *pFwImageHeader,
                                          uint32_t SlotNumber)
{
  uint8_t *pbuffer;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  /*  put it OK, to discriminate error in SFU FWIMG parts */
  *pSeStatus = SE_OK;
  if ((pFwImageHeader == NULL) || (SlotNumber > (SFU_SLOTS - 1)))
  {
    return SFU_ERROR;
  }
  pbuffer = (uint8_t *)(SlotHeaderAddress[SlotNumber] + SFU_IMG_IMAGE_OFFSET);


  /* Signature Verification */
  e_ret_status = VerifyTag(pSeStatus, pFwImageHeader, (uint8_t *) pbuffer);

  return e_ret_status;
}


/**
  * @brief  Write a valid header in slot #0
  * @param  Address of the header to be installed in slot #0
  * @retval SFU_ SUCCESS if valid, a SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_WriteHeaderValidated(uint8_t *pHeader)
{
  /*  get header from counter area  */
  SFU_ErrorStatus e_ret_status;
  SFU_FLASH_StatusTypeDef flash_if_status;
  /* The VALID magic is made of: 3*active FW header */
  uint8_t  FWInfoInput[FW_INFO_TOT_LEN + VALID_SIZE];
  uint8_t *pDestFWInfoInput, *pSrcFWInfoInput;
  uint32_t i, j;

  e_ret_status =  SFU_LL_FLASH_Read(FWInfoInput, pHeader,  FW_INFO_TOT_LEN);
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  /*  compute Validated Header */
  if (e_ret_status == SFU_SUCCESS)
  {
    for (i = 0U, pDestFWInfoInput = FWInfoInput + FW_INFO_TOT_LEN; i < 3U; i++)
    {
      for (j = 0U, pSrcFWInfoInput = FWInfoInput + FW_INFO_TOT_LEN - FW_INFO_MAC_LEN; j < MAGIC_LENGTH; j++)
      {
        *pDestFWInfoInput = *pSrcFWInfoInput;
        pDestFWInfoInput++;
        pSrcFWInfoInput++;
      }
    }
    /*  write in flash  */
    e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, SFU_IMG_SLOT_0_REGION_BEGIN, FWInfoInput, sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
  }
  return e_ret_status;
}


/**
  * @brief  check trailer and image to restore
  * @param  pSE_Payload on success  described the positioning of the fw to restore  in 2 contiguous area.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus  SFU_IMG_CheckTrailerValid(SE_Ex_PayloadDescTypeDef  *pSE_Payload)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef      e_se_status;
  SE_Ex_PayloadDescTypeDef  se_payload;
  SE_FwRawHeaderTypeDef xFwImageHeader;
  SFU_LL_FLASH_write_t cpy;
  /*  2 header must be validated  */
  uint8_t  FWInfoInput[FW_INFO_TOT_LEN];
  uint8_t  FWInfoValid[FW_INFO_TOT_LEN + VALID_SIZE];

  /*  check trailer Magic  */
  e_ret_status =  CHECK_TRAILER_MAGIC;
  if (e_ret_status == SFU_SUCCESS)
  {
    /*  Check hdr valid fied */
    e_ret_status =  SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_VALID,  sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = VerifyFwRawHeaderTag(FWInfoInput);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    memcpy(SFU_APP_HDR_VALID_FWInfoInputAuthenticated, FWInfoInput, sizeof(SFU_APP_HDR_VALID_FWInfoInputAuthenticated));

    /* check test field */
    e_ret_status =  SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_TEST,  sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = VerifyFwRawHeaderTag(FWInfoInput);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    memcpy(SFU_APP_HDR_TEST_FWInfoInputAuthenticated, FWInfoInput, sizeof(SFU_APP_HDR_TEST_FWInfoInputAuthenticated));

    /*swap header  is now  valid.
     * image backup must be valid , compare header, then check
     * image valid,  then compute hash  */
    /* store slot header valid in ram  */
    e_ret_status =  SFU_LL_FLASH_Read(&cpy, TRAILER_CPY((TRAILER_INDEX) - 1), sizeof(SFU_LL_FLASH_write_t));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
    /*  if read failed, we consider as not swapped */
    if (e_ret_status == SFU_ERROR)
    {
      memcpy(&cpy, NOT_SWAPPED, sizeof(cpy));
    }
    e_ret_status = SFU_LL_FLASH_Read(FWInfoValid, SLOT_HDR_VALID(cpy), sizeof(FWInfoValid));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  }
  if (e_ret_status == SFU_SUCCESS)
  {

    e_ret_status = SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_VALID, sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    if (memcmp(FWInfoValid, FWInfoInput,  FW_INFO_TOT_LEN))
    {
      e_ret_status = SFU_ERROR;
    }
  }
  /*  fw version to backup is validated */
  /*  if (e_ret_status == SFU_SUCCESS)*/
  /*  e_ret_status = CheckHeaderValidated(FWInfoValid);*/
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = GetTrailerInfo(&se_payload);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = ParseFWInfo(&xFwImageHeader, SFU_APP_HDR_VALID_FWInfoInputAuthenticated);
  }

  if (e_ret_status == SFU_SUCCESS)
  {
    /*  check image is to restore is correct  */
    if (pSE_Payload)
    {
      memcpy(pSE_Payload, &se_payload, sizeof(se_payload));
    }
    e_ret_status =  VerifyFwSignatureScatter(&e_se_status, &xFwImageHeader, &se_payload);
  }

  if (e_ret_status == SFU_ERROR)
  {
    memset(SFU_APP_HDR_VALID_FWInfoInputAuthenticated, 0, sizeof(SFU_APP_HDR_VALID_FWInfoInputAuthenticated));
    memset(SFU_APP_HDR_TEST_FWInfoInputAuthenticated, 0, sizeof(SFU_APP_HDR_TEST_FWInfoInputAuthenticated));
    memset(pSE_Payload, 0, sizeof(*pSE_Payload));
  }
  return e_ret_status;
}


/**
 * @brief This function is dedicated (only) to the re-installation of a previous valid firmware when an active firmware has been deemed as invalid.
 * @param  None.
 * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
 */
SFU_ErrorStatus SFU_IMG_RollBack(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  /*
   * When we reach this point, we know we have a valid FW in slot #1.
   * We need to:
   *    1. erase slot #0
   *    2. recopy slot #1 in slot #0 except the VALID tags in header
   *    3. validate the FW in slot #0 to finalize the recovery
   */
  e_ret_status = RollbackFlash();

  /* After the rollback procedure we still have the backed-up FW in slot #1: this is now the same as the active FW  */
  if (e_ret_status == SFU_SUCCESS)
  {
    /* validate immediately the new active FW */
    e_ret_status = SFU_IMG_Validation(SLOT_1_HDR); /* the header of the newly installed FW (backed-up FW) is in the slot #1  */

    if (SFU_SUCCESS == e_ret_status)
    {
      /*
       * We have just installed a new Firmware so we need to reset the counters
       */

      /*
       * Consecutive init counter:
       *   a reset of this counter is useless as long as the installation is triggered by a Software Reset
       *   and this SW Reset is considered as a valid boot-up cause (because the counter has already been reset in SFU_BOOT_ManageResetSources()).
       *   If you change this behavior in SFU_BOOT_ManageResetSources() then you need to reset the consecutive init counter here.
       */

    } /* else do nothing we will end up in local download */
  }

  return e_ret_status;
}

/**
  * @brief  Recover from an interrupted FW installation
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_Recover(void)
{
  SFU_ErrorStatus e_ret_status;

  e_ret_status = RecoverBinary(&fw_desc_to_recover);

  if (e_ret_status == SFU_SUCCESS)
  {
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n=         Recovery procedure completed.");
#endif /* SFU_VERBOSE_DEBUG_MODE */

    e_ret_status = CLEAN_TRAILER_MAGIC;

#if defined(SFU_VERBOSE_DEBUG_MODE)
    if (SFU_ERROR == e_ret_status)
    {
      TRACE("\r\n=         Recovery procedure cannot be finalized!");
    }
#endif /* SFU_VERBOSE_DEBUG_MODE */
  }

  return e_ret_status;
}

/**
  * @brief  Check that there is an Image to Install
  * @retval SFU_SUCCESS if Image can be installed, a SFU_ERROR  otherwise.
  */
SFU_ErrorStatus SFU_IMG_FirmwareToInstall(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint8_t *pbuffer = (uint8_t *) SFU_IMG_SWAP_REGION_BEGIN;
  uint8_t  buffer[INSTALLED_LENGTH];
  uint8_t fw_header_slot[FW_INFO_TOT_LEN + VALID_SIZE]; /* header + VALID tags */

  /*
   * The anti-rollback check is implemented at installation stage (SFU_IMG_InstallNewVersion)
   * to be able to handle a specific error cause.
   */

  /*  Loading the header to verify it and check it is followed by 0s until INSTALLED_LENGTH */
  e_ret_status = SFU_LL_FLASH_Read(fw_header_to_test, pbuffer, sizeof(fw_header_to_test));
  if (e_ret_status == SFU_SUCCESS)
  {
    /*  check swap header */
    e_ret_status = SFU_IMG_GetFWInfoMAC(&fw_image_header_to_test, 2);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    uint32_t i;
    e_ret_status = SFU_LL_FLASH_Read(buffer, pbuffer, sizeof(buffer));
    for (i = FW_INFO_TOT_LEN; i < INSTALLED_LENGTH; i++)
    {
      if (buffer[i] != 0)
      {
        e_ret_status = SFU_ERROR;
        break;
      }

    }
  }

  if (e_ret_status == SFU_SUCCESS)
  {
    /*  compare the header in slot 1 with the header in swap */
    pbuffer = (uint8_t *) SFU_IMG_SLOT_1_REGION_BEGIN;
    e_ret_status = SFU_LL_FLASH_Read(fw_header_slot, pbuffer, sizeof(fw_header_slot));
    if (e_ret_status == SFU_SUCCESS)
    {
      /* image header in slot 1 not consistent with swap header */
      uint32_t trailer_begin = (uint32_t) TRAILER_BEGIN;
      uint32_t end_of_test_image = ((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + fw_image_header_to_test.FwSize + SFU_IMG_IMAGE_OFFSET);
      uint32_t end_of_valid_image = ((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + fw_image_header_validated.FwSize +  SFU_IMG_IMAGE_OFFSET);
      /* the header in swap must be the same as the header in slot #1 */
      int ret = memcmp(fw_header_slot, fw_header_to_test, sizeof(fw_header_to_test)); /* compare only the header length */
      /*
       * The image to install must not be already validated otherwise this means we try to install a backed-up FW image.
       * The only case to re-install a backed-up image is the rollback use-case, not the new image installation use-case.
       */
      e_ret_status = CheckHeaderValidated(fw_header_slot);
      /* Check if there is enough room for the trailers */
      if ((trailer_begin < end_of_test_image) || (trailer_begin < end_of_valid_image) || (ret) || (SFU_SUCCESS == e_ret_status))
      {
        /*
         * These error causes are not memorized in the BootInfo area because there won't be any error handling procedure
         * if this function returns that no new firmware can be installed (as this may be a normal case).
         */
        e_ret_status = SFU_ERROR;

#if defined(SFU_VERBOSE_DEBUG_MODE)
        /* Display the debug message(s) only once */
        if (1U == initialDeviceStatusCheck)
        {
          if ((trailer_begin < end_of_test_image) || (trailer_begin < end_of_valid_image))
          {
            TRACE("\r\n= [FWIMG] The binary image to be installed and/or the image to be backed-up overlap with the trailer area!");
          } /* check next error cause */

          if (SFU_SUCCESS == e_ret_status)
          {
            TRACE("\r\n= [FWIMG] The binary image to be installed is already tagged as VALID!");
          } /* check next error cause */

          if (ret)
          {
            TRACE("\r\n= [FWIMG] The headers in slot #1 and swap area do not match!");
          } /* no more error cause to check */
        } /* else do not print the message(s) again */
#endif /* SFU_VERBOSE_DEBUG_MODE */
      }
      else
      {
        e_ret_status = SFU_SUCCESS;
      }
    }
  }
  return e_ret_status;
}


/**
  * @brief Prepares the Candidate FW image for Installation.
  *        This stage depends on the supported features: in this example this consists in decrypting the candidate image.
  * @note This function relies on following FWIMG module ram variables, already filled by the checks:
  *       fw_header_to_test,fw_image_header_validated, fw_image_Header_to_test
  * @note Even if the Firmware Image is in clear format the decrypt function is called.
  *       But, in this case no decrypt is performed, it is only a set of copy operations
  *       to organize the Firmware Image in FLASH as expected by the swap procedure.
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_PrepareCandidateImageForInstall(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status;

  /*
   * Pre-condition: all checks have been performed,
   *                so all FWIMG module variables are populated.
   * Now we can decrypt in FLASH.
   *
   * Initial FLASH state (focus on slot #1 and swap area):
   *
   * <Slot #1>   : {Candidate Image Header + encrypted FW - page 0}
   *               {encrypted FW - page 1}
   *               .....
   *               {encrypted FW - page N}
   *               .....
   * </Slot #1>
   *
   * <Swap area> : {Candidate Image Header}
   * </Swap area>
   */
  e_ret_status =  DecryptImageInSlot1(&fw_image_header_to_test);

  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_DECRYPT_FAILURE);
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] Decryption failure!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    return e_ret_status;
  }

  /*
   * At this step, the FLASH content looks like this:
   *
   * <Slot #1>   : {decrypted FW - data from page 1 in page 0}
   *               {decrypted FW - data from page 2 in page 1}
   *               .....
   *               {decrypted FW - data from page N in page N-1}
   *               {page N is now empty}
   *               .....
   * </Slot #1>
   *
   * <Swap area> : {No Header (shift) + decrypted FW data from page 0}
   * </Swap area>
   *
   * The Candidate FW image has been decrypted
   * and a "hole" has been created in slot #1 to be able to swap.
   */

  e_ret_status = VerifyFwSignatureAfterDecrypt(&e_se_status, &fw_image_header_to_test);
  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_SIGNATURE_FAILURE);
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] The decrypted image is incorrect!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    return e_ret_status;
  }

  /* Return the result of this preparation */
  return (e_ret_status);
}


/**
  * @brief  Install the new version
  * relies on following global in ram, already filled by the check
  * fw_header_to_test,fw_image_header_validated, fw_image_Header_to_test
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_InstallNewVersion(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /*
   * At this step, the FLASH content looks like this:
   *
   * <Slot #1>   : {decrypted FW - data from page 1 in page 0}
   *               {decrypted FW - data from page 2 in page 1}
   *               .....
   *               {decrypted FW - data from page N in page N-1}
   *               {page N is now empty}
   *               .....
   * </Slot #1>
   *
   * <Swap area> : {No Header (shift) + decrypted FW data from page 0}
   * </Swap area>
   *
   */

  /*  erase last block */
  e_ret_status = EraseSlotIndex(1, (TRAILER_INDEX - 1)); /* erase the size of "swap area" at the end of slot #1 */
  if (e_ret_status ==  SFU_ERROR)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FLASH_ERROR);
    return SFU_ERROR;
  }

  /*
   * At this step, the FLASH content looks like this:
   *
   * <Slot #1>   : {decrypted FW - data from page 1 in page 0}
   *               {decrypted FW - data from page 2 in page 1}
   *               .....
   *               {decrypted FW - data from page N in page N-1}
   *               {page N is now empty}
   *               .....
   *               {at least the swap area size at the end of the last page of slot #1 is empty }
   * </Slot #1>
   *
   * <Swap area> : {No Header (shift) + decrypted FW data from page 0}
   * </Swap area>
   *
   */


  /*  write trailer  */
  e_ret_status = WriteTrailerHeader(fw_header_to_test, fw_header_validated);
  /*  finish with the magic to validate */
  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
    return e_ret_status;
  }
  e_ret_status = WRITE_TRAILER_MAGIC;
  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
    return e_ret_status;
  }

  /*
   * At this step, the FLASH content looks like this:
   *
   * <Slot #1>   : {decrypted FW - data from page 1 in page 0}
   *               {decrypted FW - data from page 2 in page 1}
   *               .....
   *               {decrypted FW - data from page N in page N-1}
   *               {page N is now empty}
   *               .....
   *               {the last page of slot #1 ends with the trailer magic patterns and the free space for the trailer info}
   * </Slot #1>
   *
   * <Swap area> : {No Header (shift) + decrypted FW data from page 0}
   * </Swap area>
   *
   * The trailer magic patterns is ActiveFwHeader|CandidateFwHeader|SWAPmagic.
   * This is followed by a free space to store the trailer info (N*CPY):
   *
   *  <--------------------------------------------------- Last page of slot #1 ------------------------------------------------->
   *  |                    | FW_INFO_TOT_LEN bytes | FW_INFO_TOT_LEN bytes |MAGIC_LENGTH bytes  | N*sizeof(SFU_LL_FLASH_write_t) |
   *  |                    | header 1              | header 2              |SWAP magic          | N*CPY                          |
   *
   */


  /*  swap  */
  e_ret_status = SwapFirmwareImages();

  if (e_ret_status == SFU_ERROR)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_SWAP);
    return e_ret_status;
  }

  /* validate immediately the new active FW */
  e_ret_status = SFU_IMG_Validation(TRAILER_HDR_TEST); /* the header of the newly installed FW is in the trailer */

  if (SFU_SUCCESS == e_ret_status)
  {
    /*
     * We have just installed a new Firmware so we need to reset the counters
     */

    /*
     * Consecutive init counter:
     *   a reset of this counter is useless as long as the installation is triggered by a Software Reset
     *   and this SW Reset is considered as a valid boot-up cause (because the counter has already been reset in SFU_BOOT_ManageResetSources()).
     *   If you change this beahivor in SFU_BOOT_ManageResetSources() then you need to reset the consecutive init counter here.
     */

  }
  else
  {
    /*
     * Else do nothing (except logging the error cause), we will end up in local download at the next reboot.
     * By not cleaning the swap pattern we may retry this installation but we do not want to do this as we might end up in an infinite loop.
     */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
  }

  /*  clear swap pattern */
  e_ret_status = CLEAN_TRAILER_MAGIC;

  if (SFU_ERROR == e_ret_status)
  {
    /* Memorize the error */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
  }

  return e_ret_status;   /* Leave the function now */
}


/**
  * @brief  Checks if there is a valid FW in slot #1
  * At the moment this check relies only on the signature and the concept of acceptable version number.
  * Modifies the FWIMG module variables: fw_image_header_to_test, fw_header_to_test
  * @param MinFwVersion minimum acceptable FW version
  * @param MaxFwVersion maximum acceptable FW version
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_ValidFwInSlot1(uint16_t MinFwVersion, uint16_t MaxFwVersion)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  SE_StatusTypeDef e_se_status;

  /*  check the header in slot #1 if any */
  e_ret_status = SFU_IMG_GetFWInfoMAC(&fw_image_header_to_test, 1);
  if (e_ret_status == SFU_SUCCESS)
  {
    /* a header has been retrieved so now we can check the FW signature */
    e_ret_status = SFU_IMG_VerifyFwSignature(&e_se_status, &fw_image_header_to_test, 1);
  } /* else there can't be any valid FW in slot #1 */
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = SFU_LL_FLASH_Read(fw_header_to_test, (uint8_t *) SFU_IMG_SLOT_1_REGION_BEGIN,
                                     sizeof(fw_header_to_test));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  } /* else there can't be any valid FW in slot #1 */
  if (e_ret_status == SFU_SUCCESS)
  {
    /* Let's check that the version of this FW present in slot #1 is in an acceptable range of versions */
    if ((fw_image_header_to_test.FwVersion < MinFwVersion) ||
        (fw_image_header_to_test.FwVersion > MaxFwVersion))
    {
      /* not a version we can authorize */
      e_ret_status = SFU_ERROR;
    }
    /* else: let's confirm there is a valid FW in slot #1 (nothing to do as e_ret_status = SFU_SUCCESS) */
  }
  return e_ret_status;
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

#undef SFU_FWIMG_CORE_C

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
