/**
  ******************************************************************************
  * @file    rfu.c
  * @author  MCD Application Team
  * @brief   Remote firmware update over TCP/IP.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
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
#if defined(GENERIC_OTA) || defined(CLD_OTA)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef OTA_DOWNLOAD_ONLY
#include "se_def.h"
#include "sfu_fwimg_regions.h"
#else /* Allows to test the download on platforms which do not support the in-app FW update. */
#define FLASH_PAGE_SIZE 0x800
#define SE_FW_HEADER_TOT_LEN sizeof(uint32_t)
#endif

#include "http_lib.h"
#include "msg.h"
#include "flash.h"
#include "iot_flash_config.h"
#include "rfu.h"

/**
  * @brief Specifies a structure containing values related to the management of multi-images in Flash.
  */
typedef struct
{
  uint32_t  MaxSizeInBytes;        /*!< The maximum allowed size for the FwImage in User Flash (in Bytes) */
  uint32_t  DownloadAddr;          /*!< The download address for the FwImage in UserFlash */
  uint32_t  ImageOffsetInBytes;    /*!< Image write starts at this offset */
  uint32_t  ExecutionAddr;         /*!< The execution address for the FwImage in UserFlash */
} SFU_FwImageFlashTypeDef;

uint32_t SFU_APP_InstallAtNextReset(uint32_t *fw_header);
uint32_t SFU_APP_GetDownloadAreaInfo(SFU_FwImageFlashTypeDef *pArea);

/* Private defines -----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if defined(GENERIC_OTA) || defined(CLD_OTA)
static int FW_UPDATE_DownloadNewFirmware(SFU_FwImageFlashTypeDef *fw_image_dwl_area,const char * const url, const char * ca_certs);
#endif /* GENERIC_OTA || CLD_OTA */
static uint32_t WriteInstallHeader(uint32_t *pfw_header);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  Write the header of the firmware to install
  * @param  pfw_header pointer to header to write.
  * @retval HAL_OK on success otherwise HAL_ERROR
  */
static uint32_t WriteInstallHeader(uint32_t *pfw_header)
{
  uint32_t ret = HAL_OK;
#ifndef OTA_DOWNLOAD_ONLY
  uint8_t zero_buffer[SFU_IMG_IMAGE_OFFSET - SE_FW_HEADER_TOT_LEN];

  memset(zero_buffer, 0x00, SFU_IMG_IMAGE_OFFSET - SE_FW_HEADER_TOT_LEN);
  ret = FLASH_Erase_Size((uint32_t) SFU_IMG_SWAP_REGION_BEGIN_VALUE, SFU_IMG_IMAGE_OFFSET);
  if (ret == HAL_OK)
  {
    ret = FLASH_Write((uint32_t)SFU_IMG_SWAP_REGION_BEGIN_VALUE, pfw_header, SE_FW_HEADER_TOT_LEN);
  }
  if (ret == HAL_OK)
  {
    ret = FLASH_Write((uint32_t )(SFU_IMG_SWAP_REGION_BEGIN_VALUE + SE_FW_HEADER_TOT_LEN), (void *)zero_buffer, SFU_IMG_IMAGE_OFFSET - SE_FW_HEADER_TOT_LEN);
  }
#endif
  return ret;
}


/**
  * @brief  Write in Flash the next header image to install.
  *         This function is used by the User Application to request a Firmware installation (at next reboot).
  * @param  fw_header FW header of the FW to be installed
  * @retval HAL_OK if successful, otherwise HAL_ERROR
  */
