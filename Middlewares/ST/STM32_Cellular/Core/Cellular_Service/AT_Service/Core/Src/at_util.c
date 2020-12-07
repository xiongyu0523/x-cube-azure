/**
  ******************************************************************************
  * @file    at_util.c
  * @author  MCD Application Team
  * @brief   This file provides code for atcore utilities
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
#include <string.h>
#include "at_util.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define MAX_32BITS_STRING_SIZE (8U)  /* = max string size for a 32bits value (FFFF.FFFF) */
#define MAX_64BITS_STRING_SIZE (16U) /* = max string size for a 64bits value (FFFF.FFFF.FFFF.FFFF) */
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
uint32_t ATutil_ipow(uint32_t base, uint16_t exp)
{
  uint16_t local_exp = exp;
  uint32_t local_base = base;

  /* implementation of power function */
  uint32_t result = 1U;
  while (local_exp != 0U)
  {
    if ((local_exp & 1U) != 0U)
    {
      result *= local_base;
    }
    local_exp >>= 1;
    local_base *= local_base;
  }

  return result;
}

uint32_t ATutil_convertStringToInt(const uint8_t *p_string, uint16_t size)
{
  uint32_t conv_nbr = 0U;

  /* auto-detect if this is an hexa value (format: 0x....) */
  if ((size > 2U) && (p_string[1] == 120U)) /* ASCII value 120 = 'x' */
  {
    conv_nbr = ATutil_convertHexaStringToInt32(p_string, size);
  }
  else
  {
    uint16_t idx, nb_digit_ignored = 0U, loop = 0U;

    /* decimal value */
    for (idx = 0U; idx < size; idx++)
    {
      /* consider only the numbers */
      if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
      {
        loop++;
        conv_nbr = conv_nbr +
                   (((uint32_t) p_string[idx] - 48U) * ATutil_ipow(10U, (size - loop - nb_digit_ignored)));
      }
      else
      {
        nb_digit_ignored++;
      }
    }
  }

  return (conv_nbr);
}

uint32_t ATutil_convertHexaStringToInt32(const uint8_t *p_string, uint16_t size)
{
  uint16_t idx, nb_digit_ignored, loop = 0U;
  uint32_t conv_nbr = 0U;
  uint16_t str_size_to_convert;

  /* This function assumes that the string value is an hexadecimal value with or without Ox prefix
   * It converts a string to its hexadecimal value (32 bits value)
   * example:
   * explicit input string format from "0xW" to "0xWWWWXXXX"
   * implicit input string format from "W" to "WWWWXXXX"
   * where X,Y,W and Z are characters from '0' to 'F'
   */

  /* auto-detect if 0x is present */
  if ((size > 2U) && (p_string[1] == 120U)) /* ASCII value 120 = 'x' */
  {
    /* 0x is present */
    nb_digit_ignored = 2U;
  }
  else
  {
    /* 0x is not present */
    nb_digit_ignored = 0U;
  }

  /* if 0x is present, we can skip it */
  str_size_to_convert = size - nb_digit_ignored;

  /* check maximum string size */
  if (str_size_to_convert > MAX_32BITS_STRING_SIZE)
  {
    /* conversion error */
    return (0U);
  }

  /* convert string to hexa value */
  for (idx = nb_digit_ignored; idx < size; idx++)
  {
    if ((p_string[idx] >= 48U) && (p_string[idx] <= 57U))
    {
      /* 0 to 9 */
      loop++;
      conv_nbr = conv_nbr + (((uint32_t)p_string[idx] - 48U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
    }
    else if ((p_string[idx] >= 97U) && (p_string[idx] <= 102U))
    {
      /* a to f */
      loop++;
      conv_nbr = conv_nbr + (((uint32_t)p_string[idx] - 97U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
    }
    else if ((p_string[idx] >= 65U) && (p_string[idx] <= 70U))
    {
      /* A to F */
      loop++;
      conv_nbr = conv_nbr + (((uint32_t)p_string[idx] - 65U + 10U) * ATutil_ipow(16U, (size - loop - nb_digit_ignored)));
    }
    else
    {
      nb_digit_ignored++;
    }
  }

  return (conv_nbr);
}

uint8_t ATutil_convertHexaStringToInt64(const uint8_t *p_string, uint16_t size, uint32_t *high_part_value,
                                        uint32_t *low_part_value)
{
  uint16_t nb_digit_ignored;
  uint16_t str_size_to_convert, high_part_size, low_part_size;

  /* This function assumes that the string value is an hexadecimal value with or without Ox prefix
   * It converts a string to its hexadecimal value (64 bits made of two 32 bits values)
   * example:
   * explicit input string format from "0xW" to "0xWWWWXXXXYYYYZZZZ"
   * implicit input string format from "W" to "WWWWXXXXYYYYZZZZ"
   * where X,Y,W and Z are characters from '0' to 'F'
   */

  /* init decoded values */
  *high_part_value = 0U;
  *low_part_value = 0U;

  /* auto-detect if 0x is present */
  if ((size > 2U) && (p_string[1] == 120U)) /* ASCII value 120 = 'x' */
  {
    /* 0x is present */
    nb_digit_ignored = 2U;
  }
  else
  {
    /* 0x is not present */
    nb_digit_ignored = 0U;
  }

  /* if 0x is present, we can skip it */
  str_size_to_convert = size - nb_digit_ignored;

  /* check maximum string size */
  if (str_size_to_convert > MAX_64BITS_STRING_SIZE)
  {
    /* conversion error */
    return (0U);
  }

  if (str_size_to_convert > 8U)
  {
    high_part_size = str_size_to_convert - 8U;
    /* convert upper part if exists */
    *high_part_value = ATutil_convertHexaStringToInt32((const uint8_t *)(p_string + nb_digit_ignored), high_part_size);
  }
  else
  {
    high_part_size = 0U;
  }

  /* convert lower part */
  low_part_size = str_size_to_convert - high_part_size;
  *low_part_value =  ATutil_convertHexaStringToInt32((const uint8_t *)(p_string + nb_digit_ignored + high_part_size), low_part_size);

  /* string successfully converted */
  return (1U);
}

uint8_t ATutil_isNegative(const uint8_t *p_string, uint16_t size)
{
  /* returns 1 if number in p_string is negative */
  uint8_t isneg = 0U;
  uint16_t idx;

  /* search for "-" until to find a valid number */
  for (idx = 0U; idx < size; idx++)
  {
    /* search for "-" */
    if (p_string[idx] == 45U)
    {
      isneg = 1U;
    }
    /* check for leave-loop condition (negative or valid number found) */
    if ((isneg == 1U) ||
        ((p_string[idx] >= 48U) && (p_string[idx] <= 57U)))
    {
      break;
    }
  }
  return (isneg);
}

void ATutil_convertStringToUpperCase(uint8_t *p_string, uint16_t size)
{
  uint16_t idx = 0U;
  while ((p_string[idx] != 0U) && (idx < size))
  {
    /* if lower case character... */
    if ((p_string[idx] >= 97U) && (p_string[idx] <= 122U))
    {
      /* ...convert it to uppercase character */
      p_string[idx] -= 32U;
    }
    idx++;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
