/**
  ******************************************************************************
  * @file    cellular_runtime.c
  * @author  MCD Application Team
  * @brief   implementation of cellular_runtime functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
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
#include "stdbool.h"
#include "cellular_runtime_custom.h"
#include "cellular_runtime_standard.h"

/* Private defines -----------------------------------------------------------*/
#define CRC_IP_ADDR_DIGIT_SIZE 3U

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* functions ---------------------------------------------------------*/

uint32_t crc_get_ip_addr(uint8_t* string, uint8_t* addr, uint16_t* port)
{
  uint8_t i;
  uint8_t j;
  uint32_t ret;
  uint32_t offset;
  bool leave;

  ret    = 0U;
  offset = 0;

  leave = false;

  for (i=0U ; (i<4U) && (leave == false) ; i++)
  {
    for (j=0U ; j<=CRC_IP_ADDR_DIGIT_SIZE ; j++)
    {
      if(   (string[j+offset] < (uint8_t)'0') || (string[j+offset] > (uint8_t)'9'))
      {
        break;
      }
    }

    if(j == (CRC_IP_ADDR_DIGIT_SIZE+1U))
    {
      ret = 1;
      leave = true;
    }
    else
    {
      addr[i] = (uint8_t)crs_atoi(&string[offset]);
      if(string[offset + j] != (uint8_t)'.')
      {
        leave = true;
      }
      offset = offset + j + 1U;
    }
  }

  if(i != 4U)
  {
    ret = 1;
  }
  else
  {
    if(port != NULL)
    {
      if(string[offset-1U] == (uint8_t)':')
      {
        *port = (uint16_t)crs_atoi(&string[offset]);
      }
      else
      {
        *port = 0;
      }
    }
  }

  if (ret == 1U)
  {
    addr[0] = 0U;
    addr[1] = 0U;
    addr[2] = 0U;
    addr[3] = 0U;
    if(port != NULL)
    {
        *port = 0;
    }
  }
  return ret;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
