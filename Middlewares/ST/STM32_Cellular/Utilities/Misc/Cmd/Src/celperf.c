/**
  ******************************************************************************
  * @file    celperf.c
  * @author  MCD Application Team
  * @brief   cellular throughput
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
#include "celperf.h"
#include "menu_utils.h"
#include "dc_common.h"
#include "dc_cellular.h"
#include "error_handler.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"

#include "cmsis_os_misrac2012.h"
#include "com_sockets.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"

#if (USE_CELPERF == 1)
/* Private defines -----------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_UTILITIES, DBL_LVL_P0, "" format "\n\r", ## args)
#else /* USE_PRINTF == 1 */
#define PrintForce(format, args...)     printf("" format "\n\r", ## args);
#endif /* USE_PRINTF */

#define CELPERF_ITERATION_NB        1U
#define CELPERF_SIZE                128
#define CELPERF_RX_TX_BUF_SIZE      1600
#define CELPERF_LOCAL_PORT          8888U


/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static int32_t       celperf_socket;
static uint8_t      *celperf_cmd_label = ((uint8_t *)"perf");
static uint32_t      celperf_iteration_nb = CELPERF_ITERATION_NB;
static int32_t       celperf_size = CELPERF_SIZE;

static uint8_t celperf_ip_addr[4] = {192U, 168U, 2U, 1U};
static uint16_t celperf_ip_port    = CELPERF_LOCAL_PORT;

static uint8_t celperf_rxbuf[CELPERF_RX_TX_BUF_SIZE];

/* first byte celperf_snd_buf_const buufer is 0 */
static const uint8_t celperf_snd_buf_const[CELPERF_RX_TX_BUF_SIZE] =
{
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};


/* Global variables ----------------------------------------------------------*/
/* cellular signal quality */

/* Private function prototypes -----------------------------------------------*/
/* Callback */
static void celperf_measure_start(void);
static void celperf_all_measure_start_sndrcv(void);
static void celperf_all_measure_start_snd(void);
static void celperf_help(void);

/* Private functions ---------------------------------------------------------*/

static void celperf_help(void)
{
  CMD_print_help(celperf_cmd_label);
  PrintForce("%s help", celperf_cmd_label)
  PrintForce("%s start <snd|rcv>", celperf_cmd_label)
  PrintForce("%s addr [ddd.ddd.ddd.ddd[:port]]", celperf_cmd_label)
  PrintForce("%s port [port]", celperf_cmd_label)
  /*
        PrintForce("%s unit", celperf_cmd_label)
        PrintForce("%s size [size]", celperf_cmd_label)
        PrintForce("%s nb [nb]", celperf_cmd_label)
  */
}

/**
  * @brief  console cmd management
  * @param  cmd_line_p - command line
  * @note   Unused
  * @retval None
  */
static cmd_status_t celperf_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;
  const uint8_t    *cmd_p;
  uint8_t     ip_addr[4];
  uint16_t    port;
  uint32_t    nb;
  uint32_t     ret;

  PrintForce("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (memcmp((const CRC_CHAR_t *)cmd_p, (CRC_CHAR_t *)celperf_cmd_label,
              crs_strlen(cmd_p)) == 0)
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
      celperf_help();
    }
    /*  1st parameter analysis                     */
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
    {
      celperf_help();
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "start", crs_strlen(argv_p[0])) == 0)
    {
      if (memcmp((CRC_CHAR_t *)argv_p[1], "snd", crs_strlen(argv_p[1])) == 0)
      {
        celperf_all_measure_start_snd();
      }
      else if (memcmp((CRC_CHAR_t *)argv_p[1], "rcv", crs_strlen(argv_p[1])) == 0)
      {
        celperf_all_measure_start_sndrcv();
      }
      else
      {
        PrintForce("%s ", celperf_cmd_label)
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "unit", crs_strlen(argv_p[0])) == 0)
    {
      celperf_measure_start();
    }

    else if (memcmp((CRC_CHAR_t *)argv_p[0], "addr", crs_strlen(argv_p[0])) == 0)
    {
      if(argc == 2U)
      {
        ret = crc_get_ip_addr(argv_p[1], ip_addr, &port);

        if (ret == 0U)
        {
          celperf_ip_addr[0] = ip_addr[0];
          celperf_ip_addr[1] = ip_addr[1];
          celperf_ip_addr[2] = ip_addr[2];
          celperf_ip_addr[3] = ip_addr[3];
        }

        if(port != 0U)
        {
          celperf_ip_port = port;
        }
      }

      PrintForce("IP addr : %d.%d.%d.%d:%d", celperf_ip_addr[0], celperf_ip_addr[1], celperf_ip_addr[2],
                 celperf_ip_addr[3], celperf_ip_port)
    }

    else if ( memcmp((CRC_CHAR_t *)argv_p[0], "size", crs_strlen(argv_p[0])) == 0)
    {
      celperf_size = crs_atoi(argv_p[1]);
      if (celperf_size > CELPERF_RX_TX_BUF_SIZE)
      {
        PrintForce("size max : %d", CELPERF_RX_TX_BUF_SIZE)
        celperf_size = (int32_t)CELPERF_RX_TX_BUF_SIZE;
      }

      PrintForce("size : %ld", celperf_size)
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "nb", crs_strlen(argv_p[0])) == 0)
    {
      nb = (uint32_t)crs_atoi(argv_p[1]);
      PrintForce("iteration nb : %ld\n\r", nb)
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "port", crs_strlen(argv_p[0])) == 0)
    {
      if(argc == 2U)
      {
        celperf_ip_port = (uint16_t)crs_atoi(argv_p[1]);
      }
      PrintForce("ip port : %d", celperf_ip_port)
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "param", crs_strlen(argv_p[0])) == 0)
    {
      PrintForce("IP addr       : %ld.%ld.%ld.%ld:%d", celperf_ip_addr[0], celperf_ip_addr[1], celperf_ip_addr[2],
                 celperf_ip_addr[3], celperf_ip_port)
      PrintForce("size          : %ld", celperf_size)
      PrintForce("iteration nb  : %ld", celperf_iteration_nb)
    }
    else
    {
      PrintForce("%s %s: bad command. Usage:", celperf_cmd_label, argv_p[0])
      celperf_help();
    }
  }

  PrintForce("")
  return CMD_OK;
}

