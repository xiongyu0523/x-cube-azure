/**
  ******************************************************************************
  * @file    cmd.c
  * @author  MCD Application Team
  * @brief   cosole cmd management
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
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "usart.h"
#include "cmd.h"
#include "cmsis_os_misrac2012.h"
#include "error_handler.h"
#include "plf_config.h"
#include "string.h"
#if (USE_LINK_UART == 1)
#include "dc_generic.h"
#endif  /* (USE_LINK_UART == 1) */
#include <stdbool.h>

#if (USE_CMD_CONSOLE == 1)

/* Private defines -----------------------------------------------------------*/

#define CMD_MAX_CMD          18U
#define CMD_MAX_LINE_SIZE   100U
#define CMD_LINE_SIZE_MAX   256U


/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_UTILITIES, DBL_LVL_P0, "" format "", ## args)
#else
#include <stdio.h>
#define PrintForce(format, args...)   printf("" format "", ## args);
#endif  /* (USE_PRINTF == 0U) */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t         *CmdName;
  uint8_t         *CmdLabel;
  CMD_HandlerCmd  CmdHandler;
} CMD_Struct_t;

/* Private variables ---------------------------------------------------------*/

static uint8_t             CMD_ReceivedChar;
static uint8_t             CMD_Prompt[5];
static CMD_Struct_t        CMD_a_cmd_list[CMD_MAX_CMD];
static uint8_t             CMD_LastCommandLine[CMD_MAX_LINE_SIZE];
static uint8_t             CMD_CommandLine[2][CMD_MAX_LINE_SIZE];
static osSemaphoreId       CMD_rcvSemaphore  = 0U;
static uint32_t            CMD_ReadMemType   = 8U;
static uint32_t            CMD_NbCmd         = 0U;

static uint8_t CMD_last_command_echo;

static uint8_t*  CMD_current_cmd;
static uint8_t*  CMD_current_rcv_line;
static uint8_t*  CMD_completed_line;
static uint32_t  CMD_CurrentPos    = 0U;

#if (USE_LINK_UART == 1)
static uint8_t*  CMD_link_current_rcv_line;
static uint8_t*  CMD_link_completed_line;
static uint8_t             CMD_LinkReceivedChar;
#endif /* (USE_LINK_UART == 1) */
/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void cmd_thread(const void *argument);
static void CMD_process(void);
static void CMD_GetLine(uint8_t *command_line, uint32_t max_size);
static uint8_t CMD_print_char_print(uint8_t p_car);
static cmd_status_t CMD_BoardReset(uint8_t *p_Cmd_p);
static cmd_status_t CMD_Help(uint8_t *p_Cmd_p);

/* Functions Definition ------------------------------------------------------*/

static void CMD_process(void)
{
  uint8_t  command_line[CMD_MAX_LINE_SIZE];
  uint32_t i;
  uint32_t cmd_size;

  uint32_t cmd_line_len;

  /* get command line */
  CMD_GetLine(command_line, CMD_MAX_LINE_SIZE);
  if (CMD_last_command_echo == 0U)
  {
    if (command_line[0] != (uint8_t)'#')
    {
      /* not a comment line    */
      cmd_line_len = crs_strlen(command_line);
      if (command_line[0] == 0U)
      {
        if (CMD_LastCommandLine[0] == 0U)
        {
          /* no last command: display help  */
          traceIF_trace_off();
          (void)memcpy((CRC_CHAR_t *)command_line, (CRC_CHAR_t *)"help", crs_strlen("help")+1U);
        }
        else
        {
          /* execute again last command  */
          (void)memcpy((CRC_CHAR_t *)command_line, (CRC_CHAR_t *)CMD_LastCommandLine, crs_strlen(CMD_LastCommandLine)+1U);
        }
      }
      else if (cmd_line_len > 1U)
      {
        /* store last command             */
        (void)memcpy((CRC_CHAR_t *)CMD_LastCommandLine, (CRC_CHAR_t *)command_line, crs_strlen(command_line)+1U);
      }
      else
      {
        /* Nothing to do */
      }

      /* command analysis                     */
      for(i = 0; i<CMD_MAX_LINE_SIZE ;  i++)
      {
         if((command_line[i] == (uint8_t)' ') || (command_line[i] == (uint8_t)0))
         {
           break;
         }
      }

      if(i != CMD_MAX_LINE_SIZE)
      {
        /* not an empty line        */
        cmd_size = i;
        for (i = 0U; i < CMD_NbCmd ; i++)
        {
          if (memcmp((CRC_CHAR_t *)CMD_a_cmd_list[i].CmdName, (CRC_CHAR_t *)command_line, cmd_size)
              == 0)
          {
            /* Command  found => call processing  */
            PrintForce("\r\n")
            (void)CMD_a_cmd_list[i].CmdHandler((uint8_t *)command_line);
            break;
          }
        }
        if (i >= CMD_NbCmd)
        {
          /* unknown command   */
          PrintForce("\r\nCMD : unknown command : %s\r\n", command_line)
          (void)CMD_Help(command_line);
        }
      }
    }
    else
    {
      PrintForce("\r\n")
    }
    PrintForce("%s", (CRC_CHAR_t *)CMD_Prompt)
  }
  else
  {
    CMD_last_command_echo = 0U;
  }
}


