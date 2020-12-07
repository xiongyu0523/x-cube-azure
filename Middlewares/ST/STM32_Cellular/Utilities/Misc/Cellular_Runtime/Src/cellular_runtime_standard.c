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
#include <stdbool.h>
#include <math.h>
#include "plf_config.h"
#include "cellular_runtime_standard.h"
#include "string.h"

/* Private defines -----------------------------------------------------------*/
#define CRS_STRLEN_MAX 2048U

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* functions ---------------------------------------------------------*/


uint8_t* crs_itoa(int32_t num, uint8_t* str, uint32_t base)
{
  uint32_t i;
  uint32_t j;
  bool isNegative;
  int32_t rem;
  int32_t char32;
  uint8_t temp;
  int32_t num_tmp;

  num_tmp = num;
  isNegative = false;
  i = 0U;

  /* Handle 0 explicitely, otherwise empty string is printed for 0 */
  if (num_tmp == 0)
  {
      str[i] = (uint8_t)'0';
      i++;
      str[i] = (uint8_t)'\0';
  }
  else
  {
    if ((num_tmp < 0) && (base == 10U))
    {
      isNegative = true;
      num_tmp = -num_tmp;
    }

    while (num_tmp != 0)
    {
      rem = num_tmp % (int32_t)base;
      if((rem > 9))
      {
        char32 = ((rem-10) + (int32_t)'a');
        str[i] = (uint8_t)char32;
      }
      else
      {
        char32 = (rem + (int32_t)'0');
        str[i] = (uint8_t)char32;
      }
      num_tmp = num_tmp/(int32_t)base;
      i++;
    }

    if (isNegative == true)
    {
      str[i] = (uint8_t)'-';
      i++;
    }

    str[i] = (uint8_t)'\0';

    for(j = 0U; j<(i>>1) ; j++)
    {
      temp     = str[j];
      str[j]   = str[i-j-1U];
      str[i-j-1U] = temp;
    }
  }
  return str;
}

int32_t crs_atoi(const uint8_t* string)
{
  int32_t result;
  int32_t digit;

  uint32_t offset;
  int8_t sign;
  uint8_t digit8;

  result = 0;
  offset = 0;

  if (*string == (uint8_t)'-')
  {
    sign = 1;
    offset++;
  }
  else
  {
    sign = 0;
    if (string[offset] == (uint8_t)'+')
    {
      offset++;
    }
  }

  while (true)
  {
    digit8 = string[offset] - (uint8_t)'0';
    digit  = (int32_t)digit8;
    if (digit > 9)
    {
      break;
    }
    result = (10*result) + digit;
    offset++;
  }

  if (sign != 0)
  {
    result = -result;
  }
  return result;
}

int32_t crs_atoi_hex(uint8_t* string)
{
  int32_t result;
  uint32_t digit;
  uint8_t digit8;
  uint32_t offset;

  result = 0;
  offset = 0;

  while (true)
  {
    if ((string[offset] >= (uint8_t)'0') && (string[offset] <= (uint8_t)'9') )
    {
      digit8 = string[offset] - (uint8_t)'0';
      digit  = (uint32_t)digit8;
    }
    else if((string[offset] >= (uint8_t)'a') && (string[offset] <= (uint8_t)'f') )
    {
      digit8 = string[offset] - (uint8_t)'a' + 10U;
      digit  = (uint32_t)digit8;
    }
    else if((string[offset] >= (uint8_t)'A') && (string[offset] <= (uint8_t)'F') )
    {
      digit8 =  string[offset] - (uint8_t)'A' + 10U;
      digit  = (uint32_t)digit8;
    }
    else
    {
      break;
    }

    result = (16*result) + (int32_t)digit;
    offset++;
  }

  return result;
}


uint32_t crs_strlen(const uint8_t* string)
{
  uint32_t i;
  for(i=0U ; i<CRS_STRLEN_MAX ; i++)
  {
    if(string[i] == 0U)
    {
      break;
    }
  }
  if (i>=CRS_STRLEN_MAX)
  {
    i = 0U;
  }
  return i;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