/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  private_gui_data - value provided at service subscription
  * @note   Unused
  * @retval None
  */

/**
  * @brief  Create socket
  * @note   If requested close and create socket
  * @param  None
  * @retval None
  */
static uint32_t celperf_create_socket(void)
{
  uint32_t ret;
  ret = 0;
  com_ip_addr_t celperf_distantip;
  com_sockaddr_in_t address;

  /* Create a TCP socket */
  celperf_socket = com_socket(COM_AF_INET, COM_SOCK_STREAM, COM_IPPROTO_TCP);

  if (celperf_socket >= 0)
  {

    COM_IP4_ADDR(&celperf_distantip,
                 ((uint32_t)celperf_ip_addr[0]),
                 ((uint32_t)celperf_ip_addr[1]),
                 ((uint32_t)celperf_ip_addr[2]),
                 ((uint32_t)celperf_ip_addr[3]));

    address.sin_family      = (uint8_t)COM_AF_INET;
    address.sin_port        = com_htons(celperf_ip_port);
    address.sin_addr.s_addr = celperf_distantip.addr;
    if (com_connect(celperf_socket, (com_sockaddr_t const *)&address,
                    (int32_t)sizeof(com_sockaddr_in_t)) != COM_SOCKETS_ERR_OK)
    {
      ret = 1U;
      (void)com_closesocket(celperf_socket);
      PrintForce("socket KO")
    }
    else
    {
      PrintForce("socket connect OK")
    }
  }
  else
  {
    PrintForce("socket create KO")
  }

  return ret;
}


static void celperf_iteration_snd_rcv(uint32_t nb, uint32_t size)
{
  /* first byte celperf_sndrcv_buf_const buffer is 1 */
  static const uint8_t celperf_sndrcv_buf_const[CELPERF_RX_TX_BUF_SIZE] =
  {
    '1', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  };
  uint32_t read_buf_size;
  uint32_t time_started;
  uint32_t time_stopped;
  uint32_t total_time;

  uint32_t i;
  uint32_t count;
  count = 0U;
  time_started = HAL_GetTick();
  for (i = 0U; i < nb ; i++)
  {
    if (com_send(celperf_socket, (const com_char_t *)celperf_sndrcv_buf_const, (int32_t)size, 0) > 0)
    {
      read_buf_size = (uint32_t)com_recv(celperf_socket, (uint8_t *)celperf_rxbuf, (int32_t)size, 0);
      if (read_buf_size == size)
      {
        if (memcmp((const CRC_CHAR_t *)celperf_sndrcv_buf_const, celperf_rxbuf, size) == 0)
        {
          count++;
        }
      }
    }
    else
    {
      PrintForce("CELPERF send NOK")
      break;
    }
  }

  time_stopped = HAL_GetTick();
  total_time = time_stopped - time_started;

  PrintForce("%5ld\t%5ld\t%7ld\t%7ld\t\t%6ld", size, nb,  size * nb * 2U, total_time,
             (size * nb * 2U) / (total_time / 1000U))

}

static void celperf_iteration_snd(uint32_t nb, uint32_t size)
{
  uint32_t time_started;
  uint32_t time_stopped;
  uint32_t total_time;

  uint32_t i;

  time_started = HAL_GetTick();
  for (i = 0U; i < nb ; i++)
  {
    if (com_send(celperf_socket, (const com_char_t *)celperf_snd_buf_const, (int32_t)size, 0) > 0)
    {
    }
    else
    {
      PrintForce("CELPERF send NOK")
      break;
    }
  }

  time_stopped = HAL_GetTick();
  total_time = time_stopped - time_started;

  PrintForce("%5ld\t%5ld\t%7ld\t%7ld\t\t%6ld", size, nb,  size * nb, total_time,
             (size * nb) / (total_time / 1000U))

}