uint32_t SFU_APP_InstallAtNextReset(uint32_t *fw_header)
{
  if (fw_header == NULL)
  {
    return HAL_ERROR;
  }
  if (WriteInstallHeader(fw_header) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}


/**
  * @brief  Provide the area descriptor to write a FW image in Flash.
  *         This function is used by the User Application to know where to store a new Firmware Image before asking for its installation.
  * @param  pArea pointer to area descriptor
  * @retval HAL_OK if successful, otherwise HAL_ERROR
  */
uint32_t SFU_APP_GetDownloadAreaInfo(SFU_FwImageFlashTypeDef *pArea)
{
  uint32_t ret = HAL_OK;
#ifndef OTA_DOWNLOAD_ONLY
  if (pArea != NULL)
  {
    pArea->DownloadAddr = SFU_IMG_SLOT_DWL_REGION_BEGIN_VALUE;
    pArea->MaxSizeInBytes = (uint32_t)SFU_IMG_SLOT_DWL_REGION_SIZE;
    pArea->ImageOffsetInBytes = SFU_IMG_IMAGE_OFFSET;
  }
  else
  {
    ret = HAL_ERROR;
  }
#endif
  return ret;
}
#endif /* GENERIC_OTA || CLD_OTA */

#if defined(GENERIC_OTA) || defined(CLD_OTA)
#define ALIGN64bit(a)      (((a + 7) / 8) * 8)
#define MAX_ERROR_RETRY_NB 3
#define MAX_DUPLICATE_RANGE_NB 3
#define FW_PAGE_SIZE    1024
#define HEADERS_SIZE    1024
#define OTA_READ_TIMEOUT  30000

/**
 * @brief   Download a firmware image from an HTTP server into Flash memory.
 * @note    The HTTP server must support the "Range:" request header. This is the case with HTTP/1.1.
 * @param   In: fw_image_dwl_area  Memory area for the new firmware
 * @param   In: url                Network location of the new firmware (HTTP URL: "http://<hostname>:<port>/<path>")
 * @param   In: ca_certs           Root CA certificates (required for server authentication if a TLS session is needed)
 * @retval  Error code
 *             RFU_OK (0) Success.
 *             <0         Failure.
 *                          RFU_ERR_HTTP  Error downloading over HTTP.
 *                          RFU_ERR_FLASH Error erasing or programming the Flash memory.
 */
static int FW_UPDATE_DownloadNewFirmware(SFU_FwImageFlashTypeDef *fw_image_dwl_area, const char * const url, const char * ca_certs)
{
  char host[64]; /* to be fine-tuned according to your host */
  int  port = 80;
  bool is_tls = false;
  char query[256]; /* to be fine-tuned according to your path */
  uint32_t *http_response_buffer = (uint32_t*) ALIGN64bit((uint32_t) malloc(FW_PAGE_SIZE + HEADERS_SIZE + 8));
  uint32_t areaAddr = fw_image_dwl_area->DownloadAddr;
  int  net_rc = 0;
  int  flash_rc = 0;
  int  rfu_rc = RFU_OK;
  char range_header[256];
  uint32_t full_fw_size = FW_PAGE_SIZE;
  uint32_t range_start = 0;
  uint32_t range_end = 0;
  uint32_t cumulated_received_size = 0;
  uint32_t range_size = FW_PAGE_SIZE;
  uint32_t body_size = 0;
  uint32_t response_size = FW_PAGE_SIZE + HEADERS_SIZE;
  uint8_t * pBody = NULL;
  uint32_t remaining = 0;
  http_handle_t httpHnd = 0;
  int error_retry_nb = 0;
  unsigned int duplicate_range_retry_nb = 0;


  net_rc = http_url_parse(host, sizeof(host), &port, &is_tls, query, sizeof(query), url);
  if (net_rc != HTTP_OK)
  {
    msg_error("Could not parse URL %s\n", url);
    rfu_rc = RFU_ERR_HTTP;
    goto error;
  }

  printf("Erasing Flash memory.\n");
  msg_debug("Calling FLASH_Erase_Size() with fw_image_dwl_area->DownloadAddr=%x fw_image_dwl_area->MaxSizeInBytes=%d\n",
            fw_image_dwl_area->DownloadAddr, fw_image_dwl_area->MaxSizeInBytes);
  if (FLASH_Erase_Size((uint32_t)(fw_image_dwl_area->DownloadAddr), fw_image_dwl_area->MaxSizeInBytes) != HAL_OK)
  {
      msg_error("Could not erase flash memory\n");
      rfu_rc = RFU_ERR_FLASH;
      goto error;
  }

  printf("Downloading firmware from %s.\n", url);
  do {
    if (http_is_open(httpHnd) != true)
    {
      printf("\n");
      msg_debug("Connecting.\n");
      net_rc = http_create_session(&httpHnd, host, port, is_tls?HTTP_PROTO_HTTPS:HTTP_PROTO_HTTP);
      if (net_rc != HTTP_OK)
      {
        msg_error("Could not create session for %s\n", url);
        rfu_rc = RFU_ERR_HTTP;
         goto error;
      }

      uint32_t timeout=OTA_READ_TIMEOUT;

      net_rc = http_sock_setopt(httpHnd, NET_SO_RCVTIMEO, (void *) &timeout, sizeof(uint32_t));
      if (net_rc != HTTP_OK)
      {
        msg_error("Could not set sock read timeout (%d)\n", net_rc);
        rfu_rc = RFU_ERR_HTTP;
        goto error;
      }
      if (is_tls && (NULL != ca_certs))
      {
        net_rc = http_sock_setopt(httpHnd, NET_SO_TLS_CA_CERT, (void *)ca_certs, strlen(ca_certs) + 1);
        msg_debug("http_sock_setopt(\"NET_SO_TLS_CA_CERT\") ret=%d\n", net_rc);

        if (net_rc < 0)
        {
          msg_error("error setting connection option (%d)\n", net_rc);
          rfu_rc = RFU_ERR_HTTP;
          goto error;
        }
      }
      net_rc = http_connect(httpHnd);
      if (net_rc != HTTP_OK)
      {
        msg_error("Could not connect to %s (return code %d)\n", url, net_rc);
         rfu_rc = RFU_ERR_HTTP;
         goto error;
      }
    }

    remaining = full_fw_size - cumulated_received_size;
    remaining = (remaining > range_size)?range_size:remaining;
    sprintf(range_header, "Range: bytes=%lu-%lu\r\n", cumulated_received_size, cumulated_received_size + remaining - 1);
    response_size = FW_PAGE_SIZE+HEADERS_SIZE;
    net_rc = http_get(httpHnd, query,
                  range_header,
                  (uint8_t*)http_response_buffer,
                  &response_size);
    msg_debug("http_get() ret=%d response_size=%d\n", net_rc, response_size);
    if (((net_rc == HTTP_OK)||(net_rc == HTTP_EOF)) && (response_size > 0))
    {
      range_start = 0;
      range_end = 0;
      full_fw_size = 0;
      body_size = response_size;
      pBody = http_find_body((uint8_t*)http_response_buffer, &body_size);
      msg_debug("pBody=%x body_size=%d\n", pBody, body_size);
      msg_debug("HTTP response:\n%.*s\n", pBody - (uint8_t *)http_response_buffer, (char *)http_response_buffer);
      http_content_range((uint8_t*)http_response_buffer, response_size, &range_start, &range_end, &full_fw_size);
      if (range_start != cumulated_received_size)
      {
        msg_debug("range_start(%d) != cumulated_received_size(%d) -> ask again\n",
                  range_start, cumulated_received_size);
        if (duplicate_range_retry_nb < MAX_DUPLICATE_RANGE_NB)
        {
          duplicate_range_retry_nb += 1;
          /* The received range is different than the one requested.
             Something is wrong in the connection. So close it.
             It will be re-opened at beginning of loop.
          */
          http_close(httpHnd);
          continue;
        }
        else
        {
          msg_error("Reached max number of duplicate range. Exiting.\n");
          net_rc = HTTP_ERR;
          break;
        }
      }
      else
      {
        duplicate_range_retry_nb = 0;
      }

      if (body_size > 0)
      {
        /* The start of the firmware chunk can be at any address in the HTTP response
           but we need a word-aligned source buffer to write in Flash memory
           so we shift the firmware chunk to the beggining of buffer which is
           a (8 bytes = L4 flash word) word-aligned buffer: http_response_buffer */
        char *csrc = (char *)pBody;
        char *cdest = (char *)http_response_buffer;
        for (int i = 0; i < body_size; i++)
        {
          cdest[i] = csrc[i];
        }

        flash_rc = FLASH_Write(areaAddr, (uint32_t*)http_response_buffer, ALIGN64bit(body_size));
        msg_debug("FLASH_Write(%x, %x, %d) returns %d\n", areaAddr, http_response_buffer, ALIGN64bit(body_size), flash_rc);
        if (flash_rc)
        {
          printf("ERROR: Unable to write flash area at [%lx - %lx [ - return=%d\n", areaAddr, ALIGN64bit(body_size), flash_rc);
          rfu_rc = RFU_ERR_FLASH;
          goto error;
        }
        printf(".");
        areaAddr += body_size;
        cumulated_received_size += body_size;
      }
    }
    if (net_rc == HTTP_EOF)
    {
      msg_debug("EOF -> Disconnecting.\n");
      http_close(httpHnd);
    }
    else
    {
      if (HTTP_OK != net_rc)
      {
        /* It is not possible to recover at HTTP level. Try Dw again.
           For instance, case of a timeout :
           If you re-send the same request you may receive an answer
           to previous request and answer to new request.
           So close the HTTP connection and re-open it at beginning of loop. */
        if (error_retry_nb < MAX_ERROR_RETRY_NB)
        {
          error_retry_nb++;
          printf("Retrying %d : close and reconnect.\n", error_retry_nb);
          http_close(httpHnd);
          net_rc = HTTP_OK;
        }
      }
    }

  } while (((net_rc == HTTP_OK) || (net_rc == HTTP_EOF)) && (cumulated_received_size < full_fw_size)
           && (cumulated_received_size < fw_image_dwl_area->MaxSizeInBytes));

  printf("\n");
  if ((net_rc == HTTP_OK) || (net_rc == HTTP_EOF))
  {
    printf("Downloaded total size %lu bytes\n", cumulated_received_size);
    rfu_rc = RFU_OK;
  } else {
    printf("network error : %d\n", net_rc);
    rfu_rc = RFU_ERR_HTTP;
  }
error:
  free(http_response_buffer);

  http_close(httpHnd);
  return rfu_rc;
}

int rfu_update(const char * const url, const char * ca_certs)
{
  int ret = RFU_ERR;
  uint32_t  fw_header_input[SE_FW_HEADER_TOT_LEN/4];
  SFU_FwImageFlashTypeDef fw_image_dwl_area;

  /* Get Info about the download area */
  if (SFU_APP_GetDownloadAreaInfo(&fw_image_dwl_area) != HAL_ERROR)
  {
    /* Download new firmware image*/
    ret = FW_UPDATE_DownloadNewFirmware(&fw_image_dwl_area, url, ca_certs);

    if (ret == RFU_OK)
    {
      /* Read header in slot 1 */
      memcpy((void *) fw_header_input, (void *) fw_image_dwl_area.DownloadAddr, sizeof(fw_header_input));

      /* Ask for installation at next reset */
      (void)SFU_APP_InstallAtNextReset(fw_header_input);
    }
  }

  if (ret != RFU_OK)
  {
    printf("  -- !!Operation failed!! \r\n\n");
  }
  return ret;
}

#endif /* GENERIC_OTA || CLD_OTA */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
