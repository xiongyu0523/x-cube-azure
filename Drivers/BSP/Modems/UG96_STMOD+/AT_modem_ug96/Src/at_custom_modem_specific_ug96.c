#ifdef USE_MODEM_UG96
/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          UG96 Quectel modem (3G)
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

/* AT commands format
 * AT+<X>=?    : TEST COMMAND
 * AT+<X>?     : READ COMMAND
 * AT+<X>=...  : WRITE COMMAND
 * AT+<X>      : EXECUTION COMMAND
*/

/* UG96 COMPILATION FLAGS to define in project option if needed:*/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_custom_modem_specific_ug96.h"
#include "at_custom_modem_signalling_ug96.h"
#include "at_custom_modem_socket_ug96.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"
#include "plf_modem_config_ug96.h"
#include "error_handler.h"

#if defined(USE_MODEM_UG96)
#if defined(HWREF_B_CELL_UG96)
#else
#error Hardware reference not specified
#endif /* HWREF_B_CELL_UG96 */
#endif /* USE_MODEM_UG96 */

/* Private typedef -----------------------------------------------------------*/
typedef char ug96_TYPE_CHAR_t;

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "UG96:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "UG96:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "UG96 API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "UG96 ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P1, (char *)pbuf, size);
#else
#define PrintINFO(format, args...)  printf("UG96:" format "\n\r", ## args);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   printf("UG96 ERROR:" format "\n\r", ## args);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

/* Private defines -----------------------------------------------------------*/
/* ###########################  START CUSTOMIZATION PART  ########################### */
#define UG96_DEFAULT_TIMEOUT  ((uint32_t)15000)
#define UG96_RDY_TIMEOUT      ((uint32_t)10000)
#define UG96_SIMREADY_TIMEOUT ((uint32_t)30000)
#define UG96_ESCAPE_TIMEOUT   ((uint32_t)1000)  /* maximum time allowed to receive a response to an Escape command */
#define UG96_COPS_TIMEOUT     ((uint32_t)180000) /* 180 sec */
#define UG96_CGATT_TIMEOUT    ((uint32_t)140000) /* 140 sec (75 sec in doc but seems not always enough) */
#define UG96_CGACT_TIMEOUT    ((uint32_t)150000) /* 15 sec */
#define UG96_ATH_TIMEOUT      ((uint32_t)90000) /* 90 sec */
#define UG96_AT_TIMEOUT       ((uint32_t)3000)  /* timeout for AT */
#define UG96_SOCKET_PROMPT_TIMEOUT ((uint32_t)10000)
#define UG96_QIOPEN_TIMEOUT        ((uint32_t)150000) /* 150 sec */
#define UG96_QICLOSE_TIMEOUT       ((uint32_t)150000) /* 150 sec */
#define UG96_QIACT_TIMEOUT         ((uint32_t)150000) /* 150 sec */
#define UG96_QIDEACT_TIMEOUT       ((uint32_t)40000) /* 40 sec */
#define UG96_QIDNSGIP_TIMEOUT      ((uint32_t)60000) /* 60 sec */
#define UG96_QPING_TIMEOUT         ((uint32_t)150000) /* 150 sec */

/* Global variables ----------------------------------------------------------*/
/* UG96 Modem device context */
static atcustom_modem_context_t UG96_ctxt;

/* shared variables specific to UG96 */
ug96_shared_variables_t ug96_shared = {
 .boot_synchro = AT_FALSE,
 .sim_card_ready = AT_FALSE,
 .change_ipr_baud_rate = AT_FALSE,
 .QIOPEN_waiting = AT_FALSE,
 .QICGSP_config_command = AT_TRUE,
};

/* Private variables ---------------------------------------------------------*/

/* Socket Data receive: to analyze size received in data header */
static AT_CHAR_t SocketHeaderDataRx_Buf[4];
static uint8_t SocketHeaderDataRx_Cpt;
static uint8_t SocketHeaderDataRx_Cpt_Complete;

/* ###########################  END CUSTOMIZATION PART  ########################### */

/* Private function prototypes -----------------------------------------------*/
/* ###########################  START CUSTOMIZATION PART  ########################### */
static void ug96_modem_init(atcustom_modem_context_t *p_modem_ctxt);
static void ug96_modem_reset(atcustom_modem_context_t *p_modem_ctxt);
static void reinitSyntaxAutomaton_ug96(void);
static void reset_variables_ug96(void);
static void init_ug96_qiurc_dnsgip(void);
static void socketHeaderRX_reset(void);
static void SocketHeaderRX_addChar(ug96_TYPE_CHAR_t *rxchar);
static uint16_t SocketHeaderRX_getSize(void);

/* ###########################  END CUSTOMIZATION PART  ########################### */

