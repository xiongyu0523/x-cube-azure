/**
  ******************************************************************************
  * @file    trace_interface.c
  * @author  MCD Application Team
  * @brief   This file contains trace define utilities
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
#include "trace_interface.h"
#include "main.h"
#include "cmsis_os_misrac2012.h"
#include <stdio.h>
#include <string.h>

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */


/* Private typedef -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define PrintForce(format, args...)  TracePrintForce(DBG_CHAN_UTILITIES, DBL_LVL_P0, format, ## args)
#define TRACE_IF_CHAN_MAX_VALUE 14U
/* Private defines -----------------------------------------------------------*/
#define MAX_HEX_PRINT_SIZE     210U

/* Private variables ---------------------------------------------------------*/
static uint8_t traceIF_traceEnable = 1U;
static uint32_t traceIF_Level = TRACE_IF_MASK;
static osMutexId traceIF_uart_mutex;

static uint8_t traceIF_traceComponent[TRACE_IF_CHAN_MAX_VALUE] =
{
  1U,   /*  DBG_CHAN_GENERIC           */
  1U,   /*  DBG_CHAN_MAIN              */
  1U,   /*  DBG_CHAN_ATCMD             */
  1U,   /*  DBG_CHAN_COM               */
  1U,   /*  DBG_CHAN_ECHOCLIENT        */
  1U,   /*  DBG_CHAN_HTTP              */
  1U,   /*  DBG_CHAN_PING              */
  1U,   /*  DBG_CHAN_IPC               */
  1U,   /*  DBG_CHAN_PPPOSIF           */
  1U,   /*  DBG_CHAN_CELLULAR_SERVICE  */
  1U,   /*  DBG_CHAN_NIFMAN            */
  1U,   /*  DBG_CHAN_DATA_CACHE        */
  1U,   /*  DBG_CHAN_UTILITIES         */
  1U,   /*  DBG_CHAN_ERROR_LOGGER      */
};

#if (USE_CMD_CONSOLE == 1)
static uint8_t *trace_cmd_label = (uint8_t *)"trace";
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private function prototypes -----------------------------------------------*/
static void ITM_Out(uint32_t port, uint32_t ch);
#if (USE_CMD_CONSOLE == 1)
static cmd_status_t traceIF_cmd(uint8_t *cmd_line_p);
static void traceIF_cmd_Help(void);
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Global variables ----------------------------------------------------------*/
uint8_t dbgIF_buf[DBG_CHAN_MAX_VALUE][DBG_IF_MAX_BUFFER_SIZE];
uint8_t *traceIF_UartBusyFlag = NULL;

/* Functions Definition ------------------------------------------------------*/
/* static functions */

#if (USE_CMD_CONSOLE == 1)
static void CMD_ComponentEnableDisable(uint8_t *component, uint8_t enable)
{
  static uint8_t *traceIF_traceComponentName[DBG_CHAN_MAX_VALUE] =
  {
    (uint8_t *)"generic",
    (uint8_t *)"main",
    (uint8_t *)"atcmd",
    (uint8_t *)"com",
    (uint8_t *)"echoclient",
    (uint8_t *)"http",
    (uint8_t *)"ping",
    (uint8_t *)"ipc",
    (uint8_t *)"ppposif",
    (uint8_t *)"cellular_service",
    (uint8_t *)"nifman",
    (uint8_t *)"data_cache",
    (uint8_t *)"utilities",
    (uint8_t *)"error"
  };
  uint8_t    i ;
  if (strncmp((TRACE_INTERF_CHAR_t *)component, "all", strlen((TRACE_INTERF_CHAR_t *)component)) == 0)
  {
    for (i = 0U; i < TRACE_IF_CHAN_MAX_VALUE ; i++)
    {
      traceIF_traceComponent[i] = enable;
    }
  }
  else
  {
    for (i = 0U; i < TRACE_IF_CHAN_MAX_VALUE ; i++)
    {
      if (strncmp((TRACE_INTERF_CHAR_t *)component, (TRACE_INTERF_CHAR_t *)traceIF_traceComponentName[i],
                  strlen((TRACE_INTERF_CHAR_t *)component)) == 0)
      {
        break;
      }
    }
    if (i >= TRACE_IF_CHAN_MAX_VALUE)
    {
      PrintForce("invalid canal name %s\r\n", component);
    }
    else
    {
      traceIF_traceComponent[i] = enable;
    }
  }
}

static void traceIF_cmd_Help(void)
{
  CMD_print_help(trace_cmd_label);

  PrintForce("%s help\r\n", trace_cmd_label);
  PrintForce("%s on (active traces)\r\n", trace_cmd_label);
  PrintForce("%s off (deactive traces)\r\n", trace_cmd_label);
  PrintForce("%s enable  all|generic|main|atcmd|com|echoclient|http|ping|ipc|ppposif|cellular_service|nifman|data_cache|utilities|error\r\n",
             trace_cmd_label)
  PrintForce(" -> enable traces of selected component\r\n")
  PrintForce("%s disable all|generic|main|atcmd|com|echoclient|http|ping|ipc|ppposif|cellular_service|nifman|data_cache|utilities|error\r\n",
             trace_cmd_label)
  PrintForce(" -> disable traces of selected component\r\n")
}

