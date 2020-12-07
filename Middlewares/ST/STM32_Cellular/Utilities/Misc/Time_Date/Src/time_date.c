/**
  ******************************************************************************
  * @file    time_date.c
  * @author  MCD Application Team
  * @brief   This file contains date and time utilities
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
#include "plf_config.h"
#include "time_date.h"
#include "error_handler.h"
#include "dc_time.h"
#include "menu_utils.h"
#include "cellular_runtime_custom.h"
#include "string.h"



/* Private macros ------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define TIMEDATE_SETUP_LABEL         "Time Date Menu"
#define TIMEDATE_DEFAULT_PARAMA_NB   1
#define TIMEDATE_STRING_SIZE         30U
#define TIMEDATE_DOW_LEN            4U
#define TIMEDATE_MONTH_LEN          3U

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
int32_t timedate_set(uint8_t *dateStr)
{
  dc_time_date_rt_info_t  dc_time_date_rt_info;
  static uint8_t* dow_string[7] = {"Mon,", "Tue,",  "Wed,", "Thu,", "Fri,", "Sat,", "Sun,"};
  static uint8_t* month_string[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  static uint8_t dow_value[7]  =
                 {
                    RTC_WEEKDAY_MONDAY,
                    RTC_WEEKDAY_TUESDAY,
                    RTC_WEEKDAY_WEDNESDAY,
                    RTC_WEEKDAY_THURSDAY,
                    RTC_WEEKDAY_FRIDAY,
                    RTC_WEEKDAY_SATURDAY,
                    RTC_WEEKDAY_SUNDAY
                 };
  uint32_t offset;
  int32_t res;
  uint32_t i;

  dc_time_date_rt_info.wday = RTC_WEEKDAY_SUNDAY;
  dc_time_date_rt_info.month = 1;
  res = 0;
  offset = 0;

  /*======*/
  /* day  */
  /*======*/
  for (i=0U; i<7U; i++)
  {
    if(memcmp((CRC_CHAR_t *)&dateStr[offset], (CRC_CHAR_t *)dow_string[i], TIMEDATE_DOW_LEN) == 0)
    {
      dc_time_date_rt_info.wday = dow_value[i];
      break;
    }
  }

  if (i == 7U)
  {
      res = -1;
      goto end;
  }

  offset += TIMEDATE_DOW_LEN+1U;

  /*======*/
  /* mday */
  /*======*/
  dc_time_date_rt_info.mday = (uint32_t)crs_atoi(&dateStr[offset]);

  for (i=0U ; i<4U ; i++)
  {
    if(dateStr[offset + i] == (uint8_t)' ')
    {
      break;
    }
  }

  if (i == 4U)
  {
      res = -1;
      goto end;
  }

  offset += i+1U;

  /*=======*/
  /* month */
  /*=======*/
  for (i= 0U; i<12U; i++)
  {
    if(memcmp((CRC_CHAR_t *)&dateStr[offset], (CRC_CHAR_t *)month_string[i], TIMEDATE_MONTH_LEN) == 0)
    {
      dc_time_date_rt_info.month = i+1U;
      break;
    }
  }
  if (i == 12U)
  {
      res = -1;
      goto end;
  }

  offset += TIMEDATE_MONTH_LEN+1U;

  /*======*/
  /* year */
  /*======*/
  dc_time_date_rt_info.year = (uint32_t)crs_atoi(&dateStr[offset]);
  for (i=0U; i<6U; i++)
  {
    if(dateStr[offset + i] == (uint8_t)' ')
    {
      break;
    }
  }

  if (i == 6U)
  {
      res = -1;
      goto end;
  }

  offset += i+1U;

  /*======*/
  /* hour */
  /*======*/
  dc_time_date_rt_info.hour = (uint32_t)crs_atoi(&dateStr[offset]);
  for (i=0U; i<4U; i++)
  {
    if(dateStr[offset + i] == (uint8_t)':')
    {
      break;
    }
  }

  if (i == 4U)
  {
      res = -1;
      goto end;
  }

  offset += i+1U;

  /*=========*/
  /* minutes */
  /*=========*/
  dc_time_date_rt_info.min = (uint32_t)crs_atoi(&dateStr[offset]);
  for (i=0U; i<4U; i++)
  {
    if(dateStr[offset + i] == (uint8_t)':')
    {
      break;
    }
  }

  if (i == 4U)
  {
      res = -1;
      goto end;
  }

  offset += i+1U;

  /*=========*/
  /* seconds */
  /*=========*/
  dc_time_date_rt_info.sec = (uint32_t)crs_atoi(&dateStr[offset]);

  (void)dc_srv_set_time_date(&dc_time_date_rt_info, DC_DATE_AND_TIME);

  end:
  return res;
}

int32_t timedate_http_date_set(uint8_t *dateStr)
{

  static uint8_t* date_header  =  "HTTP/1.1 200 OK\r\nDate: ";
  uint32_t header_size;
  int32_t res;


  header_size = crs_strlen(date_header);

  /*========*/
  /* Header */
  /*========*/
  if(memcmp(dateStr, date_header, header_size) != 0)
  {
    res = -1;
  }
  else
  {
    res = timedate_set(&dateStr[header_size]);
  }

  return res;
}


void timedate_setup_handler(void)
{
  static uint8_t timedate_input_sring[TIMEDATE_STRING_SIZE];
  PrintSetup("timedate_setup_handler\n\r")

  menu_utils_get_string((uint8_t *)"Date GMT <Day, dd Month yyyy hh:mm:ss> (ex: Mon, 11 Dec 2017 17:22:05) ",
                        timedate_input_sring, TIMEDATE_STRING_SIZE);
  (void)timedate_set((uint8_t *)timedate_input_sring);

}

void timedate_setup_dump(void)
{
  static uint8_t *timedate_day_of_week_sring[8] =
  {
    (uint8_t *)"",
    (uint8_t *)"Mon",
    (uint8_t *)"Tue",
    (uint8_t *)"Wed",
    (uint8_t *)"Thu",
    (uint8_t *)"Fri",
    (uint8_t *)"Sat",
    (uint8_t *)"Sun",
  };

  dc_time_date_rt_info_t  dc_time_date_rt_info;
  (void)dc_srv_get_time_date(&dc_time_date_rt_info, DC_DATE_AND_TIME);

  PrintSetup("Date: %s %02ld/%02ld/%ld - %02ld:%02ld:%02ld\n\r",
             timedate_day_of_week_sring[dc_time_date_rt_info.wday],
             dc_time_date_rt_info.mday,
             dc_time_date_rt_info.month,
             dc_time_date_rt_info.year,
             dc_time_date_rt_info.hour,
             dc_time_date_rt_info.min,
             dc_time_date_rt_info.sec)

}


int32_t timedate_init(void)
{
  /*   return setup_record(SETUP_APPLI_TIMEDATE, timedate_setup_handler, timedate_setup_dump, timedate_default_setup_table, TIMEDATE_DEFAULT_PARAMA_NB); */
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