static uint8_t CMD_print_char_print(uint8_t p_car)
{
  uint8_t ret;
  if ((p_car > 0x1FU) && (p_car < 0x7FU))
  {
    ret =  p_car;
  }
  else
  {
    ret = (uint8_t)'.';
  }
  return ret;
}

uint32_t CMD_GetValue(uint8_t *string_p, uint32_t *value_p)
{
  uint32_t ret;
  uint8_t digit8;
  uint32_t digit;
  ret = 0U;

  if (string_p == NULL)
  {
    ret = 1U;
  }
  else
  {
    if (memcmp((CRC_CHAR_t *)string_p, "0x", 2U) == 0)
    {
      *value_p = (uint32_t)crs_atoi_hex(&string_p[2]);
    }
    else
    {
      digit8 = (*string_p - (uint8_t)'0');
      digit  = (uint32_t)digit8;
      if (digit <= 9U)
      {
        *value_p = (uint32_t)crs_atoi(string_p);
      }
      else
      {
        ret = 1U;
        *value_p = 0U;
      }
    }
  }
  return ret;
}

/*-------------------------------------------------------------------------*/
/* F_Dumpx                                                                 */
/*-------------------------------------------------------------------------*/
static void CMD_Dump8(uint8_t *addr, uint32_t size)
{
  /*_VAR_*/
  uint8_t    tmp_char;
  uint32_t   i;
  uint32_t   j;
  uint8_t    *tmp_string_p;
  static uint8_t cmd_line_p[100];

  tmp_string_p = addr;

  for (i = 0U; i < size; i += 16U)
  {
    /* begin of line addr display        */
    (void)memset(cmd_line_p, 0, sizeof(cmd_line_p));
    (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                  "%08lX : ", (uint32_t)&tmp_string_p[i]);
    /*  hexa display      */

    for (j = 0U ; (j < 16U) && ((i + j) < size) ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "%02X ",
                    tmp_string_p[ i + j]);
    }
    for (; j < 16U ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)], "   ");
    }

    /* ASCII display      */
    (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)], " ");
    for (j = 0U ; (j < 16U) && ((i + j) < size) ; j++)
    {
      tmp_char = tmp_string_p[i + j];
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "%c",
                    CMD_print_char_print((uint8_t)tmp_char));
    }
    PrintForce("%s\r\n", cmd_line_p);
  }
}