/**
  * @brief  console cmd management
  * @param  cmd_line_p - command line
  * @note   Unused
  * @retval None
  */
static cmd_status_t traceIF_cmd(uint8_t *cmd_line_p)
{
  uint8_t    *argv_p[10];
  uint32_t    argc;
  const uint8_t    *cmd_p;
  uint32_t    level;
  uint32_t    ret ;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  cmd_p = (uint8_t *)strtok((TRACE_INTERF_CHAR_t *)cmd_line_p, " \t");

  if (strncmp((const TRACE_INTERF_CHAR_t *)cmd_p, (const TRACE_INTERF_CHAR_t *)trace_cmd_label,
              strlen((const TRACE_INTERF_CHAR_t *)cmd_p)) == 0)
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

    if ((argc == 0U) || (strncmp((TRACE_INTERF_CHAR_t *)argv_p[0], "help", strlen((TRACE_INTERF_CHAR_t *)argv_p[0])) == 0))
    {
      traceIF_cmd_Help();
    }
    else if (strncmp((TRACE_INTERF_CHAR_t *)argv_p[0], "on", strlen((TRACE_INTERF_CHAR_t *)argv_p[0])) == 0)
    {
      traceIF_traceEnable = 1;
      PrintForce("\n\r <<< TRACE ACTIVE >>>\n\r")
    }
    else if (strncmp((TRACE_INTERF_CHAR_t *)argv_p[0], "enable", strlen((TRACE_INTERF_CHAR_t *)argv_p[0])) == 0)
    {
      PrintForce("\n\r <<< TRACE ENABLE >>>\n\r")
      CMD_ComponentEnableDisable(argv_p[1], 1);
    }
    else if (strncmp((TRACE_INTERF_CHAR_t *)argv_p[0], "disable", strlen((TRACE_INTERF_CHAR_t *)argv_p[0])) == 0)
    {
      PrintForce("\n\r <<< TRACE DISABLE >>>\n\r")
      CMD_ComponentEnableDisable(argv_p[1], 0);
    }
    else if (strncmp((TRACE_INTERF_CHAR_t *)argv_p[0], "level", strlen((TRACE_INTERF_CHAR_t *)argv_p[0])) == 0)
    {
      PrintForce("\n\r <<< TRACE LEVEL >>>\n\r")
      ret = CMD_GetValue(argv_p[1], (uint32_t *)&level);
      if (ret != 0U)
      {
        PrintForce("invalid level %s\r\n", argv_p[1]);
        cmd_status = CMD_SYNTAX_ERROR;
      }
      else
      {
        traceIF_Level = level;
      }
    }
    else if (strncmp((TRACE_INTERF_CHAR_t *)argv_p[0], "off", strlen((TRACE_INTERF_CHAR_t *)argv_p[0])) == 0)
    {
      traceIF_traceEnable = 0;
      PrintForce("\n\r <<< TRACE INACTIVE >>>\n\r")
    }
    else
    {
      PrintForce("\n\rTRACE usage\n\r")
      traceIF_cmd_Help();
    }
  }

  return cmd_status;
}
#endif  /* (USE_CMD_CONSOLE == 1) */


static void ITM_Out(uint32_t port, uint32_t ch)
{
  /* check port validity (0-31)*/
  if (port <= 31U)
  {
    uint32_t tmp_mask;
    tmp_mask = (ITM->TER & (1UL << port));
    if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&   /* ITM enabled ? */
        (tmp_mask != 0UL))   /* ITM selected Port enabled ? */
    {

      /* Wait until ITM port is ready */
      while (ITM->PORT[port].u32 == 0UL)
      {
        __NOP();
      }

      /* then send data, one byte at a time */
      ITM->PORT[port].u8 = (uint8_t) ch;
    }
  }
}


static void traceIF_uartTransmit(uint8_t *ptr, uint16_t len)
{
  (void)osMutexWait(traceIF_uart_mutex, RTOS_WAIT_FOREVER);
  (void)HAL_UART_Transmit(&TRACE_INTERFACE_UART_HANDLE, (uint8_t *)ptr, len, 0xFFFFU);
  (void)osMutexRelease(traceIF_uart_mutex);
  if (traceIF_UartBusyFlag != NULL)
  {
    while (HAL_UART_Receive_IT(&TRACE_INTERFACE_UART_HANDLE, traceIF_UartBusyFlag, 1U) != HAL_OK)
    {
    }
    traceIF_UartBusyFlag = NULL;
  }
}

/* exported functions */
void traceIF_itmPrint(uint8_t port, uint8_t lvl, uint8_t *pptr, uint16_t len)
{
  uint32_t i;
  uint8_t *ptr;
  ptr = pptr;

  if (traceIF_traceEnable != 0U)
  {
    if ((traceIF_Level & lvl) != 0U)
    {
      if (traceIF_traceComponent[port] != 0U)
      {
        for (i = 0U; i < len; i++)
        {
          ITM_Out((uint32_t) port, (uint32_t) *ptr);
          ptr++;
        }
      }
    }
  }
}