/* Functions Definition ------------------------------------------------------*/
void ATCustom_UG96_init(atparser_context_t *p_atp_ctxt)
{
  /* Commands Look-up table */
  static const atcustom_LUT_t ATCMD_UG96_LUT[] =
  {
    /* cmd enum - cmd string - cmd timeout (in ms) - build cmd ftion - analyze cmd ftion */
    {CMD_AT,             "",             UG96_AT_TIMEOUT,       fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_OK,          "OK",           UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CONNECT,     "CONNECT",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_RING,        "RING",         UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_CARRIER,  "NO CARRIER",   UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_ERROR,       "ERROR",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_UG96},
    {CMD_AT_NO_DIALTONE, "NO DIALTONE",  UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_BUSY,        "BUSY",         UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_ANSWER,   "NO ANSWER",    UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CME_ERROR,   "+CME ERROR",   UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_UG96},
    {CMD_AT_CMS_ERROR,   "+CMS ERROR",   UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CmsErr},

    /* GENERIC MODEM commands */
    {CMD_AT_CGMI,        "+CGMI",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMI},
    {CMD_AT_CGMM,        "+CGMM",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMM},
    {CMD_AT_CGMR,        "+CGMR",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMR},
    {CMD_AT_CGSN,        "+CGSN",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_CGSN_UG96,  fRspAnalyze_CGSN},
    {CMD_AT_GSN,         "+GSN",         UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_GSN},
    {CMD_AT_CIMI,        "+CIMI",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CIMI},
    {CMD_AT_CEER,        "+CEER",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEER},
    {CMD_AT_CMEE,        "+CMEE",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_CMEE,       fRspAnalyze_None},
    {CMD_AT_CPIN,        "+CPIN",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_CPIN,       fRspAnalyze_CPIN_UG96},
    {CMD_AT_CFUN,        "+CFUN",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_CFUN,       fRspAnalyze_CFUN_UG96},
    {CMD_AT_COPS,        "+COPS",        UG96_COPS_TIMEOUT,     fCmdBuild_COPS,       fRspAnalyze_COPS},
    {CMD_AT_CNUM,        "+CNUM",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CNUM},
    {CMD_AT_CGATT,       "+CGATT",       UG96_CGATT_TIMEOUT,    fCmdBuild_CGATT,      fRspAnalyze_CGATT},
    {CMD_AT_CGPADDR,     "+CGPADDR",     UG96_DEFAULT_TIMEOUT,  fCmdBuild_CGPADDR,    fRspAnalyze_CGPADDR},
    {CMD_AT_CREG,        "+CREG",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_CREG,       fRspAnalyze_CREG},
    {CMD_AT_CGREG,       "+CGREG",       UG96_DEFAULT_TIMEOUT,  fCmdBuild_CGREG,      fRspAnalyze_CGREG},
    {CMD_AT_CGEREP,      "+CGEREP",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_CGEREP,     fRspAnalyze_None},
    {CMD_AT_CGEV,        "+CGEV",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGEV},
    {CMD_AT_CSQ,         "+CSQ",         UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CSQ},
    {CMD_AT_CGDCONT,     "+CGDCONT",     UG96_DEFAULT_TIMEOUT,  fCmdBuild_CGDCONT,    fRspAnalyze_None},
    {CMD_AT_CGACT,       "+CGACT",       UG96_CGACT_TIMEOUT,    fCmdBuild_CGACT,      fRspAnalyze_None},
    {CMD_AT_CGDATA,      "+CGDATA",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_CGDATA,     fRspAnalyze_None},
    {CMD_ATD,            "D",            UG96_DEFAULT_TIMEOUT,  fCmdBuild_ATD_UG96,   fRspAnalyze_None},
    {CMD_ATE,            "E",            UG96_DEFAULT_TIMEOUT,  fCmdBuild_ATE,        fRspAnalyze_None},
    {CMD_ATH,            "H",            UG96_ATH_TIMEOUT,      fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATO,            "O",            UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATV,            "V",            UG96_DEFAULT_TIMEOUT,  fCmdBuild_ATV,        fRspAnalyze_None},
    {CMD_ATX,            "X",            UG96_DEFAULT_TIMEOUT,  fCmdBuild_ATX,        fRspAnalyze_None},
    {CMD_AT_ESC_CMD,     "+++",          UG96_ESCAPE_TIMEOUT,   fCmdBuild_ESCAPE_CMD, fRspAnalyze_None},
    {CMD_AT_IPR,         "+IPR",         UG96_DEFAULT_TIMEOUT,  fCmdBuild_IPR,        fRspAnalyze_IPR},
    {CMD_AT_IFC,         "+IFC",         UG96_DEFAULT_TIMEOUT,  fCmdBuild_IFC,        fRspAnalyze_None},
    {CMD_AT_AND_W,       "&W",           UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_DIRECT_CMD,  "",             UG96_DEFAULT_TIMEOUT,  fCmdBuild_DIRECT_CMD, fRspAnalyze_DIRECT_CMD},

    /* MODEM SPECIFIC COMMANDS */
    {CMD_AT_QPOWD,       "+QPOWD",       UG96_DEFAULT_TIMEOUT,  fCmdBuild_QPOWD_UG96, fRspAnalyze_None},
    {CMD_AT_QCFG,        "+QCFG",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_QCFG_UG96,  fRspAnalyze_QCFG_UG96},
    {CMD_AT_QIND,        "+QIND",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_QIND_UG96},
    {CMD_AT_QUSIM,       "+QUSIM",       UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_QUSIM_UG96},
    {CMD_AT_QICSGP,      "+QICSGP",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_QICSGP_UG96,   fRspAnalyze_None},
    {CMD_AT_QIURC,       "+QIURC",       UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QIURC_UG96},
    {CMD_AT_SOCKET_PROMPT, "> ",         UG96_SOCKET_PROMPT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_SEND_OK,      "SEND OK",     UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_SEND_FAIL,    "SEND FAIL",   UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_QIDNSCFG,     "+QIDNSCFG",   UG96_DEFAULT_TIMEOUT,  fCmdBuild_QIDNSCFG_UG96, fRspAnalyze_None},
    {CMD_AT_QIDNSGIP,     "+QIDNSGIP",   UG96_QIDNSGIP_TIMEOUT, fCmdBuild_QIDNSGIP_UG96, fRspAnalyze_None},
    {CMD_AT_QCCID,        "+QCCID",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QCCID_UG96},

    /* MODEM SPECIFIC COMMANDS USED FOR SOCKET MODE */
    {CMD_AT_QIACT,       "+QIACT",       UG96_QIACT_TIMEOUT,    fCmdBuild_QIACT_UG96,   fRspAnalyze_QIACT_UG96},
   /* {CMD_AT_QIDEACT,     "+QIDEACT",     UG96_QIDEACT_TIMEOUT,  fCmdBuild_QIDEACT_UG96, fRspAnalyze_QIDEACT_UG96}, */
    {CMD_AT_QIOPEN,      "+QIOPEN",      UG96_QIOPEN_TIMEOUT,   fCmdBuild_QIOPEN_UG96, fRspAnalyze_QIOPEN_UG96},
    {CMD_AT_QICLOSE,     "+QICLOSE",     UG96_QICLOSE_TIMEOUT,  fCmdBuild_QICLOSE_UG96, fRspAnalyze_None},
    {CMD_AT_QISEND,      "+QISEND",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_QISEND_UG96, fRspAnalyze_None},
    {CMD_AT_QISEND_WRITE_DATA,  "",      UG96_DEFAULT_TIMEOUT,  fCmdBuild_QISEND_WRITE_DATA_UG96, fRspAnalyze_None},
    {CMD_AT_QIRD,        "+QIRD",        UG96_DEFAULT_TIMEOUT,  fCmdBuild_QIRD_UG96, fRspAnalyze_QIRD_UG96},
    {CMD_AT_QISTATE,     "+QISTATE",     UG96_DEFAULT_TIMEOUT,  fCmdBuild_QISTATE_UG96, fRspAnalyze_QISTATE_UG96},
    {CMD_AT_QPING,        "+QPING",      UG96_QPING_TIMEOUT,    fCmdBuild_QPING_UG96,    fRspAnalyze_QPING_UG96},

    /* MODEM SPECIFIC EVENTS */
    {CMD_AT_WAIT_EVENT,     "",          UG96_DEFAULT_TIMEOUT,        fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_BOOT_EVENT,     "",          UG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_RDY_EVENT,      "RDY",       UG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
    /* {CMD_AT_SIMREADY_EVENT, "",          UG96_SIMREADY_TIMEOUT,       fCmdBuild_NoParams,   fRspAnalyze_None}, */
  };
#define size_ATCMD_UG96_LUT ((uint16_t) (sizeof (ATCMD_UG96_LUT) / sizeof (atcustom_LUT_t)))

  /* common init */
  ug96_modem_init(&UG96_ctxt);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  UG96_ctxt.modem_LUT_size = size_ATCMD_UG96_LUT;
  UG96_ctxt.p_modem_LUT = (const atcustom_LUT_t *)ATCMD_UG96_LUT;

  /* override default termination string for AT command: <CR> */
  (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->endstr, "\r");

  /* ###########################  END CUSTOMIZATION PART  ########################### */
}

uint8_t ATCustom_UG96_checkEndOfMsgCallback(uint8_t rxChar)
{
  uint8_t last_char = 0U;

  /* static variables */
  static const uint8_t QIRD_string[] = "+QIRD";
  static uint8_t QIRD_Counter = 0U;

  switch (UG96_ctxt.state_SyntaxAutomaton)
  {
    case WAITING_FOR_INIT_CR:
    {
      /* waiting for first valid <CR>, char received before are considered as trash */
      if ((AT_CHAR_t)('\r') == rxChar)
      {
        /* current     : xxxxx<CR>
        *  command format : <CR><LF>xxxxxxxx<CR><LF>
        *  waiting for : <LF>
        */
        UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
      }
      break;
    }

    case WAITING_FOR_CR:
    {
      if ((AT_CHAR_t)('\r') == rxChar)
      {
        /* current     : xxxxx<CR>
        *  command format : <CR><LF>xxxxxxxx<CR><LF>
        *  waiting for : <LF>
        */
        UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
      }
      break;
    }

    case WAITING_FOR_LF:
    {
      /* waiting for <LF> */
      if ((AT_CHAR_t)('\n') == rxChar)
      {
        /*  current        : <CR><LF>
        *   command format : <CR><LF>xxxxxxxx<CR><LF>
        *   waiting for    : x or <CR>
        */
        UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_FIRST_CHAR;
        last_char = 1U;
        QIRD_Counter = 0U;
      }
      break;
    }

    case WAITING_FOR_FIRST_CHAR:
    {
      if (UG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_waiting_header)
      {
        /* Socket Data RX - waiting for Header: we are waiting for +QIRD
        *
        * <CR><LF>+QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
        *          ^- waiting this string
        */
        if (rxChar == QIRD_string[QIRD_Counter])
        {
          QIRD_Counter++;
          if (QIRD_Counter == (uint8_t) strlen((const ug96_TYPE_CHAR_t *)QIRD_string))
          {
            /* +QIRD detected, next step */
            socketHeaderRX_reset();
            UG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_header;
          }
        }
      }

      /* NOTE:
       * if we are in socket_RxData_state = SocketRxDataState_waiting_header, we are waiting for +QIRD (test above)
       * but if we receive another message, we need to evacuate it without modifying socket_RxData_state
       * That's why we are nt using "else if" here, if <CR> if received before +QIND, it means that we have received
       * something else
       */
      if (UG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_header)
      {
        /* Socket Data RX - Header received: we are waiting for second <CR>
        *
        * <CR><LF>+QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
        *                    ^- waiting this <CR>
        */
        if ((AT_CHAR_t)('\r') == rxChar)
        {
          /* second <CR> detected, we have received data header
          *  now waiting for <LF>, then start to receive socket datas
          *  Verify that size received in header is the expected one
          */
          uint16_t size_from_header = SocketHeaderRX_getSize();
          if (UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size != size_from_header)
          {
            /* update buffer size received - should not happen */
            UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size = size_from_header;
          }
          UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
          UG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_data;
        }
        else if ((rxChar >= (AT_CHAR_t)('0')) && (rxChar <= (AT_CHAR_t)('9')))
        {
          /* receiving size of data in header */
          SocketHeaderRX_addChar((ug96_TYPE_CHAR_t *)&rxChar);
        }
        else if (rxChar == (AT_CHAR_t)(','))
        {
          /* receiving data field separator in header: +QIRD: 4,"10.7.76.34",7678
           *  data size field has been receied, now ignore all chars until <CR><LF>
           *  additonal fields (remote IP address and port) will be analyzed later
           */
          SocketHeaderDataRx_Cpt_Complete = 1U;
        }
        else {/* nothing to do */ }
      }
      else if (UG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_data)
      {
        /* receiving socket data: do not analyze char, just count expected size
        *
        * <CR><LF>+QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
        *.                          ^- start to read data: count char
        */
        UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_SOCKET_DATA;
        UG96_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
        /* check if full buffer has been received */
        if (UG96_ctxt.socket_ctxt.socket_rx_count_bytes_received == UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size)
        {
          UG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
          UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
        }
      }
      /* waiting for <CR> or x */
      else if ((AT_CHAR_t)('\r') == rxChar)
      {
        /*   current        : <CR>
        *   command format : <CR><LF>xxxxxxxx<CR><LF>
        *   waiting for    : <LF>
        */
        UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
      }
      else {/* nothing to do */ }
      break;
    }

    case WAITING_FOR_SOCKET_DATA:
      UG96_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
      /* check if full buffer has been received */
      if (UG96_ctxt.socket_ctxt.socket_rx_count_bytes_received == UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size)
      {
        UG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
        UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
      }
      break;

    default:
      /* should not happen */
      break;
  }

  /* ###########################  START CUSTOMIZATION PART  ######################### */
  /* if modem does not use standard syntax or has some specificities, replace previous
  *  function by a custom function
  */
  if (last_char == 0U)
  {
    /* UG96 special cases
    *
    *  SOCKET MODE: when sending DATA using AT+QISEND, we are waiting for socket prompt "<CR><LF>> "
    *               before to send DATA. Then we should receive "OK<CR><LF>".
    */

    if (UG96_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
    {
      switch (UG96_ctxt.socket_ctxt.socket_send_state)
      {
        case SocketSendState_WaitingPrompt1st_greaterthan:
        {
          /* detecting socket prompt first char: "greater than" */
          if ((AT_CHAR_t)('>') == rxChar)
          {
            UG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt2nd_space;
          }
          break;
        }

        case SocketSendState_WaitingPrompt2nd_space:
        {
          /* detecting socket prompt second char: "space" */
          if ((AT_CHAR_t)(' ') == rxChar)
          {
            UG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_Prompt_Received;
            last_char = 1U;
          }
          else
          {
            /* if char iommediatly after "greater than" is not a "space", reinit state */
            UG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
          }
          break;
        }

        default:
          break;
      }
    }
  }

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (last_char);
}

at_status_t ATCustom_UG96_getCmd(atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  /* static variables */
  static at_bool_t ug96_flow_control_forced = AT_FALSE;

  /* local variables */
  at_status_t retval = ATSTATUS_OK;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  PrintAPI("enter ATCustom_UG96_getCmd() for SID %d", curSID)

  /* retrieve parameters from SID command (will update SID_ctxt) */
  if (atcm_retrieve_SID_parameters(&UG96_ctxt, p_atp_ctxt) != ATSTATUS_OK)
  {
    return (ATSTATUS_ERROR);
  }

  /* new command: reset command context */
  atcm_reset_CMD_context(&UG96_ctxt.CMD_ctxt);

  /* For each SID, athe sequence of AT commands to send id defined (it can be dynamic)
  * Determine and prepare the next command to send for this SID
  */

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  if (curSID == (at_msg_t) SID_CS_CHECK_CNX)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_MODEM_CONFIG)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if ((curSID == (at_msg_t) SID_CS_POWER_ON) ||
           (curSID == (at_msg_t) SID_CS_RESET))
  {
    uint8_t at_max_retries = 10U; /* maximum nbr of AT send to establish synchro if autobaud */
    uint8_t start_sequence_step = at_max_retries + 3U;

    /* POWER_ON and RESET are almost the same, specific differences are managed case by case */

    /* for reset, only HW reset is supported */
    if ((curSID == (at_msg_t) SID_CS_RESET) &&
        (UG96_ctxt.SID_ctxt.reset_type != CS_RESET_HW))
    {
      PrintErr("Reset type (%d) not supported", UG96_ctxt.SID_ctxt.reset_type)
      retval = ATSTATUS_ERROR;
    }
    else
    {
      if (p_atp_ctxt->step == 0U)
      {
        /* reset modem specific variables */
        reset_variables_ug96();

        /* check if RDY has been received */
        if (UG96_ctxt.persist.modem_at_ready == AT_TRUE)
        {
          PrintINFO("Modem START indication already received, continue init sequence...")
          /* now reset modem_at_ready (in case of reception of reset indication) */
          UG96_ctxt.persist.modem_at_ready = AT_FALSE;
          /* if yes, go to next step: jump to POWER ON sequence step */
          p_atp_ctxt->step = start_sequence_step;

          if (curSID == (at_msg_t) SID_CS_RESET)
          {
            /* reinit context for reset case */
            ug96_modem_reset(&UG96_ctxt);
          }
        }
        else
        {
          if (curSID == (at_msg_t) SID_CS_RESET)
          {
            /* reinit context for reset case */
            ug96_modem_reset(&UG96_ctxt);
          }

          PrintINFO("Modem START indication not received yet, waiting for it...")
          /* wait for RDY (if not received, this is not an error, see below with autobaud) */
          atcm_program_TEMPO(p_atp_ctxt, UG96_RDY_TIMEOUT, INTERMEDIATE_CMD);
        }
      }
      else if (p_atp_ctxt->step == 1U)
      {
        /*  force flow control as it can explain why RDY has not been received */
        ug96_flow_control_forced = AT_TRUE;
        atcm_program_AT_CMD_ANSWER_OPTIONAL(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
      }
      /* For power ON sequence, if modem is not configured, baud rate can be set to AUTO (ie IPR = 0)
      *  Try to send AT many times (10) until modem answer is received (ie auto baud configuration is done)
      *  Then we will force baud rate to nominal value with AT+IPR command
      */
      else if ((p_atp_ctxt->step >= 2U) && (p_atp_ctxt->step <= (at_max_retries + 1U)))
      {
        if (UG96_ctxt.persist.modem_at_ready == AT_TRUE)
        {
          PrintDBG("RDY has been received, proceed to normal sequence")
          /* RDY has been received during tempo */
          UG96_ctxt.persist.modem_at_ready = AT_FALSE;

          /* go to next step: jump to POWER ON sequence step */
          p_atp_ctxt->step = start_sequence_step;
        }
        else if (ug96_shared.boot_synchro == AT_FALSE)
        {
          PrintDBG("test connection [try number %d] ", p_atp_ctxt->step)
          /* check modem connection */
          atcm_program_AT_CMD_ANSWER_OPTIONAL(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, INTERMEDIATE_CMD);
        }
        else
        {
          PrintDBG("modem synchro established, proceed to normal power sequence")
          /* need to set IPR cmd and save */
          ug96_shared.change_ipr_baud_rate = AT_TRUE;

          /* go to next step: jump to POWER ON sequence step */
          p_atp_ctxt->step = start_sequence_step;
        }
      }
      else if (p_atp_ctxt->step == (at_max_retries + 2U))
      {
        /* error, impossible to synchronize with modem */
        PrintErr("Impossible to sync with modem")
        retval = ATSTATUS_ERROR;
      }
      else
      {
        /* ignore case > at_max_retries + 1U (fall in start_sequence_step below) */
      }

      /* start power ON sequence here */
      if (p_atp_ctxt->step == start_sequence_step)
      {
        /*  set flow control to be sure that config is correct */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 1U))
      {
        /* disable echo */
        UG96_ctxt.CMD_ctxt.command_echo = AT_FALSE;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATE, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 2U))
      {
        /* request detailled error report */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CMEE, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 3U))
      {
        /* enable full response format */
        UG96_ctxt.CMD_ctxt.dce_full_resp_format = AT_TRUE;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATV, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 4U))
      {
        /* check flow control */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 5U))
      {
        /* Read baud rate settings */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_IPR, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 6U))
      {
        /* power on with AT+CFUN=0 */
        UG96_ctxt.CMD_ctxt.cfun_value = 0U;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 7U))
      {
        /* check URC state */
        ug96_shared.QCFG_received_param_name = QCFG_unknown;
        ug96_shared.QCFG_received_param_value = QCFG_unknown;
        ug96_shared.QCFG_command_param = QCFG_stateurc_check;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, INTERMEDIATE_CMD);
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 8U))
      {
        if ((ug96_shared.QCFG_received_param_name == QCFG_stateurc_check) &&
            (ug96_shared.QCFG_received_param_value == QCFG_stateurc_disable))
        {
          /* enable URC state */
          ug96_shared.QCFG_received_param_name = QCFG_unknown;
          ug96_shared.QCFG_received_param_value = QCFG_unknown;
          ug96_shared.QCFG_command_param = QCFG_stateurc_enable;
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, INTERMEDIATE_CMD);
        }
        else
        {
          /* jump to next step */
          atcm_program_SKIP_CMD(p_atp_ctxt);
        }
      }
      /* do we need to set IPR (baud rate) ? */
      else if (p_atp_ctxt->step == (start_sequence_step + 9U))
      {
        if (ug96_shared.change_ipr_baud_rate == AT_TRUE)
        {
          /* set requested baud rate */
          UG96_ctxt.CMD_ctxt.baud_rate = MODEM_UART_BAUDRATE;
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IPR, INTERMEDIATE_CMD);
        }
        else
        {
          /* skip */
          atcm_program_SKIP_CMD(p_atp_ctxt);

        }
      }
      else if (p_atp_ctxt->step == (start_sequence_step + 10U))
      {
        /* do we have change modem settings ? */
        if ((ug96_shared.change_ipr_baud_rate == AT_TRUE) ||
            (ug96_flow_control_forced == AT_TRUE))
        {
          /* save parameters to NV */
          ug96_shared.change_ipr_baud_rate = AT_FALSE;
          ug96_flow_control_forced = AT_FALSE;
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_AND_W, FINAL_CMD);
        }
        else
        {
          atcm_program_NO_MORE_CMD(p_atp_ctxt);
        }

      }
      else if (p_atp_ctxt->step > (start_sequence_step + 10U))
      {
        /* error, invalid step */
        retval = ATSTATUS_ERROR;
      }
      else
      {
        /* ignore */
      }
    }
  }
  else if (curSID == (at_msg_t) SID_CS_POWER_OFF)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* software power off for this modem
       * hardware power off will just reset modem GPIOs
       */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QPOWD, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_INIT_MODEM)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* CFUN parameters here are coming from user settings in CS_init_modem() */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* check if +QIND: PB DONE has been received */
      if (ug96_shared.sim_card_ready == AT_TRUE)
      {
        PrintINFO("Modem SIM 'PB DONE' indication already received, continue init sequence...")
        ug96_shared.sim_card_ready = AT_FALSE;
        /* go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        PrintINFO("***** wait a few seconds for optional 'PB DONE' *****")
        /* wait for a few seconds */
        atcm_program_TEMPO(p_atp_ctxt, UG96_SIMREADY_TIMEOUT, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* check is CPIN is requested */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 3U)
    {
      if (UG96_ctxt.persist.sim_pin_code_ready == AT_FALSE)
      {
        if (strlen((const ug96_TYPE_CHAR_t *)&UG96_ctxt.SID_ctxt.modem_init.pincode.pincode) != 0U)
        {
          /* send PIN value */
          PrintINFO("CPIN required, we send user value to modem")
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
          /* will wait for modem message 'PB DONE' indication */
          ug96_shared.sim_card_ready = AT_FALSE;
        }
        else
        {
          /* no PIN provided by user */
          PrintINFO("CPIN required but not provided by user")
          retval = ATSTATUS_ERROR;
        }
      }
      else
      {
        PrintINFO("CPIN not required")
        /* no PIN required, skip cmd and go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
        /* will not wait for modem message 'PB DONE' indication, do as we had receive it */
        ug96_shared.sim_card_ready = AT_TRUE;
      }
    }
    else if (p_atp_ctxt->step == 4U)
    {
      /* check PDP context parameters */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGDCONT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_DEVICE_INFO)
  {
    if (p_atp_ctxt->step == 0U)
    {
      switch (UG96_ctxt.SID_ctxt.device_info->field_requested)
      {
        case CS_DIF_MANUF_NAME_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMI, FINAL_CMD);
          break;

        case CS_DIF_MODEL_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMM, FINAL_CMD);
          break;

        case CS_DIF_REV_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, FINAL_CMD);
          break;

        case CS_DIF_SN_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMEI_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_GSN, FINAL_CMD);
          break;

        case CS_DIF_IMSI_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CIMI, FINAL_CMD);
          break;

        case CS_DIF_PHONE_NBR_PRESENT:
          /* not AT+CNUM not supported by UG96 */
          retval = ATSTATUS_ERROR;
          break;

        case CS_DIF_ICCID_PRESENT:
          atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_QCCID, FINAL_CMD);
          break;

        default:
          /* error, invalid step */
          retval = ATSTATUS_ERROR;
          break;
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_SIGNAL_QUALITY)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CSQ, FINAL_CMD);
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_ATTACHSTATUS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_REGISTER_NET)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* check if actual registration status is the expected one */
      const CS_OperatorSelector_t *operatorSelect = &(UG96_ctxt.SID_ctxt.write_operator_infos);
      if (UG96_ctxt.SID_ctxt.read_operator_infos.mode != operatorSelect->mode)
      {
        /* write registration status */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
      }
      else
      {
        /* skip this step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 3U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGREG, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_NETSTATUS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGREG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* read registration status */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      CS_UrcEvent_t urcEvent = UG96_ctxt.SID_ctxt.urcEvent;

      /* is an event linked to CREG or CGREG ? */
      if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
      {
        (void) atcm_subscribe_net_event(&UG96_ctxt, p_atp_ctxt);
      }
      else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
      {
        /* no command to monitor signal quality with URC in UG96 */
        PrintErr("Signal quality monitoring no supported by this modem")
        retval = ATSTATUS_ERROR;
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      CS_UrcEvent_t urcEvent = UG96_ctxt.SID_ctxt.urcEvent;

      /* is an event linked to CREG or CGREG ? */
      if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
      {
        (void) atcm_unsubscribe_net_event(&UG96_ctxt, p_atp_ctxt);
      }
      else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
      {
        /* no command to monitor signal quality with URC in UG96 */
        PrintErr("Signal quality monitoring no supported by this modem")
        retval = ATSTATUS_ERROR;
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_REGISTER_PDN_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      if (UG96_ctxt.persist.urc_subscript_pdn_event == CELLULAR_FALSE)
      {
        /* set event as suscribed */
        UG96_ctxt.persist.urc_subscript_pdn_event = CELLULAR_TRUE;

        /* request PDN events */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGEREP, FINAL_CMD);
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DEREGISTER_PDN_EVENT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      if (UG96_ctxt.persist.urc_subscript_pdn_event == CELLULAR_TRUE)
      {
        /* set event as unsuscribed */
        UG96_ctxt.persist.urc_subscript_pdn_event = CELLULAR_FALSE;

        /* request PDN events */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGEREP, FINAL_CMD);
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_ATTACH_PS_DOMAIN)
  {
    if (p_atp_ctxt->step == 0U)
    {
      UG96_ctxt.CMD_ctxt.cgatt_write_cmd_param = CGATT_ATTACHED;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_DETACH_PS_DOMAIN)
  {
    if (p_atp_ctxt->step == 0U)
    {
      UG96_ctxt.CMD_ctxt.cgatt_write_cmd_param = CGATT_DETACHED;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_ACTIVATE_PDN)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    if (p_atp_ctxt->step == 0U)
    {
      /* ckeck PDN state */
      ug96_shared.pdn_already_active = AT_FALSE;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QIACT, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* PDN activation */
      if (ug96_shared.pdn_already_active == AT_TRUE)
      {
        /* PDN already active - exit */
        PrintINFO("Skip PDN activation (already active)")
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        /* request PDN activation */
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIACT, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* ckeck PDN state */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QIACT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#else
    /* DATA MODE*/
    if (p_atp_ctxt->step == 0U)
    {
      /* PDN activation */
      UG96_ctxt.CMD_ctxt.pdn_state = PDN_STATE_ACTIVATE;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGACT, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* get IP address */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 2U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGDATA, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == (at_msg_t) SID_CS_DEACTIVATE_PDN)
  {
    /* not implemented yet */
    retval = ATSTATUS_ERROR;
  }
  else if (curSID == (at_msg_t) SID_CS_DEFINE_PDN)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    if (p_atp_ctxt->step == 0U)
    {
      ug96_shared.QICGSP_config_command = AT_TRUE;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICSGP, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      ug96_shared.QICGSP_config_command = AT_FALSE;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICSGP, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#else
    /* DATA MODE
     * NOTE: UG96 returns an error to CGDCONT if SIM error (in our case if CFUN = 0)
     *       this is the reason why we are starting radio before
     */
    if (p_atp_ctxt->step == 0U)
    {
      ug96_shared.sim_card_ready = AT_FALSE;
      /* CFUN parameters here are coming from user settings in CS_init_modem() */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* check if +QIND: PB DONE has been received */
      if (ug96_shared.sim_card_ready == AT_TRUE)
      {
        PrintINFO("Modem SIM 'PB DONE' indication already received, continue init sequence...")
        /* go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        PrintINFO("***** wait a few seconds for optional 'PB DONE' *****")
        /* wait for a few seconds */
        atcm_program_TEMPO(p_atp_ctxt, UG96_SIMREADY_TIMEOUT, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGDCONT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == (at_msg_t) SID_CS_SET_DEFAULT_PDN)
  {
    /* nothing to do here
     * Indeed, default PDN has been saved automatically during analysis of SID command
     * cf function: atcm_retrieve_SID_parameters()
     */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else if (curSID == (at_msg_t) SID_CS_GET_IP_ADDRESS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* get IP address */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      /* SOCKET MODE */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QIACT, FINAL_CMD);
#else
      /* DATA MODE*/
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, FINAL_CMD);
#endif /* USE_SOCKETS_TYPE */
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DIAL_COMMAND)
  {
    /* SOCKET CONNECTION FOR COMMAND DATA MODE */
    if (p_atp_ctxt->step == 0U)
    {
      /* reserve a modem CID for this socket_handle */
      ug96_shared.QIOPEN_current_socket_connected = 0U;
      socket_handle_t sockHandle = UG96_ctxt.socket_ctxt.socket_info->socket_handle;
      (void) atcm_socket_reserve_modem_cid(&UG96_ctxt, sockHandle);
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIOPEN, INTERMEDIATE_CMD);
      PrintINFO("For Client Socket Handle=%ld : MODEM CID affected=%d",
                sockHandle,
                UG96_ctxt.persist.socket[sockHandle].socket_connId_value)

    }
    else if (p_atp_ctxt->step == 1U)
    {
      if (ug96_shared.QIOPEN_current_socket_connected == 0U)
      {
        /* Waiting for +QIOPEN urc indicating that socket is open */
        atcm_program_TEMPO(p_atp_ctxt, UG96_QIOPEN_TIMEOUT, INTERMEDIATE_CMD);
      }
      else
      {
        /* socket opened */
        ug96_shared.QIOPEN_waiting = AT_FALSE;
        /* socket is connected */
        (void) atcm_socket_set_connected(&UG96_ctxt, UG96_ctxt.socket_ctxt.socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else  if (p_atp_ctxt->step == 2U)
    {
      if (ug96_shared.QIOPEN_current_socket_connected == 0U)
      {
        /* QIOPEN NOT RECEIVED,
        *  cf TCP/IP AT Commands Manual V1.0, paragraph 2.1.4 - 3/
        *  "if the URC cannot be received within 150 seconds, AT+QICLOSE should be used to close
        *   the socket".
        *
        *  then we will have to return an error to cellular service !!! (see next step)
        */
        ug96_shared.QIOPEN_waiting = AT_FALSE;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICLOSE, INTERMEDIATE_CMD);
      }
      else
      {
        /* socket opened */
        ug96_shared.QIOPEN_waiting = AT_FALSE;
        /* socket is connected */
        (void) atcm_socket_set_connected(&UG96_ctxt, UG96_ctxt.socket_ctxt.socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else  if (p_atp_ctxt->step == 3U)
    {
      /* if we fall here, it means we have send CMD_AT_QICLOSE on previous step
      *  now inform cellular service that opening has failed => return an error
      */
      /* release the modem CID for this socket_handle */
      (void) atcm_socket_release_modem_cid(&UG96_ctxt, UG96_ctxt.socket_ctxt.socket_info->socket_handle);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SEND_DATA)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* Check data size to send */
      if (UG96_ctxt.SID_ctxt.socketSendData_struct.buffer_size > MODEM_MAX_SOCKET_TX_DATA_SIZE)
      {
        PrintErr("Data size to send %ld exceed maximum size %ld",
                 UG96_ctxt.SID_ctxt.socketSendData_struct.buffer_size,
                 MODEM_MAX_SOCKET_TX_DATA_SIZE)
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
        retval = ATSTATUS_ERROR;
      }
      else
      {
        UG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QISEND, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* waiting for socket prompt: "<CR><LF>> " */
      if (UG96_ctxt.socket_ctxt.socket_send_state == SocketSendState_Prompt_Received)
      {
        PrintDBG("SOCKET PROMPT ALREADY RECEIVED")
        /* go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        PrintDBG("WAITING FOR SOCKET PROMPT")
        atcm_program_WAIT_EVENT(p_atp_ctxt, UG96_SOCKET_PROMPT_TIMEOUT, INTERMEDIATE_CMD);
      }
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* socket prompt received, send DATA */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_QISEND_WRITE_DATA, FINAL_CMD);

      /* reinit automaton to receive answer */
      reinitSyntaxAutomaton_ug96();
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if ((curSID == (at_msg_t) SID_CS_RECEIVE_DATA) ||
           (curSID == (at_msg_t) SID_CS_RECEIVE_DATA_FROM))
  {
    if (p_atp_ctxt->step == 0U)
    {
      UG96_ctxt.socket_ctxt.socket_receive_state = SocketRcvState_RequestSize;
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIRD, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* check that data size to receive is not null */
      if (UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size != 0U)
      {
        /* check that data size to receive does not exceed maximum size
        *  if it's the case, only request maximum size we can receive
        */
        if (UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size >
            UG96_ctxt.socket_ctxt.socketReceivedata.max_buffer_size)
        {
          PrintINFO("Data size available (%ld) exceed buffer maximum size (%ld)",
                    UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size,
                    UG96_ctxt.socket_ctxt.socketReceivedata.max_buffer_size)
          UG96_ctxt.socket_ctxt.socket_rx_expected_buf_size = UG96_ctxt.socket_ctxt.socketReceivedata.max_buffer_size;
        }

        /* receive datas */
        UG96_ctxt.socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Header;
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIRD, FINAL_CMD);
      }
      else
      {
        /* no datas to receive */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SOCKET_CLOSE)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* is socket connected ?
      * due to UG96 socket connection mechanism (waiting URC QIOPEN), we can fall here but socket
      * has been already closed if error occurs during connection
      */
      if (atcm_socket_is_connected(&UG96_ctxt, UG96_ctxt.socket_ctxt.socket_info->socket_handle) == AT_TRUE)
      {
        atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICLOSE, INTERMEDIATE_CMD);
      }
      else
      {
        /* release the modem CID for this socket_handle */
        (void) atcm_socket_release_modem_cid(&UG96_ctxt, UG96_ctxt.socket_ctxt.socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* release the modem CID for this socket_handle */
      (void) atcm_socket_release_modem_cid(&UG96_ctxt, UG96_ctxt.socket_ctxt.socket_info->socket_handle);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DATA_SUSPEND)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* wait for 1 second */
      atcm_program_TEMPO(p_atp_ctxt, 1000U, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* send escape sequence +++ (RAW command type)
      *  CONNECT expected before 1000 ms
      */
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_ESC_CMD, FINAL_CMD);
      reinitSyntaxAutomaton_ug96();
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SOCKET_CNX_STATUS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QISTATE, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DATA_RESUME)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATO, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DNS_REQ)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIDNSCFG, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 1U)
    {
      /* initialize +QIURC "dnsgip" parameters */
      init_ug96_qiurc_dnsgip();
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIDNSGIP, INTERMEDIATE_CMD);
    }
    else if (p_atp_ctxt->step == 2U)
    {
      /* do we have received a valid DNS response ? */
      if ((ug96_shared.QIURC_dnsgip_param.finished == AT_TRUE) && (ug96_shared.QIURC_dnsgip_param.error == 0U))
      {
        /* yes */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        /* not yet, waiting for DNS informations */
        atcm_program_TEMPO(p_atp_ctxt, UG96_QIDNSGIP_TIMEOUT, FINAL_CMD);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SUSBCRIBE_MODEM_EVENT)
  {
    /* nothing to do here
     * Indeed, default modem events subscribed havebeen saved automatically during analysis of SID command
     * cf function: atcm_retrieve_SID_parameters()
     */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else if (curSID == (at_msg_t) SID_CS_PING_IP_ADDRESS)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QPING, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DIRECT_CMD)
  {
    if (p_atp_ctxt->step == 0U)
    {
      atcm_program_AT_CMD(&UG96_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_DIRECT_CMD, FINAL_CMD);
      atcm_program_CMD_TIMEOUT(&UG96_ctxt, p_atp_ctxt, UG96_ctxt.SID_ctxt.direct_cmd_tx->cmd_timeout);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SIM_SELECT)
  {
    if (p_atp_ctxt->step == 0U)
    {
      /* select the SIM slot */
      if (atcm_select_hw_simslot(UG96_ctxt.persist.sim_selected) != ATSTATUS_OK)
      {
        retval = ATSTATUS_ERROR;
      }
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */
  else
  {
    PrintErr("Error, invalid command ID %d", curSID)
    retval = ATSTATUS_ERROR;
  }

  /* if no error, build the command to send */
  if (retval == ATSTATUS_OK)
  {
    retval = atcm_modem_build_cmd(&UG96_ctxt, p_atp_ctxt, p_ATcmdTimeout);
  }

  return (retval);
}

at_endmsg_t ATCustom_UG96_extractElement(atparser_context_t *p_atp_ctxt,
                                         const IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  at_endmsg_t retval_msg_end_detected = ATENDMSG_NO;
  uint8_t exit_loop = 0U;
  uint16_t idx, start_idx;
  uint16_t *p_parseIndex = &(element_infos->current_parse_idx);

  PrintAPI("enter ATCustom_UG96_extractElement()")
  PrintAPI("input message: size=%d ", p_msg_in->size)

  /* if this is beginning of message, check that message header is correct and jump over it */
  if (*p_parseIndex == 0U)
  {
    /* ###########################  START CUSTOMIZATION PART  ########################### */
    /* MODEM RESPONSE SYNTAX:
    * <CR><LF><response><CR><LF>
    *
    */
    start_idx = 0U;
    /* search initial <CR><LF> sequence (for robustness) */
    if ((p_msg_in->buffer[0] == (AT_CHAR_t)('\r')) && (p_msg_in->buffer[1] == (AT_CHAR_t)('\n')))
    {
      /* <CR><LF> sequence has been found, it is a command line */
      PrintDBG("cmd init sequence <CR><LF> found - break")
      *p_parseIndex = 2U;
      start_idx = 2U;
    }
    for (idx = start_idx; idx < (p_msg_in->size - 1U); idx++)
    {
      if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_QIRD) &&
          (UG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Payload) &&
          (UG96_ctxt.socket_ctxt.socket_RxData_state != SocketRxDataState_finished))
      {
        PrintDBG("receiving socket data (real size=%d)", SocketHeaderRX_getSize())
        element_infos->str_start_idx = 0U;
        element_infos->str_end_idx = (uint16_t) UG96_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        element_infos->str_size = (uint16_t) UG96_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        UG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_finished;
        return (ATENDMSG_YES);
      }
    }
    /* ###########################  END CUSTOMIZATION PART  ########################### */
  }

  element_infos->str_start_idx = *p_parseIndex;
  element_infos->str_end_idx = *p_parseIndex;
  element_infos->str_size = 0U;

  /* reach limit of input buffer ? (empty message received) */
  if (*p_parseIndex >= p_msg_in->size)
  {
    return (ATENDMSG_YES);
  }

  /* extract parameter from message */
  do
  {
    switch (p_msg_in->buffer[*p_parseIndex])
    {
      /* ###########################  START CUSTOMIZATION PART  ########################### */
      /* ----- test separators ----- */
      case ' ':
        if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CGDATA)
        {
          /* specific case for UG96 which answer CONNECT <value> even when ATX0 sent before */
          exit_loop = 1U;
        }
        else
        {
          element_infos->str_end_idx = *p_parseIndex;
          element_infos->str_size++;
        }
        break;

      case ':':
      case ',':
        exit_loop = 1U;
        break;

      /* ----- test end of message ----- */
      case '\r':
        exit_loop = 1U;
        retval_msg_end_detected = ATENDMSG_YES;
        break;

      default:
        /* increment end position */
        element_infos->str_end_idx = *p_parseIndex;
        element_infos->str_size++;
        break;
        /* ###########################  END CUSTOMIZATION PART  ########################### */
    }

    /* increment index */
    (*p_parseIndex)++;

    /* reach limit of input buffer ? */
    if (*p_parseIndex >= p_msg_in->size)
    {
      exit_loop = 1U;
      retval_msg_end_detected = ATENDMSG_YES;
    }
  } while (exit_loop != 1U);

  /* increase parameter rank */
  element_infos->param_rank = (element_infos->param_rank + 1U);

  return (retval_msg_end_detected);
}

at_action_rsp_t ATCustom_UG96_analyzeCmd(at_context_t *p_at_ctxt,
                                         const IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval ;

  PrintAPI("enter ATCustom_UG96_analyzeCmd()")

  /* search in LUT the ID corresponding to command received */
  if (ATSTATUS_OK != atcm_searchCmdInLUT(&UG96_ctxt, p_atp_ctxt, p_msg_in, element_infos))
  {
    /* if cmd_id not found in the LUT, may be it's a text line to analyze */

    /* 1st STEP: search in common modems commands

      NOTE: if this modem has a specific behaviour for one of the common commands, bypass this
      step and manage all command in the 2nd step
    */
    retval = atcm_check_text_line_cmd(&UG96_ctxt, p_at_ctxt, p_msg_in, element_infos);

    /* 2nd STEP: search in specific modems commands if not found at 1st STEP */
    if (retval == ATACTION_RSP_NO_ACTION)
    {
      switch (p_atp_ctxt->current_atcmd.id)
      {
        /* ###########################  START CUSTOMIZED PART  ########################### */
        case CMD_AT_QIRD:
          if (fRspAnalyze_QIRD_data_UG96(p_at_ctxt, &UG96_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
          {
            /* received a valid intermediate answer */
            retval = ATACTION_RSP_INTERMEDIATE;
          }
          break;

        case CMD_AT_QISTATE:
          if (fRspAnalyze_QISTATE_UG96(p_at_ctxt, &UG96_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
          {
            /* received a valid intermediate answer */
            retval = ATACTION_RSP_INTERMEDIATE;
          }
          break;

        /* ###########################  END CUSTOMIZED PART  ########################### */
        default:
          /* this is not one of modem common command, need to check if this is an answer to a modem's specific cmd */
          PrintINFO("receive an un-expected line... is it a text line ?")
          retval = ATACTION_RSP_IGNORED;
          break;
      }
    }

    /* we fall here when cmd_id not found in the LUT
    * 2 cases are possible:
    *  - this is a valid line: ATACTION_RSP_INTERMEDIATE
    *  - this is an invalid line: ATACTION_RSP_ERROR
    */
    return (retval);
  }

  /* cmd_id has been found in the LUT: determine next action */
  switch (element_infos->cmd_id_received)
  {
    case CMD_AT_OK:
      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DATA_SUSPEND)
      {
        PrintINFO("MODEM SWITCHES TO COMMAND MODE")
        atcm_set_modem_data_mode(&UG96_ctxt, AT_FALSE);
      }
      if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON) ||
          (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
      {
        if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT) ||
            (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_IFC))
        {
          ug96_shared.boot_synchro = AT_TRUE;
          PrintDBG("modem synchro established")
        }
        if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATE)
        {
          PrintDBG("Echo successfully disabled")
        }
      }
      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_PING_IP_ADDRESS)
      {
        PrintDBG("This is a valid PING request")
        atcm_validate_ping_request(&UG96_ctxt);
      }
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_RING:
    case CMD_AT_NO_CARRIER:
    case CMD_AT_NO_DIALTONE:
    case CMD_AT_BUSY:
    case CMD_AT_NO_ANSWER:
      /* VALUES NOT MANAGED IN CURRENT IMPLEMENTATION BECAUSE NOT EXPECTED */
      retval = ATACTION_RSP_ERROR;
      break;

    case CMD_AT_CONNECT:
      PrintINFO("MODEM SWITCHES TO DATA MODE")
      atcm_set_modem_data_mode(&UG96_ctxt, AT_TRUE);
      retval = (at_action_rsp_t)(ATACTION_RSP_FLAG_DATA_MODE | ATACTION_RSP_FRC_END);
      break;

    /* ###########################  START CUSTOMIZATION PART  ########################### */
    case CMD_AT_CREG:
      /* check if response received corresponds to the command we have send
      *  if not => this is an URC
      */
      if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      else
      {
        retval = ATACTION_RSP_URC_FORWARDED;
      }
      break;

    case CMD_AT_CGREG:
      /* check if response received corresponds to the command we have send
      *  if not => this is an URC
      */
      if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      else
      {
        retval = ATACTION_RSP_URC_FORWARDED;
      }
      break;

    case CMD_AT_RDY_EVENT:
      PrintDBG(" MODEM READY TO RECEIVE AT COMMANDS")
      /* modem is ready */
      UG96_ctxt.persist.modem_at_ready = AT_TRUE;

      /* if we were waiting for this event, we can continue the sequence */
      if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON) ||
          (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
      {
        /* UNLOCK the WAIT EVENT : as there are still some commands to send after, use CONTINUE */
        UG96_ctxt.persist.modem_at_ready = AT_FALSE;
        retval = ATACTION_RSP_FRC_CONTINUE;
      }
      else
      {
        retval = ATACTION_RSP_URC_IGNORED;
      }
      break;

    case CMD_AT_SOCKET_PROMPT:
      PrintINFO(" SOCKET PROMPT RECEIVED")
      /* if we were waiting for this event, we can continue the sequence */
      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SEND_DATA)
      {
        /* UNLOCK the WAIT EVENT */
        retval = ATACTION_RSP_FRC_END;
      }
      else
      {
        retval = ATACTION_RSP_URC_IGNORED;
      }
      break;

    case CMD_AT_SEND_OK:
      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SEND_DATA)
      {
        retval = ATACTION_RSP_FRC_END;
      }
      else
      {
        retval = ATACTION_RSP_ERROR;
      }
      break;

    case CMD_AT_SEND_FAIL:
      retval = ATACTION_RSP_ERROR;
      break;

    case CMD_AT_QIURC:
      /* retval will be override in analyze of +QUIRC content
      *  indeed, QIURC can be considered as an URC or a normal msg (for DNS request)
      */
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case CMD_AT_QIOPEN:
      /* now waiting for an URC  */
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case CMD_AT_QUSIM:
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case CMD_AT_QIND:
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case CMD_AT_CFUN:
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case CMD_AT_CPIN:
      PrintDBG(" SIM STATE RECEIVED")
      /* retval will be override in analyze of +CPIN content if needed
      */
      retval = ATACTION_RSP_URC_IGNORED;
      break;

    case CMD_AT_QCFG:
      retval = ATACTION_RSP_INTERMEDIATE;
      break;

    case CMD_AT_CGEV:
      retval = ATACTION_RSP_URC_FORWARDED;
      break;

    case CMD_AT_QPING:
      retval = ATACTION_RSP_URC_FORWARDED;
      break;

    /* ###########################  END CUSTOMIZATION PART  ########################### */

    case CMD_AT:
      retval = ATACTION_RSP_IGNORED;
      break;

    case CMD_AT_INVALID:
      retval = ATACTION_RSP_ERROR;
      break;

    case CMD_AT_ERROR:
      /* ERROR does not contains parameters, call the analyze function explicity */
      retval = fRspAnalyze_Error_UG96(p_at_ctxt, &UG96_ctxt, p_msg_in, element_infos);
      break;

    case CMD_AT_CME_ERROR:
    case CMD_AT_CMS_ERROR:
      /* do the analyze here because will not be called by parser */
      retval = fRspAnalyze_Error_UG96(p_at_ctxt, &UG96_ctxt, p_msg_in, element_infos);
      break;

    default:
      /* check if response received corresponds to the command we have send
      *  if not => this is an ERROR
      */
      if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      else if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
      {
        retval = ATACTION_RSP_IGNORED;
      }
      else
      {
        PrintINFO("UNEXPECTED MESSAGE RECEIVED")
        retval = ATACTION_RSP_IGNORED;
      }
      break;
  }

  return (retval);
}

at_action_rsp_t ATCustom_UG96_analyzeParam(at_context_t *p_at_ctxt,
                                           const IPC_RxMessage_t *p_msg_in,
                                           at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PrintAPI("enter ATCustom_UG96_analyzeParam()")

  /* analyse parameters of the command we received:
  * call the corresponding function from the LUT
  */
  retval = (atcm_get_CmdAnalyzeFunc(&UG96_ctxt, element_infos->cmd_id_received))(p_at_ctxt,
                                                                                 &UG96_ctxt,
                                                                                 p_msg_in,
                                                                                 element_infos);

  return (retval);
}

/* function called to finalize an AT command */
at_action_rsp_t ATCustom_UG96_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter ATCustom_UG96_terminateCmd()")

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* additional tests */
  if (UG96_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
  {
    /* special case for SID_CS_SEND_DATA
    * indeed, this function is called when an AT cmd is finished
    * but for AT+QISEND, it is called a 1st time when prompt is received
    * and a second time when data have been sent.
    */
    if (p_atp_ctxt->current_SID != (at_msg_t) SID_CS_SEND_DATA)
    {
      /* reset socket_send_state */
      UG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_No_Activity;
    }
  }

  if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATD) ||
      (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATO) ||
      (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CGDATA))
  {
    if (element_infos->cmd_id_received != (CMD_ID_t) CMD_AT_CONNECT)
    {
      PrintErr("expected CONNECT not received !!!")
      return (ATACTION_RSP_ERROR);
    }
    else
    {
      /* force last command (no command can be sent in DATA mode) */
      p_atp_ctxt->is_final_cmd = 1U;
      PrintDBG("CONNECT received")
    }
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */
  return (retval);
}

/* function called to finalize a SID */
at_status_t ATCustom_UG96_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PrintAPI("enter ATCustom_UG96_get_rsp()")

  /* prepare response for a SID - common part */
  retval = atcm_modem_get_rsp(&UG96_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* prepare response for a SID
  *  all specific behaviors for SID which are returning datas in rsp_buf have to be implemented here
  */
  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_DNS_REQ:
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_DNS_REQ,
                               (uint16_t) sizeof(ug96_shared.QIURC_dnsgip_param.hostIPaddr),
                               (void *)ug96_shared.QIURC_dnsgip_param.hostIPaddr) != DATAPACK_OK)
      {
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_POWER_OFF:
      /* reinit context for power off case */
      ug96_modem_reset(&UG96_ctxt);
      break;

    default:
      break;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  /* reset SID context */
  atcm_reset_SID_context(&UG96_ctxt.SID_ctxt);

  /* reset socket context */
  atcm_reset_SOCKET_context(&UG96_ctxt);

  return (retval);
}

at_status_t ATCustom_UG96_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PrintAPI("enter ATCustom_UG96_get_urc()")

  /* prepare response for an URC - common part */
  retval = atcm_modem_get_urc(&UG96_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* prepare response for an URC
  *  all specific behaviors for an URC have to be implemented here
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

at_status_t ATCustom_UG96_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PrintAPI("enter ATCustom_UG96_get_error()")

  /* prepare response when an error occured - common part */
  retval = atcm_modem_get_error(&UG96_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /*  prepare response when an error occured
  *  all specific behaviors for an error have to be implemented here
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/* Private function Definition -----------------------------------------------*/

/* UG96 modem init fonction
*  call common init function and then do actions specific to this modem
*/
static void ug96_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter ug96_modem_init")

  /* common init function (reset all contexts) */
  atcm_modem_init(p_modem_ctxt);

  /* modem specific actions if any */
}

/* UG96 modem reset fonction
*  call common reset function and then do actions specific to this modem
*/
static void ug96_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PrintAPI("enter ug96_modem_reset")

  /* common reset function (reset all contexts except SID) */
  atcm_modem_reset(p_modem_ctxt);

  /* modem specific actions if any */
}

static void reinitSyntaxAutomaton_ug96(void)
{
  UG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

static void reset_variables_ug96(void)
{
  /* Set default values of UG96 specific variables after SWITCH ON or RESET */
  ug96_shared.change_ipr_baud_rate = AT_FALSE;
  ug96_shared.boot_synchro = AT_FALSE;
  ug96_shared.sim_card_ready = AT_FALSE;
}

static void init_ug96_qiurc_dnsgip(void)
{
  ug96_shared.QIURC_dnsgip_param.finished = AT_FALSE;
  ug96_shared.QIURC_dnsgip_param.wait_header = AT_TRUE;
  ug96_shared.QIURC_dnsgip_param.error = 0U;
  ug96_shared.QIURC_dnsgip_param.ip_count = 0U;
  ug96_shared.QIURC_dnsgip_param.ttl = 0U;
  (void) memset((void *)ug96_shared.QIURC_dnsgip_param.hostIPaddr, 0, MAX_SIZE_IPADDR);
}

static void socketHeaderRX_reset(void)
{
  (void) memset((void *)SocketHeaderDataRx_Buf, 0, 4U);
  SocketHeaderDataRx_Cpt = 0U;
  SocketHeaderDataRx_Cpt_Complete = 0U;
}
static void SocketHeaderRX_addChar(ug96_TYPE_CHAR_t *rxchar)
{
  if ((SocketHeaderDataRx_Cpt_Complete == 0U) && (SocketHeaderDataRx_Cpt < 4U))
  {
    (void) memcpy((void *)&SocketHeaderDataRx_Buf[SocketHeaderDataRx_Cpt], (void *)rxchar, sizeof(char));
    SocketHeaderDataRx_Cpt++;
  }
}
static uint16_t SocketHeaderRX_getSize(void)
{
  uint16_t retval = (uint16_t) ATutil_convertStringToInt((uint8_t *)SocketHeaderDataRx_Buf, (uint16_t)SocketHeaderDataRx_Cpt);
  return (retval);
}



/* ###########################  END CUSTOMIZATION PART  ########################### */

#endif /* USE_MODEM_UG96 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