static void CMD_Dump16(uint16_t *addr, uint32_t size)
{
  uint16_t tmp_char;
  uint32_t i;
  uint32_t j;
  uint16_t *tmp_string_p;
  static uint8_t cmd_line_p[100];

  /* 8 chars by line displayed    */
  tmp_string_p = addr;
  for (i = 0U; i < size ; i += 8U)
  {
    /* begin of line addr display        */
    (void)memset(cmd_line_p, 0, sizeof(cmd_line_p));
    (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                  "%08lX : ",
                  (uint32_t)&tmp_string_p[i]);

    /* 2 byte format display       */
    for (j = 0U ; (j < 8U) && ((i + j) < size) ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "%04X ", tmp_string_p[i + j]);
    }
    for (; j < 8U ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)], "     ");
    }

    /* ASCII display      */
    (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)], " ");
    for (j = 0U ; (j < 8U) && ((i + j) < size) ; j++)
    {
      tmp_char = tmp_string_p[i + j];
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "%c%c", CMD_print_char_print((uint8_t)(tmp_char >> 8U)),
                    CMD_print_char_print((uint8_t)(tmp_char & 0x00FFU)));
    }
    PrintForce("%s\r\n", cmd_line_p);
  }
}
static void CMD_Dump32(uint32_t *addr, uint32_t size)
{
  uint8_t   tmp[4];
  uint32_t  tmp_char;
  uint32_t i;
  uint32_t j;
  uint32_t  *tmp_string_p;
  static uint8_t cmd_line_p[100];

  /* 4 Bytes format display    */
  tmp_string_p = addr;
  for (i = 0U; i < size ; i += 4U)
  {
    /* begin of line addr display        */
    (void)memset(cmd_line_p, 0, sizeof(cmd_line_p));
    (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                  "%08lX : ",
                  (uint32_t)&tmp_string_p[i]);

    /* hexa display       */
    for (j = 0U ; (j < 4U) && ((i + j) < size) ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "%08lX ", tmp_string_p[i + j]);
    }
    for (; j < 4U ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "            ");
    }

    /* ASCII display      */
    (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)], " ");
    for (j = 0U ; (j < 4U) && ((i + j) < size) ; j++)
    {
      tmp_char = tmp_string_p[i + j];
      (void)memcpy(tmp, (uint8_t *)&tmp_char, sizeof(uint32_t));
      (void)sprintf((CRC_CHAR_t *)&cmd_line_p[crs_strlen(cmd_line_p)],
                    "%c%c%c%c",
                    CMD_print_char_print(tmp[3]),
                    CMD_print_char_print(tmp[2]),
                    CMD_print_char_print(tmp[1]),
                    CMD_print_char_print(tmp[0]));
    }
    PrintForce("%s\r\n", cmd_line_p);
  }
}

/*-------------------------------------------------------------------------*/
/* CMD_ReadMem                                                              */
/*-------------------------------------------------------------------------*/
static cmd_status_t CMD_ReadMem(uint8_t *cmd_line_p)
{
  /*_VAR_*/

  uint32_t   argc;
  uint32_t   ret;
  cmd_status_t res;
  uint32_t   value;
  uint8_t   *argv_p[10];
  static uint32_t  CMD_ReaMemdSize   = 16;
  static uint8_t   *CMD_ReaMemdAddr_p = 0U;

  res = CMD_OK;
  /* command name suppress  */
  (void)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  /* parmeters parsing                     */
  for (argc = 0U ; argc < 10U ; argc++)
  {
    argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
    if (argv_p[argc] == NULL)
    {
      break;
    }
  }

  /* parameters test                        */
  if (strncmp((CRC_CHAR_t *)argv_p[0], (CRC_CHAR_t *)"help", crs_strlen(argv_p[0])) == 0)
  {
    PrintForce("\r\nreadmem [<address> [<size> [b|s|l]]]]\r\n");
  }
  else
  {
    if (argc >= 3U)
    {
      const uint8_t *tmp_str;
      tmp_str = argv_p[2];
      if ((tmp_str[0] == (uint8_t)'w') || (tmp_str[0] == (uint8_t)'s'))
      {
        /* lecture 16 bits */
        CMD_ReadMemType = 16;
      }
      else if (tmp_str[0] == (uint8_t)'l')
      {
        /* lecture 32 bits */
        CMD_ReadMemType = 32;
      }
      else
      {
        /* lecture 8 bits */
        CMD_ReadMemType = 8;
      }
    }

    if (argc >= 2U)
    {
      ret = CMD_GetValue(argv_p[1], &CMD_ReaMemdSize);
      if (ret != 0U)
      {
        PrintForce("<size> non valide\r\n");
        res = CMD_SYNTAX_ERROR;
      }
    }

    if ((res == CMD_OK ) && (argc >= 1U))
    {
      ret = CMD_GetValue(argv_p[0], &value);
      if (ret != 0U)
      {
        PrintForce("<addr> non valide\r\n");
        res = CMD_SYNTAX_ERROR;
      }
      CMD_ReaMemdAddr_p = (uint8_t *) value;
    }

    if (res == CMD_OK )
    {
      PrintForce("\r\n");
      if (CMD_ReaMemdSize > CMD_LINE_SIZE_MAX)
      {
        CMD_ReaMemdSize = CMD_LINE_SIZE_MAX;
      }

      /* Line of 16 bytes displayed   */
      if (CMD_ReadMemType == 32U)
      {
        CMD_Dump32((uint32_t *)CMD_ReaMemdAddr_p, CMD_ReaMemdSize);
      }
      else if (CMD_ReadMemType == 16U)
      {
        CMD_Dump16((uint16_t *)CMD_ReaMemdAddr_p, CMD_ReaMemdSize);
      }
      else
      {
        CMD_Dump8(CMD_ReaMemdAddr_p, CMD_ReaMemdSize);
      }
      /* reading next prepare   */
      CMD_ReaMemdAddr_p = CMD_ReaMemdAddr_p + (CMD_ReaMemdSize * (CMD_ReadMemType >> 3));
    }
  }
  return res;
}