void traceIF_trace_off(void)
{
  traceIF_traceEnable = 0;
}

void traceIF_trace_on(void)
{
  traceIF_traceEnable = 1;
}

void traceIF_uartPrint(uint8_t port, uint8_t lvl, uint8_t *pptr, uint16_t len)
{
  uint8_t *ptr;
  ptr = pptr;

  if (traceIF_traceEnable != 0U)
  {
    if ((traceIF_Level & lvl) != 0U)
    {
      if (traceIF_traceComponent[port] != 0U)
      {
        traceIF_uartTransmit(ptr, len);
      }
    }
  }
}

void traceIF_itmPrintForce(uint8_t port, uint8_t *pptr, uint16_t len)
{
  uint32_t i;
  uint8_t *ptr;
  ptr = pptr;

  for (i = 0U; i < len; i++)
  {
    ITM_Out((uint32_t) port, (uint32_t) *ptr);
    ptr++;
  }
}

void traceIF_uartPrintForce(uint8_t port, uint8_t *pptr, uint16_t len)
{
  UNUSED(port);
  uint8_t *ptr;
  ptr = pptr;

  traceIF_uartTransmit(ptr, len);
}


void traceIF_hexPrint(dbg_channels_t chan, dbg_levels_t level, uint8_t *buff, uint16_t len)
{
#if ((TRACE_IF_TRACES_ITM == 1U) || (TRACE_IF_TRACES_UART == 1U))
  static  uint8_t car[MAX_HEX_PRINT_SIZE];

  uint32_t i;
  uint16_t tmp_len;
  tmp_len = len;

  if (((tmp_len * 2U) + 1U) > MAX_HEX_PRINT_SIZE)
  {
    TracePrint(chan,  level, "TR (%d/%d)\n\r", (MAX_HEX_PRINT_SIZE >> 1) - 2U, tmp_len)
    tmp_len = (MAX_HEX_PRINT_SIZE >> 1) - 2U;
  }

  for (i = 0U; i < tmp_len; i++)
  {
    uint8_t digit = ((buff[i] >> 4U) & 0xfU);
    if (digit <= 9U)
    {
      car[2U * i] =  digit + 0x30U;
    }
    else
    {
      car[2U * i] =  digit + 0x41U - 10U;
    }

    digit = (0xfU & buff[i]);
    if (digit <= 9U)
    {
      car[(2U * i) + 1U] =  digit + 0x30U;
    }
    else
    {
      car[(2U * i) + 1U] =  digit + 0x41U - 10U;
    }
  }
  car[2U * i] =  0U;

  TracePrint(chan,  level, "%s ", (TRACE_INTERF_CHAR_t *)car)
#endif  /* ((TRACE_IF_TRACES_ITM == 1U) || (TRACE_IF_TRACES_UART == 1U)) */
}

void traceIF_BufCharPrint(dbg_channels_t chan, dbg_levels_t level, TRACE_INTERF_CHAR_t *buf, uint16_t size)
{
  for (uint32_t cpt = 0U; cpt < size; cpt++)
  {
    if (buf[cpt] == (TRACE_INTERF_CHAR_t)0)
    {
      TracePrint(chan, level, "<NULL CHAR>")
    }
    else if (buf[cpt] == '\r')
    {
      TracePrint(chan, level, "<CR>")
    }
    else if (buf[cpt] == '\n')
    {
      TracePrint(chan, level, "<LF>")
    }
    else if (buf[cpt] == (TRACE_INTERF_CHAR_t)0x1A)
    {
      TracePrint(chan, level, "<CTRL-Z>")
    }
    else if ((buf[cpt] >= (TRACE_INTERF_CHAR_t)0x20) && (buf[cpt] <= (TRACE_INTERF_CHAR_t)0x7E))
    {
      /* printable TRACE_INTERF_CHAR_t */
      TracePrint(chan, level, "%c", buf[cpt])
    }
    else
    {
      /* Special Character - not printable */
      TracePrint(chan, level, "<SC>")
    }
  }
  TracePrint(chan, level, "\n\r")
}

void traceIF_BufHexPrint(dbg_channels_t chan, dbg_levels_t level, TRACE_INTERF_CHAR_t *buf, uint16_t size)
{
  for (uint32_t cpt = 0U; cpt < size; cpt++)
  {
    TracePrint(chan, level, "0x%02x ", (uint8_t) buf[cpt])
    if ((cpt != 0U) && (((cpt + 1U) % 16U) == 0U))
    {
      TracePrint(chan, level, "\n\r")
    }
  }
  TracePrint(chan, level, "\n\r")
}

void traceIF_Init(void)
{
  traceIF_traceEnable = 1;
#if (USE_CMD_CONSOLE == 1)
  CMD_Declare((uint8_t *)"trace", traceIF_cmd, (uint8_t *)"trace management");
#endif  /* (USE_CMD_CONSOLE == 1) */
  osMutexDef(osTraceUartMutex);
  traceIF_uart_mutex = osMutexCreate(osMutex(osTraceUartMutex));
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