static void celperf_all_measure_start_sndrcv(void)
{
  uint32_t ret;

  ret = celperf_create_socket();

  if (ret == 0U)
  {
    PrintForce("tcpip perf snd/rcv started...\r\n")
    PrintForce(" size\t iter\tdata(B)\ttime(ms)\t throughput(Byte/s)")

    celperf_iteration_snd_rcv(1000,  16);
    celperf_iteration_snd_rcv(1000,  32);
    celperf_iteration_snd_rcv(1000,  64);
    celperf_iteration_snd_rcv(1000, 128);
    celperf_iteration_snd_rcv(200,  256);
    celperf_iteration_snd_rcv(100,  512);
    celperf_iteration_snd_rcv(100, 1024);
    celperf_iteration_snd_rcv(100, 1400);

    if (com_closesocket(celperf_socket) != COM_SOCKETS_ERR_OK)
    {
      PrintForce("socket close NOK - stay in closing state")
    }
    else
    {
      PrintForce("Perf measure completed")
    }
  }
  else
  {
    PrintForce("socket create FAIL")
  }
}

static void celperf_all_measure_start_snd(void)
{
  uint32_t ret;

  ret = celperf_create_socket();

  if (ret == 0U)
  {
    PrintForce("tcpip perf snd started...\r\n")
    PrintForce(" size\t iter\tdata(B)\ttime(ms)\t throughput(Byte/s)")

    celperf_iteration_snd(5000,  16);
    celperf_iteration_snd(3000,  32);
    celperf_iteration_snd(1000,  64);
    celperf_iteration_snd(1000, 128);
    celperf_iteration_snd(500,  256);
    celperf_iteration_snd(200,  512);
    celperf_iteration_snd(100, 1024);
    celperf_iteration_snd(100, 1400);

    if (com_closesocket(celperf_socket) != COM_SOCKETS_ERR_OK)
    {
      PrintForce("socket close NOK - stay in closing state")
    }
    else
    {
      PrintForce("Perf measure completed")
    }

  }
  else
  {
    PrintForce("socket create FAIL")
  }
}


/**
  * @brief  Process a request
  * @note   Create, Send, Receive and Close socket
  * @param  send     - pointer on data buffer to send
  * @note   -
  * @param  response - pointer on data buffer for the response
  * @note   -
  * @retval bool     - false/true : process NOK/OK
  */
static void celperf_measure_start(void)
{
  int32_t read_buf_size;
  uint32_t ret;
  uint32_t time_started;
  uint32_t time_stopped;
  uint32_t total_time;
  uint8_t loop_exit;

  uint32_t i;

  ret = celperf_create_socket();

  if (ret == 0U)
  {
    PrintForce("perf start : iteration %ld - %ld size", celperf_iteration_nb, celperf_size)
    time_started = HAL_GetTick();

    loop_exit = 0U;

    for (i = 0U; (i < celperf_iteration_nb) && (loop_exit == 0U) ; i++)
    {
      if (com_send(celperf_socket, (const com_char_t *)celperf_snd_buf_const, celperf_size, 0) > 0)
      {
        PrintForce("socket send data %ld OK", celperf_size)
        read_buf_size = com_recv(celperf_socket, (uint8_t *)celperf_rxbuf, celperf_size, 0);
        if (read_buf_size == celperf_size)
        {
          if (memcmp((const CRC_CHAR_t *)celperf_snd_buf_const, celperf_rxbuf, (uint32_t)celperf_size) == 0)
          {
            PrintForce("CELPERF receive %ld (%ld/%ld) OK", celperf_size, i + 1U, celperf_iteration_nb)
          }
          else
          {
            PrintForce("CELPERF receive WRONG")
            loop_exit = 1;
          }
        }
        else
        {
          PrintForce("CELPERF receive Bad size %ld", read_buf_size)
          loop_exit = 1;
        }
      }
      else
      {
        PrintForce("CELPERF send NOK");
        loop_exit = 1;
      }
    }

    time_stopped = HAL_GetTick();
    total_time = time_stopped - time_started;
    PrintForce("Total time %ld - data %ld - throughput %ld", total_time, (uint32_t)celperf_size * celperf_iteration_nb,
               ((uint32_t)celperf_size * celperf_iteration_nb) / (total_time / 1000U))

    if (com_closesocket(celperf_socket) != COM_SOCKETS_ERR_OK)
    {
      PrintForce("socket close NOK - stay in closing state")
    }
  }
  else
  {
    PrintForce("socket create FAIL")
  }
}




/**
  * @brief  Socket thread
  * @note   Infinite loop celperf body
  * @param  argument - parameter osThread
  * @note   UNUSED
  * @retval None
  */
/* Functions Definition ------------------------------------------------------*/

void celperf_init(void)
{
  /* Socket initialization */
}

/**
  * @brief  Start
  * @note   paerf start
  * @param  None
  * @retval None
  */
void celperf_start(void)
{
  CMD_Declare(celperf_cmd_label, celperf_cmd, "cellular throughput measure");
}

#endif  /* (USE_CELPERF == 1)     */
#endif  /* (USE_CMD_CONSOLE == 1) */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
