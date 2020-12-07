/**
  ******************************************************************************
  * @file    nifman.c
  * @author  MCD Application Team
  * @brief   This file contains the Network InterFace MANager code
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
#include <stdint.h>
#include "cmsis_os_misrac2012.h"
#include "dc_common.h"
#include "cellular_init.h"
#include "main.h"
#include "nifman.h"
#include "plf_config.h"
#include "error_handler.h"
#include "com_sockets.h"
#include "cellular_runtime_custom.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif_client.h"
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#include "cellular_service_task.h"
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private typedef -----------------------------------------------------------*/
typedef char NIFMAN_CHAR_t; /* used in stdio.h and string.h service call */

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_NIFMAN, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PrintForce(format, args...)   printf("" format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */

/* Private variables ---------------------------------------------------------*/
static osMessageQId nifman_queue;
#if (USE_CMD_CONSOLE == 1)
static uint8_t *nifman_cmd_label = (uint8_t *)"nifman";
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Global variables ----------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void   nifman_notif_cb(dc_com_event_id_t dc_event_id, const void *private_gui_data);
static void nifman_thread(void const *argument);
#if (USE_CMD_CONSOLE == 1)
static cmd_status_t nifman_cmd(uint8_t *cmd_line_p);
static void nifman_cmd_help(void);
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Data cache callback ------------------------------------------------------- */
static void   nifman_notif_cb(dc_com_event_id_t dc_event_id, const void *private_gui_data)
{
  UNUSED(private_gui_data);
  if ((dc_event_id == DC_COM_CELLULAR_DATA_INFO)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      || (dc_event_id == DC_COM_PPP_CLIENT_INFO)
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
     )
  {
    (void)osMessagePut(nifman_queue, (uint32_t)dc_event_id, 0U);
  }
}

#if (USE_CMD_CONSOLE == 1)
static void nifman_cmd_help(void)
{
  CMD_print_help(nifman_cmd_label);
  PrintForce("%s help", nifman_cmd_label);
  PrintForce("%s status", nifman_cmd_label);
}

static cmd_status_t nifman_cmd(uint8_t *cmd_line_p)
{
  static uint8_t *nifman_NetworkName_p[4] =
  {
    (uint8_t *)"NO NETWORK",
    (uint8_t *)"CELLULAR SOCKET MODEM",
    (uint8_t *)"CELLULAR SOCKETS LWIP",
    (uint8_t *)"WIFI",
  };

  uint8_t    *argv_p[10];
  uint32_t    argc;
  uint8_t    *cmd_p;

  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  cmd_p = (uint8_t *)strtok((NIFMAN_CHAR_t *)cmd_line_p, " \t");

  if (strncmp((NIFMAN_CHAR_t *)cmd_p, (NIFMAN_CHAR_t *)nifman_cmd_label, strlen((NIFMAN_CHAR_t *)cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < 10U ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if (argc == 0U)
    {
      nifman_cmd_help();
    }
    /*  1st parameter analysis                     */
    else if (strncmp((NIFMAN_CHAR_t *)argv_p[0], "help", strlen((NIFMAN_CHAR_t *)argv_p[0])) == 0)
    {
      nifman_cmd_help();
    }
    else if (strncmp((NIFMAN_CHAR_t *)argv_p[0], "status", strlen((NIFMAN_CHAR_t *)argv_p[0])) == 0)
    {
      dc_nifman_info_t  dc_nifman_info;
      (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&dc_nifman_info, sizeof(dc_nifman_info_t));
      /* cellular service status */
      PrintForce("Nifman Status");
      if (dc_nifman_info.rt_state == DC_SERVICE_ON)
      {
        PrintForce("State     : On");
        PrintForce("Network   : %s", nifman_NetworkName_p[dc_nifman_info.network]);
        PrintForce("IP ADDR   : %d.%d.%d.%d",
                   COM_IP4_ADDR1(&dc_nifman_info.ip_addr),
                   COM_IP4_ADDR2(&dc_nifman_info.ip_addr),
                   COM_IP4_ADDR3(&dc_nifman_info.ip_addr),
                   COM_IP4_ADDR4(&dc_nifman_info.ip_addr));
      }
      else
      {
        PrintForce("State     : Off");
      }
    }
    else
    {
      cmd_status = CMD_SYNTAX_ERROR;
      PrintForce("%s bad command %s: Usage\n\r", cmd_p, argv_p[0]);
      nifman_cmd_help();
    }
  }
  return cmd_status;
}
#endif  /* (USE_CMD_CONSOLE == 1) */

/* nifman_thread function */
static void nifman_thread(void const *argument)
{
  osEvent event;
  dc_com_event_id_t          dc_event_id;
  dc_cellular_data_info_t    cellular_data_info;
  uint32_t err;
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  dc_ppp_client_info_t       ppp_client_info;
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  dc_nifman_info_t           nifman_info;
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  uint8_t     addr[4];
  CS_IPaddrType_t ip_addr_type;
  static uint8_t nifman_ip_addr[DC_MAX_IP_ADDR_SIZE];
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

  /* Modem activated : start ppp client config */
  for (;;)
  {
    event = osMessageGet(nifman_queue, RTOS_WAIT_FOREVER);
    dc_event_id = (dc_com_event_id_t)event.value.v;

    if (dc_event_id == DC_COM_CELLULAR_DATA_INFO)
    {
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
      switch (cellular_data_info.rt_state)
      {
        case DC_SERVICE_ON:
        {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
          /* MODEM CELLULAR DATA Ready : PPPOS to configure */
          err = ppposif_client_config();
          if (err != 0U)
          {
            ERROR_Handler(DBG_CHAN_NIFMAN, 2, ERROR_FATAL);
            cellular_data_info.rt_state   =  DC_SERVICE_FAIL;
            (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
          }
#else
          /* MODEM CELLULAR DATA operational */
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          (void)CST_get_dev_IP_address(&ip_addr_type, nifman_ip_addr);
          err = crc_get_ip_addr(&nifman_ip_addr[1], addr, NULL);
          if (err == 0)
          {
            nifman_info.ip_addr.addr = addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24);
          }
          else
          {
            nifman_info.ip_addr.addr = 0;
          }

          nifman_info.network      =  DC_CELLULAR_SOCKET_MODEM;
          nifman_info.rt_state     =  cellular_data_info.rt_state;
          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
          break;
        }
        case DC_SERVICE_SHUTTING_DOWN:
        {
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_SHUTTING_DOWN;
          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        case DC_SERVICE_FAIL:
        {
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_FAIL;
          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        case DC_SERVICE_OFF:
        {
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_OFF;
          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        default:
        {
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_OFF;
          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
      }
    }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
    else if (dc_event_id == (dc_com_event_id_t)DC_COM_PPP_CLIENT_INFO)
    {
      (void)dc_com_read(&dc_com_db, DC_COM_PPP_CLIENT_INFO, (void *)&ppp_client_info, sizeof(ppp_client_info));
      switch (ppp_client_info.rt_state)
      {
        case DC_SERVICE_ON:
        {
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_ON;
          nifman_info.network    =  DC_CELLULAR_SOCKETS_LWIP;
          nifman_info.ip_addr    =  ppp_client_info.ip_addr;

          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          break;
        }
        default:
        {
          /* inform application to reset automaton */
          (void)dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));
          nifman_info.rt_state   =  DC_SERVICE_OFF;
          (void)dc_com_write(&dc_com_db, DC_COM_NIFMAN_INFO, (void *)&nifman_info, sizeof(nifman_info));

          /* inform cellular service task to reset automaton */
          (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));
          cellular_data_info.rt_state   =  DC_SERVICE_FAIL;
          (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cellular_data_info, sizeof(cellular_data_info));

          break;
        }
      }
    }
#endif   /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
    else
    {
      /* Nothing to do */
    }
  }
}

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  component init
  * @param  none
  * @retval nifman_status_t    return status
  */
nifman_status_t nifman_init(void)
{
  nifman_status_t ret;
  osMessageQDef(nifman_queue, 1, uint32_t);
  nifman_queue = osMessageCreate(osMessageQ(nifman_queue), NULL);
  if (nifman_queue == NULL)
  {
    ret = NIFMAN_ERROR;
    ERROR_Handler(DBG_CHAN_NIFMAN, 2, ERROR_FATAL);
  }
  else
  {
    ret = NIFMAN_OK;
  }

  return ret;
}

/**
  * @brief  component start
  * @param  none
  * @retval nifman_status_t    return status
  */
nifman_status_t nifman_start(void)
{
  nifman_status_t ret;
  static osThreadId  nifman_ThreadId = NULL;

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare(nifman_cmd_label, nifman_cmd, (uint8_t *)"network interface manager");
#endif  /*  (USE_CMD_CONSOLE == 1) */

  (void)dc_com_register_gen_event_cb(&dc_com_db, nifman_notif_cb, (void *) NULL);

  osThreadDef(NIFMAN, nifman_thread, NIFMAN_THREAD_PRIO, 0, NIFMAN_THREAD_STACK_SIZE);
  nifman_ThreadId = osThreadCreate(osThread(NIFMAN), NULL);
  if (nifman_ThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_NIFMAN, 2, ERROR_FATAL);
    ret =  NIFMAN_ERROR;
  }
  else
  {
#if (STACK_ANALYSIS_TRACE == 1)
    stackAnalysis_addStackSizeByHandle(nifman_ThreadId, NIFMAN_THREAD_STACK_SIZE);
#endif /* STACK_ANALYSIS_TRACE */
    ret = NIFMAN_OK;
  }

  return ret;
}

/***************************** (C) COPYRIGHT STMicroelectronics *******END OF FILE ************/