/* cmd thread */
static void cmd_thread(const void *argument)
{
  for (;;)
  {
    CMD_process();
  }
}


static void CMD_GetLine(uint8_t *command_line, uint32_t max_size)
{
  UNUSED(max_size);
  (void)osSemaphoreWait(CMD_rcvSemaphore, RTOS_WAIT_FOREVER);
  if (CMD_last_command_echo == 1U)
  {
    PrintForce("\r\n%s\r\n", CMD_LastCommandLine)
  }
  else
  {
    (void)memcpy((CRC_CHAR_t *)command_line, (CRC_CHAR_t *)CMD_current_cmd, crs_strlen(CMD_current_cmd)+1U);
  }
}

/*-------------------------------------------------------------------------*/
/* CMD_BoardReset                                                             */
/*-------------------------------------------------------------------------*/
static cmd_status_t CMD_BoardReset(uint8_t *p_Cmd_p)
{
  UNUSED(p_Cmd_p);
  uint32_t i;
  PrintForce("Board reset requested !\r\n");
  (void)osDelay(1000);

  /* display declared commands  */
  NVIC_SystemReset();
  for (i = 1; i < 1000U; i++)
  {
    (void)osDelay(1000);
  }

  return CMD_OK;
}

#define CMD_COMMAND_ALIGN_COLUMN 16U

/*-------------------------------------------------------------------------*/
/* CMD_Help                                                             */
/*-------------------------------------------------------------------------*/
static cmd_status_t CMD_Help(uint8_t *p_Cmd_p)
{
  UNUSED(p_Cmd_p);
  uint32_t i;
  uint32_t align_offset;
  uint32_t cmd_size;
  PrintForce("***** help *****\r\n");

  PrintForce("\r\nList of commands\r\n")
  PrintForce("----------------\r\n")
  uint8_t   CMD_CmdAlignOffsetString[CMD_COMMAND_ALIGN_COLUMN];

  /* display declared commands  */
  for (i = 0U; i < CMD_NbCmd ; i++)
  {
    cmd_size = (uint32_t)crs_strlen(CMD_a_cmd_list[i].CmdName);
    align_offset = CMD_COMMAND_ALIGN_COLUMN - cmd_size;
    if ((align_offset < CMD_COMMAND_ALIGN_COLUMN))
    {
      (void)memset(CMD_CmdAlignOffsetString, (int32_t)' ', align_offset);
      CMD_CmdAlignOffsetString[align_offset] = 0U;
    }
    PrintForce("%s%s %s\r\n", CMD_a_cmd_list[i].CmdName, CMD_CmdAlignOffsetString, CMD_a_cmd_list[i].CmdLabel);
  }
  PrintForce("\r\nHelp syntax\r\n");
  PrintForce("-----------\r\n");
  PrintForce("warning: case sensitive commands\r\n");
  PrintForce("[optional parameter]\r\n");
  PrintForce("<parameter value>\r\n");
  PrintForce("<val_1>|<val_2>|...|<val_n>: parameter value list\r\n");
  PrintForce("(command description)\r\n");
  PrintForce("return key: last command re-execution\r\n");
  PrintForce("#: comment line\r\n");
  PrintForce("\r\nAdvice\r\n");
  PrintForce("-----------\r\n");
  PrintForce("to use commands it is adviced to use one of the following command to disable traces\r\n");
  PrintForce("trace off (allows disable all traces)\r\n");
  PrintForce("cst polling off  (allows to disable modem polling and avoid to display uncomfortable modem traces\r\n");
  PrintForce("\r\n");
  PrintForce("type 'trace on' to re-enable traces\r\n");

  return CMD_OK;
}

void CMD_Declare(uint8_t *cmd_name,
                 CMD_HandlerCmd cmd_handler, uint8_t *cmd_label)
{
  if (CMD_NbCmd >= CMD_MAX_CMD)
  {
    ERROR_Handler(DBG_CHAN_UTILITIES, 10, ERROR_WARNING);
  }
  else
  {
    CMD_a_cmd_list[CMD_NbCmd].CmdName    = cmd_name;
    CMD_a_cmd_list[CMD_NbCmd].CmdLabel   = cmd_label;
    CMD_a_cmd_list[CMD_NbCmd].CmdHandler = cmd_handler;

    CMD_NbCmd++;
  }
}

void CMD_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  static UART_HandleTypeDef *CMD_CurrentUart;

  CMD_CurrentUart = UartHandle;
  uint8_t rec_char;
  uint8_t* temp;
  HAL_StatusTypeDef ret;

  rec_char = CMD_ReceivedChar;

  ret = HAL_UART_Receive_IT(CMD_CurrentUart, (uint8_t *)&CMD_ReceivedChar, 1U);
  if (ret != HAL_OK)
  {
    traceIF_UartBusyFlag = &CMD_ReceivedChar;
  }

  if ((rec_char == (uint8_t)'\r') || (CMD_CurrentPos >= (CMD_MAX_LINE_SIZE - 1U)))
  {
    CMD_current_rcv_line[CMD_CurrentPos] = 0;
    temp = CMD_completed_line;
    CMD_completed_line = CMD_current_rcv_line;
    CMD_current_cmd    = CMD_completed_line;
    CMD_current_rcv_line = temp;
    CMD_CurrentPos = 0;
    (void)osSemaphoreRelease(CMD_rcvSemaphore);
  }
  else
  {
    if (rec_char == (uint8_t)'\b')
    {
      /* back space */
      if (CMD_CurrentPos > 0U)
      {
        CMD_CurrentPos--;
      }
    }
    else
    {
      /* normal char  */
      CMD_current_rcv_line[CMD_CurrentPos] = rec_char;
      CMD_CurrentPos++;
    }
  }
}

#if (USE_LINK_UART == 1)
void CMD_RxLinkCpltCallback(UART_HandleTypeDef *UartHandle)
{
  static uint32_t  CMD_LinkCurrentPos  = 0U;
  static UART_HandleTypeDef *CMD_CurrentUart;

  CMD_CurrentUart = UartHandle;
  uint8_t rec_char;
  uint8_t* temp;
  HAL_StatusTypeDef ret;

  rec_char = CMD_LinkReceivedChar;
  ret = HAL_UART_Receive_IT(CMD_CurrentUart, (uint8_t *)&CMD_LinkReceivedChar, 1U);
  if (ret != HAL_OK)
  {
    dc_generic_UartBusyFlag = &CMD_LinkReceivedChar;
  }

  if ((rec_char == (uint8_t)'\r') || (CMD_LinkCurrentPos >= (CMD_MAX_LINE_SIZE - 1U)))
  {
    CMD_link_current_rcv_line[CMD_LinkCurrentPos] = 0;
    temp = CMD_link_completed_line;
    CMD_link_completed_line = CMD_link_current_rcv_line;
    CMD_current_cmd    = CMD_link_completed_line;
    CMD_link_current_rcv_line = temp;
    CMD_LinkCurrentPos = 0;
    (void)osSemaphoreRelease(CMD_rcvSemaphore);
  }
  else
  {
    if (rec_char == (uint8_t)'\b')
    {
      /* back space */
      if (CMD_LinkCurrentPos > 0U)
      {
        CMD_LinkCurrentPos--;
      }
    }
    else
    {
      /* normal char  */
      CMD_link_current_rcv_line[CMD_LinkCurrentPos] = rec_char;
      CMD_LinkCurrentPos++;
    }
  }
}
#endif  /* (USE_LINK_UART == 1) */

/*-------------------------------------------------------------------------*/
/* CMD_print_help                                                          */
/*-------------------------------------------------------------------------*/
void CMD_print_help(uint8_t *label)
{
  PrintForce("***** %s help *****\r\n", label);
}


/*-------------------------------------------------------------------------*/
/* CMD_F_Init                                                             */
/*-------------------------------------------------------------------------*/
void CMD_Init(void)
{
#if (USE_LINK_UART == 1)
  static uint8_t   CMD_LinkCommandLine[2][CMD_MAX_LINE_SIZE];
#endif  /* (USE_LINK_UART == 1) */
  static uint8_t* CMD_promt = "$>";
  static osThreadId CMD_ThreadId;
  CMD_NbCmd           = 0U;
  CMD_ReadMemType     = 8U;

  CMD_CommandLine[0][0] = 0;
  CMD_CommandLine[1][0] = 0;
  CMD_current_rcv_line = CMD_CommandLine[0];
  CMD_current_cmd      = CMD_CommandLine[1];
  CMD_completed_line   = CMD_CommandLine[1];
  CMD_CurrentPos = 0;
  CMD_last_command_echo = 0U;
  traceIF_UartBusyFlag = NULL;

#if (USE_LINK_UART == 1)
  dc_generic_UartBusyFlag = NULL;
  CMD_link_current_rcv_line = CMD_LinkCommandLine[0];
  CMD_link_completed_line   = CMD_LinkCommandLine[1];
#endif  /* (USE_LINK_UART == 1) */

  (void)memcpy((CRC_CHAR_t *)CMD_Prompt, (CRC_CHAR_t *)CMD_promt, crs_strlen(CMD_promt)+1U);

  CMD_Declare((uint8_t *)"help", CMD_Help, (uint8_t *)"help command");
  CMD_Declare((uint8_t *)"readmem", CMD_ReadMem, (uint8_t *)"read memory");
  CMD_Declare((uint8_t *)"reset", CMD_BoardReset, (uint8_t *)"board reset");

  CMD_LastCommandLine[0] = 0;

  osSemaphoreDef(SEM_CMD_RCV);
  CMD_rcvSemaphore = osSemaphoreCreate(osSemaphore(SEM_CMD_RCV), 1);
  (void)osSemaphoreWait(CMD_rcvSemaphore, RTOS_WAIT_FOREVER);

  osThreadDef(CMD_THREAD_DEF, cmd_thread, CMD_THREAD_PRIO, 0, USED_CMD_THREAD_STACK_SIZE);
  CMD_ThreadId = osThreadCreate(osThread(CMD_THREAD_DEF), NULL);
  if (CMD_ThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_UTILITIES, 2, ERROR_FATAL);
  }
  else
  {
#if (STACK_ANALYSIS_TRACE == 1)
    stackAnalysis_addStackSizeByHandle(CMD_ThreadId, USED_CMD_THREAD_STACK_SIZE);
#endif /* STACK_ANALYSIS_TRACE */
  }


}
/*-------------------------------------------------------------------------*/
/* CMD_F_Init                                                             */
/*-------------------------------------------------------------------------*/
void CMD_Start(void)
{
  HAL_StatusTypeDef ret;

  CMD_CommandLine[0][0] = 0;
  CMD_CommandLine[1][0] = 0;

#if (USE_LINK_UART == 1)
  COM_INTERFACE_UART_INIT
  while (true)
  {
    ret = HAL_UART_Receive_IT(&COM_INTERFACE_UART_HANDLE, &CMD_LinkReceivedChar, 1U);
    if (ret == HAL_OK)
    {
      break;
    }
    (void)osDelay(10);
  }
#endif /* USE_LINK_UART */
  while (true)
  {
    ret = HAL_UART_Receive_IT(&TRACE_INTERFACE_UART_HANDLE, &CMD_ReceivedChar , 1U);
    if (ret == HAL_OK)
    {
      break;
    }
    (void)osDelay(10);
  }
}

#endif  /* USE_CMD_CONSOLE */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
