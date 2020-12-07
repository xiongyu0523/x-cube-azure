/**
  ******************************************************************************
  * @file    at_custom_signalling.c
  * @author  MCD Application Team
  * @brief   This file provides common code for the modems
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_datapack.h"
#include "at_util.h"
#include "sysctrl.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/
typedef char atcm_TYPE_CHAR_t;

/* Private defines -----------------------------------------------------------*/
#define MAX_CGEV_PARAM_SIZE         (32U)
#define ATC_GET_MINIMUM_SIZE(a,b) (((a)<(b))?(a):(b))

#define UNKNOWN_NETWORK_TYPE (uint8_t)(0x0) /* should match to NETWORK_TYPE_LUT */
#define CS_NETWORK_TYPE      (uint8_t)(0x1) /* should match to NETWORK_TYPE_LUT */
#define GPRS_NETWORK_TYPE    (uint8_t)(0x2) /* should match to NETWORK_TYPE_LUT */
#define EPS_NETWORK_TYPE     (uint8_t)(0x3) /* should match to NETWORK_TYPE_LUT */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_MODEM == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCModem:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCModem:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCModem API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCModem ERROR:" format "\n\r", ## args)
#define PrintBuf(pbuf, size)       TracePrintBufChar(DBG_CHAN_ATCMD, DBL_LVL_P1, (char *)pbuf, size);
#else
#define PrintINFO(format, args...)  printf("ATCModem:" format "\n\r", ## args);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   printf("ATCModem ERROR:" format "\n\r", ## args);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#define PrintBuf(format, args...)   do {} while(0);
#endif /* USE_TRACE_ATCUSTOM_MODEM */

/* TWO MACROS USED TO SIMPLIFY CODE WHEN MULTIPLE PARAMETERS EXPECTED FOR AN AT-COMMAND ANSWER */
#define START_PARAM_LOOP()  PrintDBG("rank = %d",element_infos->param_rank)\
  uint8_t exitcode = 0U;\
  at_endmsg_t msg_end = ATENDMSG_NO;\
  do { if (msg_end == ATENDMSG_YES) {exitcode = 1U;}

#define END_PARAM_LOOP()    msg_end = atcc_extractElement(p_at_ctxt, p_msg_in, element_infos);\
  if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
  } while (exitcode == 0U);

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void display_clear_network_state(CS_NetworkRegState_t state, uint8_t network_type);
static CS_NetworkRegState_t convert_NetworkState(uint32_t state, uint8_t network_type);
static CS_PDN_conf_id_t find_user_cid_with_matching_ip_addr(atcustom_persistent_context_t *p_persistent_ctxt,
                                                            csint_ip_addr_info_t *ip_addr_struct);
static at_action_rsp_t analyze_CmeError(at_context_t *p_at_ctxt,
                                        atcustom_modem_context_t *p_modem_ctxt,
                                        const IPC_RxMessage_t *p_msg_in,
                                        at_element_info_t *element_infos);
static void set_error_report(csint_error_type_t err_type, atcustom_modem_context_t *p_modem_ctxt);

/* Private function Definition -----------------------------------------------*/

static void display_clear_network_state(CS_NetworkRegState_t state, uint8_t network_type)
{
#if (USE_TRACE_ATCUSTOM_MODEM == 1U) /* to avoid warning when no traces */
  /* Commands Look-up table for AT+QCFG */
  static const AT_CHAR_t NETWORK_TYPE_LUT[][16] =
  {
    {"(unknown)"},
    {"(CS)"},
    {"(GPRS)"},
    {"(EPS)"},
  };

  /* check that network type is valid */
  if (network_type <= EPS_NETWORK_TYPE)
  {
    switch (state)
    {
    case CS_NRS_NOT_REGISTERED_NOT_SEARCHING:
      PrintINFO("NetworkState %s = NOT_REGISTERED_NOT_SEARCHING", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTERED_HOME_NETWORK:
      PrintINFO("NetworkState %s = REGISTERED_HOME_NETWORK", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_NOT_REGISTERED_SEARCHING:
      PrintINFO("NetworkState %s = NOT_REGISTERED_SEARCHING", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTRATION_DENIED:
      PrintINFO("NetworkState %s = REGISTRATION_DENIED", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_UNKNOWN:
      PrintINFO("NetworkState %s = UNKNOWN", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTERED_ROAMING:
      PrintINFO("NetworkState %s = REGISTERED_ROAMING", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK:
      PrintINFO("NetworkState %s = REGISTERED_SMS_ONLY_HOME_NETWORK", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTERED_SMS_ONLY_ROAMING:
      PrintINFO("NetworkState %s = REGISTERED_SMS_ONLY_ROAMING", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_EMERGENCY_ONLY:
      PrintINFO("NetworkState %s = EMERGENCY_ONLY", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK:
      PrintINFO("NetworkState %s = REGISTERED_CFSB_NP_HOME_NETWORK", NETWORK_TYPE_LUT[network_type])
        break;
    case CS_NRS_REGISTERED_CFSB_NP_ROAMING:
      PrintINFO("NetworkState %s = REGISTERED_CFSB_NP_ROAMING", NETWORK_TYPE_LUT[network_type])
        break;
    default:
      PrintINFO("unknown state value")
        break;
    }
  }
  else
  {
    PrintErr("Invalid network type %d", network_type)
  }
#endif /* USE_TRACE_ATCUSTOM_MODEM == 1U */
}

static CS_NetworkRegState_t convert_NetworkState(uint32_t state, uint8_t network_type)
{
  CS_NetworkRegState_t retval;

  switch (state)
  {
    case 0:
      retval = CS_NRS_NOT_REGISTERED_NOT_SEARCHING;
      break;
    case 1:
      retval = CS_NRS_REGISTERED_HOME_NETWORK;
      break;
    case 2:
      retval = CS_NRS_NOT_REGISTERED_SEARCHING;
      break;
    case 3:
      retval = CS_NRS_REGISTRATION_DENIED;
      break;
    case 4:
      retval = CS_NRS_UNKNOWN;
      break;
    case 5:
      retval = CS_NRS_REGISTERED_ROAMING;
      break;
    case 6:
      retval = CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK;
      break;
    case 7:
      retval = CS_NRS_REGISTERED_SMS_ONLY_ROAMING;
      break;
    case 8:
      retval = CS_NRS_EMERGENCY_ONLY;
      break;
    case 9:
      retval = CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK;
      break;
    case 10:
      retval = CS_NRS_REGISTERED_CFSB_NP_ROAMING;
      break;
    default:
      retval = CS_NRS_UNKNOWN;
      break;
  }

  display_clear_network_state(retval, network_type);

  return (retval);
}

/*
 * Try to find user cid with matching IP address
 */
static CS_PDN_conf_id_t find_user_cid_with_matching_ip_addr(atcustom_persistent_context_t *p_persistent_ctxt,
                                                            csint_ip_addr_info_t *ip_addr_struct)
{
  CS_PDN_conf_id_t user_cid = CS_PDN_NOT_DEFINED;

  /* seach user config ID corresponding to this IP address */
  for (uint8_t loop = 0U; loop < MODEM_MAX_NB_PDP_CTXT; loop++)
  {
    atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[loop];
    PrintDBG("[Compare ip addr with user cid=%d]: <%s> vs <%s>",
             loop,
             (char *)&ip_addr_struct->ip_addr_value,
             (char *)&p_tmp->ip_addr_infos.ip_addr_value)

    /* quick and dirty solution
     * should implement a better solution */
    uint8_t size1, size2, minsize;
    size1 = (uint8_t) strlen((atcm_TYPE_CHAR_t *)&ip_addr_struct->ip_addr_value);
    size2 = (uint8_t) strlen((atcm_TYPE_CHAR_t *)&p_tmp->ip_addr_infos.ip_addr_value);
    minsize = (size1 < size2) ? size1 : size2;
    if ((0 == memcmp((AT_CHAR_t *)&ip_addr_struct->ip_addr_value[0],
                     (AT_CHAR_t *)&p_tmp->ip_addr_infos.ip_addr_value[0],
                     (size_t) minsize)) &&
        (minsize != 0U)
       )
    {
      user_cid = atcm_convert_index_to_PDN_conf(loop);
      PrintDBG("Found matching user cid=%d", user_cid)
    }
  }

  return (user_cid);
}

/*
 * Analyze +CME ERROR
*/
static at_action_rsp_t analyze_CmeError(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                        const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter analyze_CmeError_CPIN()")

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0U};
    PrintDBG("CME ERROR parameter received:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
    }
    else
    {
      PrintErr("line exceed maximum size, line ignored...")
    }

    /* convert string to test to upper case */
    ATutil_convertStringToUpperCase(&line[0], 32U);

    /* extract value and compare it to expected value */
    if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM NOT INSERTED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_NOT_INSERTED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PIN NECESSARY") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PIN REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PUK REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM FAILURE") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_FAILURE;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM BUSY") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_BUSY;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM WRONG") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_WRONG;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "INCORRECT PASSWORD") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_INCORRECT_PASSWORD;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PIN2 REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN2_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PUK2 REQUIRED") != NULL)
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK2_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else
    {
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_UNKNOWN;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
  }
  END_PARAM_LOOP()

  return (retval);
}

/*
 * Update error report that will be sent to Cellular Service
 */
static void set_error_report(csint_error_type_t err_type, atcustom_modem_context_t *p_modem_ctxt)
{
  p_modem_ctxt->SID_ctxt.error_report.error_type = err_type;

  switch (err_type)
  {
    case CSERR_SIM:
      p_modem_ctxt->SID_ctxt.error_report.sim_state = p_modem_ctxt->persist.sim_state;
      break;

    default:
      /* nothing to do*/
      break;
  }
}

/* Functions Definition ------------------------------------------------------*/

/* ==========================  Build 3GPP TS 27.007 commands ========================== */
at_status_t fCmdBuild_NoParams(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_atp_ctxt);
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  /* Command as no parameters - STUB function */
  PrintAPI("enter fCmdBuild_NoParams()")

  return (retval);
}

at_status_t fCmdBuild_CGSN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGSN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+CGSN=<n> where <n>:
      * 0: serial number
      * 1: IMEI
      * 2: IMEISV (IMEI and Software version number)
      * 3: SVN (Software version number)
      *
      * <n> parameter is set previously
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                   p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param);
  }
  return (retval);
}

at_status_t fCmdBuild_CMEE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CMEE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+CMEE=<n> where <n>:
      * 0: <err> result code disabled and ERROR used
      * 1: <err> result code enabled and numeric <ERR> values used
      * 2: <err> result code enabled and verbose <ERR> values used
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->persist.cmee_level);
  }
  return (retval);
}

at_status_t fCmdBuild_CPIN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CPIN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    PrintDBG("pin code= %s", p_modem_ctxt->SID_ctxt.modem_init.pincode.pincode)

    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%s",
                   p_modem_ctxt->SID_ctxt.modem_init.pincode.pincode);
  }
  return (retval);
}

at_status_t fCmdBuild_CFUN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CFUN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_INIT_MODEM)
    {
      uint8_t fun, rst;
      /* at modem init, get CFUN mode from parameters */
      const csint_modemInit_t *modemInit_struct = &(p_modem_ctxt->SID_ctxt.modem_init);

      /* AT+CFUN=<fun>,<rst>
        * where <fun>:
        *  0: minimum functionality
        *  1: full functionality
        *  4: disable phone TX & RX
        * where <rst>:
        *  0: do not reset modem before setting <fun> parameter
        *  1: reset modem before setting <fun> parameter
        */
      fun = 0U;

      /* convert cellular service paramaters to Modem format */
      if (modemInit_struct->init == CS_CMI_MINI)
      {
        fun = 0U;
      }
      else if (modemInit_struct->init == CS_CMI_FULL)
      {
        fun = 1U;
      }
      else if (modemInit_struct->init == CS_CMI_SIM_ONLY)
      {
        fun = 4U;
      }
      else
      {
        retval = ATSTATUS_ERROR;
        PrintErr("invalid parameter")
      }

      if (retval == ATSTATUS_OK)
      {
        (modemInit_struct->reset == CELLULAR_TRUE) ? (rst = 1U) : (rst = 0U);
        (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d", fun, rst);
      }
    }
    else /* user settings */
    {
      /* set parameter defined by user */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,0", p_modem_ctxt->CMD_ctxt.cfun_value);
    }
  }
  return (retval);
}

at_status_t fCmdBuild_COPS(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_COPS()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_OperatorSelector_t *operatorSelect = &(p_modem_ctxt->SID_ctxt.write_operator_infos);

    if (operatorSelect->mode == CS_NRM_AUTO)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else if (operatorSelect->mode == CS_NRM_MANUAL)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1,%d,%s", operatorSelect->format,
                     operatorSelect->operator_name);
    }
    else if (operatorSelect->mode == CS_NRM_DEREGISTER)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (operatorSelect->mode == CS_NRM_MANUAL_THEN_AUTO)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "4,%d,%s", operatorSelect->format,
                     operatorSelect->operator_name);
    }
    else
    {
      PrintErr("invalid mode value for +COPS")
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

at_status_t fCmdBuild_CGATT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGATT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* if cgatt set by user or if in ATTACH sequence */
    if ((p_modem_ctxt->CMD_ctxt.cgatt_write_cmd_param == CGATT_ATTACHED) ||
        (p_atp_ctxt->current_SID == (at_msg_t) SID_ATTACH_PS_DOMAIN))
    {
      /* request attach */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
    }
    /* if cgatt set by user or if in DETACH sequence */
    else if ((p_modem_ctxt->CMD_ctxt.cgatt_write_cmd_param == CGATT_DETACHED) ||
             (p_atp_ctxt->current_SID == (at_msg_t) SID_DETACH_PS_DOMAIN))
    {
      /* request detach */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    /* not in ATTACH or DETACH sequence and cgatt_write_cmd_param not set by user: error ! */
    else
    {
      PrintErr("CGATT state parameter not set")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CREG:2, will be sorted at cellular service level */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                     p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CGREG:2, will be sorted at cellular service level */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                     p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CEREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CEREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CEREG:2, will be sorted at cellular service level */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                     p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGEREP(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGEREP()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* Packet Domain event reporting +CGEREP
      * 3GPP TS 27.007
      * Write command: +CGEREP=[<mode>[,<bfr>]]
      * where:
      *  <mode> = 0: no URC (+CGEV) are reported
      *         = 1: URC are discsarded when link is reserved (data on) and forwared otherwise
      *         = 2: URC are buffered when link is reserved and send when link freed, and forwared otherwise
      *  <bfr>  = 0: MT buffer of URC is cleared when <mode> 1 or 2 is entered
      *         = 1: MT buffer of URC is flushed to TE when <mode> 1 or 2 is entered
      */
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_REGISTER_PDN_EVENT)
    {
      /* enable notification (hard-coded value 1,0) */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1,0");
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DEREGISTER_PDN_EVENT)
    {
      /* disable notifications */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* nothing to do */
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGDCONT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGDCONT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {

    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PrintINFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id != CS_PDN_NOT_DEFINED)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\"",
                     modem_cid,
                     atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type),
                     p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn);
    }
    else
    {
      PrintErr("PDP context not defined for conf_id %d", current_conf_id)
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_CGACT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGACT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* PDP context activate or deactivate
      *  3GPP TS 27.007
      *  AT+CGACT=[<state>[,<cid>[,<cid>[,...]]]]
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d",
                   ((p_modem_ctxt->CMD_ctxt.pdn_state == PDN_STATE_ACTIVATE) ? 1 : 0),
                   modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_CGDATA(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGDATA()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* Enter data state
      *  3GPP TS 27.007
      *  AT+CGDATA[=<L2P>[,<cid>[,<cid>[,...]]]]
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"PPP\",%d",
                   modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_CGPADDR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_CGPADDR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* Show PDP adress(es)
      *  3GPP TS 27.007
      *  AT+CGPADDR[=<cid>[,<cid>[,...]]]
      *
      *  implemenation: we only request address for 1 cid (if more cid required, call it again)
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", modem_cid);
  }

  return (retval);
}

/* ==========================  Build V.25ter commands ========================== */
at_status_t fCmdBuild_ATD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATD()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* actually implemented specifically for each modem
      *  following example is not guaranteed ! (cid is not specified here)
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "*99#");
  }
  return (retval);
}

at_status_t fCmdBuild_ATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATE()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* echo mode ON or OFF */
    if (p_modem_ctxt->CMD_ctxt.command_echo == AT_TRUE)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
    }
    else
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

at_status_t fCmdBuild_ATV(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATV()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* echo mode ON or OFF */
    if (p_modem_ctxt->CMD_ctxt.dce_full_resp_format == AT_TRUE)
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
    }
    else
    {
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

at_status_t fCmdBuild_ATX(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATX()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* CONNECT Result code and monitor call progress
      *  for the moment, ATX0 to return result code only, dial tone and busy detection are both disabled
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
  }
  return (retval);
}

at_status_t fCmdBuild_ATZ(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ATZ()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    uint8_t profile_nbr = 0;
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", profile_nbr);
  }
  return (retval);
}

at_status_t fCmdBuild_IPR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_IPR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set baud rate */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld", p_modem_ctxt->CMD_ctxt.baud_rate);
  }
  return (retval);
}

at_status_t fCmdBuild_IFC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_IPR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set flow control */
    if (p_modem_ctxt->CMD_ctxt.flow_control_cts_rts == AT_FALSE)
    {
      /* No flow control */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0,0");
    }
    else
    {
      /* CTS/RTS activated */
      (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2,2");
    }
  }
  return (retval);
}


at_status_t fCmdBuild_ESCAPE_CMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_ESCAPE_CMD()")

  /* only for RAW command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    /* set escape sequence (as define in custom modem specific) */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%s", p_atp_ctxt->current_atcmd.name);
    /* set raw command size */
    p_atp_ctxt->current_atcmd.raw_cmd_size = strlen((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params);
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

at_status_t fCmdBuild_AT_AND_D(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_AT_AND_D()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* Set DTR function mode  (cf V.25ter)
      * hard-coded to 0
      */
    (void) sprintf((atcm_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");

  }

  return (retval);
}

at_status_t fCmdBuild_DIRECT_CMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_DIRECT_CMD()")

  /* only for RAW command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    if (p_modem_ctxt->SID_ctxt.direct_cmd_tx->cmd_size != 0U)
    {
      uint32_t str_size = p_modem_ctxt->SID_ctxt.direct_cmd_tx->cmd_size;
      (void) memcpy((void *)p_atp_ctxt->current_atcmd.params,
                    (const CS_CHAR_t *)p_modem_ctxt->SID_ctxt.direct_cmd_tx->cmd_str,
                    str_size);

      /* add termination characters */
      uint32_t endstr_size = strlen((atcm_TYPE_CHAR_t *)&p_atp_ctxt->endstr);
      (void) memcpy((void *)&p_atp_ctxt->current_atcmd.params[str_size],
                    p_atp_ctxt->endstr,
                    endstr_size);

      /* set raw command size */
      p_atp_ctxt->current_atcmd.raw_cmd_size = str_size + endstr_size;
    }
    else
    {
      PrintErr("ERROR, send buffer is empty")
      retval = ATSTATUS_ERROR;
    }
  }
  return (retval);
}
/* ==========================  Analyze 3GPP TS 27.007 commands ========================== */
at_action_rsp_t fRspAnalyze_None(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_None()")

  /* no parameters expected */

  return (retval);
}

at_action_rsp_t fRspAnalyze_Error(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PrintAPI("enter fRspAnalyze_Error()")

  /* analyse parameters for ERROR */
  /* use CmeErr function for the moment */
  retval = fRspAnalyze_CmeErr(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  return (retval);
}

at_action_rsp_t fRspAnalyze_CmeErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  const atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_ERROR;
  PrintAPI("enter fRspAnalyze_CmeErr()")

  /* Analyze CME error to report it to upper layers */
  (void) analyze_CmeError(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  /* specific treatments for +CME ERROR, depending of current command */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case CMD_AT_CGSN:
    {
      switch (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param)
      {
        case CGSN_SN:
          PrintDBG("Modem Error for CGSN_SN, use unitialized value")
          (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.serial_number), 0, MAX_SIZE_SN);
          break;

        case CGSN_IMEI:
          PrintDBG("Modem Error for CGSN_IMEI, use unitialized value")
          (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei), 0, MAX_SIZE_IMEI);
          break;

        case CGSN_IMEISV:
          PrintDBG("Modem Error for CGSN_IMEISV, use unitialized value, parameter ignored")
          break;

        case CGSN_SVN:
          PrintDBG("Modem Error for CGSN_SVN, use unitialized value, parameter ignored")
          break;

        default:
          PrintDBG("Modem Error for CGSN, unexpected parameter")
          retval = ATACTION_RSP_ERROR;
          break;
      }
      break;
    }

    case CMD_AT_CIMI:
      PrintDBG("Modem Error for CIMI, use unitialized value")
      (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imsi), 0, MAX_SIZE_IMSI);
      break;

    case CMD_AT_CGMI:
      PrintDBG("Modem Error for CGMI, use unitialized value")
      (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.manufacturer_name), 0, MAX_SIZE_MANUFACT_NAME);
      break;

    case CMD_AT_CGMM:
      PrintDBG("Modem Error for CGMM, use unitialized value")
      (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.model), 0, MAX_SIZE_MODEL);
      break;

    case CMD_AT_CGMR:
      PrintDBG("Modem Error for CGMR, use unitialized value")
      (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.revision), 0, MAX_SIZE_REV);
      break;

    case CMD_AT_CNUM:
      PrintDBG("Modem Error for CNUM, use unitialized value")
      (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.phone_number), 0, MAX_SIZE_PHONE_NBR);
      break;

    case CMD_AT_GSN:
      PrintDBG("Modem Error for GSN, use unitialized value")
      (void) memset((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei), 0, MAX_SIZE_IMEI);
      break;

    case CMD_AT_CPIN:
      PrintDBG("Analyze Modem Error for CPIN")
      break;

    /* consider all other error cases for AT commands
      * case ?:
      * etc...
      */

    default:
      PrintDBG("Modem Error for cmd (id=%ld)", p_atp_ctxt->current_atcmd.id)
      retval = ATACTION_RSP_ERROR;
      break;
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CmsErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CmsErr()")

  /* analyze parameters for +CMS ERROR */
  /* Not implemented */

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGMI()")

  /* analyze parameters for +CGMI */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Manufacturer name:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.manufacturer_name),
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t)element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGMM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGMM()")

  /* analyze parameters for +CGMM */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Model:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.model),
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t)element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGMR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGMR()")

  /* analyze parameters for +CGMR */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Revision:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.revision),
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t)element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGSN()")

  /* analyze parameters for +CGSN */
  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SN)
    {
      /* serial number */
      PrintDBG("Serial Number:")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.serial_number),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
    }
    else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEI)
    {
      /* IMEI */
      PrintDBG("IMEI:")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
    }
    else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEISV)
    {
      /* IMEISV */
      PrintDBG("IMEISV (NOT USED):")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SVN)
    {
      /* SVN */
      PrintDBG("SVN (NOT USED):")
      PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    else
    {
      /* nothing to do */
    }
  }

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("Serial Number:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.serial_number),
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t) element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CIMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CIMI()")

  /* analyze parameters for +CIMI */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("IMSI:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imsi),
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t) element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CEER(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CEER()")

  /* analyze parameters for CEER */

  return (retval);
}

at_action_rsp_t fRspAnalyze_CPIN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CPIN()")

  /* analyze parameters for CPIN */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0U};
    PrintDBG("CPIN parameter received:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
    }
    else
    {
      PrintErr("line exceed maximum size, line ignored...")
      return (ATACTION_RSP_IGNORED);
    }

    /* extract value and compare it to expected value */
    if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PIN") != NULL)
    {
      PrintDBG("waiting for SIM PIN")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PUK") != NULL)
    {
      PrintDBG("waiting for SIM PUK")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PIN2") != NULL)
    {
      PrintDBG("waiting for SIM PUK2")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK2_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "SIM PUK2") != NULL)
    {
      PrintDBG("waiting for SIM PUK")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const atcm_TYPE_CHAR_t *)&line[0], "READY") != NULL)
    {
      PrintDBG("CPIN READY")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_TRUE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_READY;
    }
    else
    {
      PrintErr("UNEXPECTED CPIN STATE")
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_UNKNOWN;
      set_error_report(CSERR_SIM, p_modem_ctxt);
    }
  }
  END_PARAM_LOOP()
  return (retval);
}

at_action_rsp_t fRspAnalyze_COPS(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_COPS()")

  /* analyze parameters for +COPS
    *  Different cases to consider (as format is different)
    *  1/ answer to COPS read command
    *     +COPS: <mode>[,<format>,<oper>[,<AcT>]]
    *  2/ answer to COPS test command
    *     +COPS: [list of supported (<stat>,long alphanumeric <oper>,
    *            short alphanumeric <oper>,numeric <oper>[,<AcT>])s]
    *            [,,(list ofsupported <mode>s),(list of supported <format>s)]
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* mode (mandatory) */
      uint32_t mode = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      switch (mode)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_AUTO;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_MANUAL;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_DEREGISTER;
          break;
        case 4:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_MANUAL_THEN_AUTO;
          break;
        default:
          PrintErr("invalid mode value in +COPS")
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_AUTO;
          break;
      }

      PrintDBG("+COPS: mode = %d", p_modem_ctxt->SID_ctxt.read_operator_infos.mode)
    }
    else if (element_infos->param_rank == 3U)
    {
      /* format (optional) */
      uint32_t format = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_FORMAT_PRESENT; /* bitfield */
      switch (format)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_LONG;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_SHORT;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_NUMERIC;
          break;
        default:
          PrintErr("invalid format value")
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_NOT_PRESENT;
          break;
      }
      PrintDBG("+COPS: format = %d", p_modem_ctxt->SID_ctxt.read_operator_infos.format)
    }
    else if (element_infos->param_rank == 4U)
    {
      /* operator name (optional) */
      if (element_infos->str_size > MAX_SIZE_OPERATOR_NAME)
      {
        PrintErr("error, operator name too long")
        return (ATACTION_RSP_ERROR);
      }
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_OPERATOR_NAME_PRESENT;  /* bitfield */
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.read_operator_infos.operator_name[0]),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PrintDBG("+COPS: operator name = %s", p_modem_ctxt->SID_ctxt.read_operator_infos.operator_name)
    }
    else if (element_infos->param_rank == 5U)
    {
      /* AccessTechno (optional) */
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_ACT_PRESENT;  /* bitfield */
      uint32_t AcT = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      switch (AcT)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM_COMPACT;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN;
          break;
        case 3:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM_EDGE;
          break;
        case 4:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSDPA;
          break;
        case 5:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSUPA;
          break;
        case 6:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSDPA_HSUPA;
          break;
        case 7:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN;
          break;
        case 8:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_EC_GSM_IOT;
          PrintINFO(">>> Access Technology : LTE Cat.M1 <<<")
          break;
        case 9:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN_NBS1;
          PrintINFO(">>> Access Technology : LTE Cat.NB1 <<<")
          break;
        default:
          PrintErr("invalid AcT value")
          break;
      }

      PrintDBG("+COPS: Access technology = %ld", AcT)

    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PrintDBG("+COPS for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CNUM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fCmdBuild_CNUM()")

  PrintDBG("+CNUM cmd NOT IMPLEMENTED")

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGATT(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGATT()")

  /* analyze parameters for +CGATT
  *  Different cases to consider (as format is different)
  *  1/ answer to CGATT read command
  *     +CGATT: <state>
  *  2/ answer to CGATT test command
  *     +CGATT: (list of supported <state>s)
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t attach = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->SID_ctxt.attach_status = (attach == 1U) ? CS_PS_ATTACHED : CS_PS_DETACHED;
      PrintDBG("attach status = %d", p_modem_ctxt->SID_ctxt.attach_status)
    }
    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PrintDBG("+CGATT for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CREG()")
  PrintDBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CREG read command
  *     +CREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
  *  2/ answer to CREG test command
  *     +CREG: (list of supported <n>s)
  *  3/ URC:
  *     +CREG: <stat>[,[<lac>],[<ci>],[<AcT>]]
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CREG)
  {
    /* analyze parameters for +CREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PrintDBG("+CREG: n=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.cs_network_state = convert_NetworkState(stat, CS_NETWORK_TYPE);
        PrintDBG("+CREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.cs_location_info.lac = (uint8_t)lac;
        PrintDBG("+CREG: lac=%ld", lac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.cs_location_info.ci = (uint16_t)ci;
        PrintDBG("+CREG: ci=%ld", ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PrintDBG("+CREG: act=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PrintDBG("+CREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }

  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_network_registration = AT_TRUE;
      p_modem_ctxt->persist.cs_network_state = convert_NetworkState(stat, CS_NETWORK_TYPE);
      PrintDBG("+CREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_location_info_lac = AT_TRUE;
      p_modem_ctxt->persist.cs_location_info.lac = (uint8_t)lac;
      PrintDBG("+CREG URC: lac=%ld", lac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.cs_location_info.ci = (uint16_t)ci;
      PrintDBG("+CREG URC: ci=%ld", ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PrintDBG("+CREG URC: act=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGREG()")
  PrintDBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CGREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CGREG read command
  *     +CGREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>, [<rac>] [,<cause_type>,<reject_cause>]]]
  *  2/ answer to CGREG test command
  *     +CGREG: (list of supported <n>s)
  *  3/ URC:
  *     +CGREG: <stat>[,[<lac>],[<ci>],[<AcT>],[<rac>]]
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CGREG)
  {
    /* analyze parameters for +CREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PrintDBG("+CGREG: n=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.gprs_network_state = convert_NetworkState(stat, GPRS_NETWORK_TYPE);
        PrintDBG("+CGREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.gprs_location_info.lac = (uint8_t)lac;
        PrintDBG("+CGREG: lac=%ld", lac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.gprs_location_info.ci = (uint16_t)ci;
        PrintDBG("+CGREG: ci=%ld", ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PrintDBG("+CGREG: act=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 7U)
      {
        /* param traced only */
        PrintDBG("+CGREG: rac=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CGREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PrintDBG("+CGREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }
  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_network_registration = AT_TRUE;
      p_modem_ctxt->persist.gprs_network_state = convert_NetworkState(stat, GPRS_NETWORK_TYPE);
      PrintDBG("+CGREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t lac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_location_info_lac = AT_TRUE;
      p_modem_ctxt->persist.gprs_location_info.lac = (uint8_t)lac;
      PrintDBG("+CGREG URC: lac=%ld", lac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.gprs_location_info.ci = (uint16_t)ci;
      PrintDBG("+CGREG URC: ci=%ld", ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PrintDBG("+CGREG URC: act=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 6U)
    {
      /* param traced only */
      PrintDBG("+CGREG URC: rac=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);

}

at_action_rsp_t fRspAnalyze_CEREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CEREG()")
  PrintDBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CEREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CEREG read command
  *     +CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
  *  2/ answer to CEREG test command
  *     +CEREG: (list of supported <n>s)
  *  3/ URC:
  *     +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]]
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CEREG)
  {
    /* analyze parameters for +CEREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PrintDBG("+CEREG: n=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.eps_network_state = convert_NetworkState(stat, EPS_NETWORK_TYPE);
        PrintDBG("+CEREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t tac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.eps_location_info.lac = (uint8_t)tac;
        PrintDBG("+CEREG: tac=%ld", tac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        p_modem_ctxt->persist.eps_location_info.ci = (uint16_t)ci;
        PrintDBG("+CEREG: ci=%ld", ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PrintDBG("+CEREG: act=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CEREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PrintDBG("+CEREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }
  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_network_registration = AT_TRUE;
      p_modem_ctxt->persist.eps_network_state = convert_NetworkState(stat, EPS_NETWORK_TYPE);
      PrintDBG("+CEREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t tac = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_location_info_tac = AT_TRUE;
      p_modem_ctxt->persist.eps_location_info.lac = (uint8_t)tac;
      PrintDBG("+CEREG URC: tac=%ld", tac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.eps_location_info.ci = (uint16_t)ci;
      PrintDBG("+CEREG URC: ci=%ld", ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PrintDBG("+CEREG URC: act=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGEV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGEV()")

  /* cf 3GPP TS 27.007 */
  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* due to parser implementation (spaces are not considered as a split character) and the format of +CGEV,
    *  we can receive an additional paramater with the PDN event name in the 1st CGEV parameter.
    *  For example:
    *    +CGEV: NW DETACH                                 => no additionnal parameter
    *    +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]       => <cid> will be present here
    *    +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]  => <PDP_type>  will be present here
    *
    * In consequence, a specific treatment is done here.

    * Here is a list of different events we can receive:
    *   +CGEV: NW DETACH
    *   +CGEV: ME DETACH
    *   +CGEV: NW CLASS <class>
    *   +CGEV: ME CLASS <class>
    *   +CGEV: NW PDN ACT <cid>[,<WLAN_Offload>]
    *   +CGEV: ME PDN ACT <cid>[,<reason>[,<cid_other>]][,<WLAN_Offload>]
    *   +CGEV: NW ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: ME ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
    *   +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>]
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
    *   +CGEV: ME PDN DEACT <cid>
    *   +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>].
    *   +CGEV: ME DEACT <p_cid>, <cid>, <event_type>
    *   +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>].
    *   +CGEV: NW MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: ME MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: REJECT <PDP_type>, <PDP_addr>
    *   +CGEV: NW REACT <PDP_type>, <PDP_addr>, [<cid>]
    *
    *  We are only interested by following events:
    *   +CGEV: NW DETACH : the network has forced a Packet domain detach (all contexts)
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>] : the nw has forced a contex deactivation
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>] : context deactivated
    */

    /* check that previous PDN URC has been reported
    *  if this is not the case, we can not report this one
    */
    if (p_modem_ctxt->persist.urc_avail_pdn_event == AT_TRUE)
    {
      PrintErr("an +CGEV URC still not reported, ignore this one")
      return (ATACTION_RSP_ERROR);
    }

    /* reset event params */
    reset_pdn_event(&p_modem_ctxt->persist);

    /* create a copy of params */
    uint8_t copy_params[MAX_CGEV_PARAM_SIZE] = {0};
    AT_CHAR_t *found;
    size_t size_mini = ATC_GET_MINIMUM_SIZE(element_infos->str_size, MAX_CGEV_PARAM_SIZE);
    (void) memcpy((void *)copy_params,
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  size_mini);
    found = (AT_CHAR_t *)strtok((atcm_TYPE_CHAR_t *)copy_params, " ");
    while (found  != NULL)
    {
      /* analyze of +CGEV event received */
      if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "NW"))
      {
        PrintDBG("<NW>")
        p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_NW;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "ME"))
      {
        PrintDBG("<ME>")
        p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_ME;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "PDN"))
      {
        PrintDBG("<PDN>")
        p_modem_ctxt->persist.pdn_event.event_scope = CGEV_EVENT_SCOPE_PDN;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "ACT"))
      {
        PrintDBG("<ACT>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_ACTIVATION;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "DEACT"))
      {
        PrintDBG("<DEACT>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DEACTIVATION;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "REJECT"))
      {
        PrintDBG("<REJECT>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_REJECT;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "DETACH"))
      {
        PrintDBG("<DETACH>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DETACH;
        /* all PDN are concerned */
        p_modem_ctxt->persist.pdn_event.conf_id = CS_PDN_ALL;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "CLASS"))
      {
        PrintDBG("<CLASS>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_CLASS;
      }
      else if (0 == strcmp((atcm_TYPE_CHAR_t *)found, "MODIFY"))
      {
        PrintDBG("<MODIFY>")
        p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_MODIFY;
      }
      else
      {
        /* if falling here, this is certainly an additional paramater (cf above explanation) */
        if (p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW)
        {
          switch (p_modem_ctxt->persist.pdn_event.event_type)
          {
            case CGEV_EVENT_TYPE_DETACH:
            {
              /*  we are in the case:
              *  +CGEV: NW DETACH
              *  => no parameter to analyze
              */
              PrintErr("No parameter expected for  NW DETACH")
              break;
            }


            case CGEV_EVENT_TYPE_DEACTIVATION:
            {
              if (p_modem_ctxt->persist.pdn_event.event_scope == CGEV_EVENT_SCOPE_PDN)
              {
                /* we are in the case:
                *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
                *   => parameter to analyze = <cid>
                */
                uint32_t cgev_cid = ATutil_convertStringToInt((uint8_t *)found, (uint16_t)strlen((atcm_TYPE_CHAR_t *)found));
                p_modem_ctxt->persist.pdn_event.conf_id = atcm_get_configID_for_modem_cid(&p_modem_ctxt->persist, (uint8_t)cgev_cid);
                PrintDBG("+CGEV modem cid=%ld (user conf Id =%d)", cgev_cid, p_modem_ctxt->persist.pdn_event.conf_id)
              }
              else
              {
                /* we are in the case:
                *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
                *   => parameter to analyze = <PDP_type>
                *
                *   skip, parameter not needed in this case
                */
              }
              break;
            }

            default:
            {
              PrintDBG("event type (= %d) ignored", p_modem_ctxt->persist.pdn_event.event_type)
              break;
            }
          }
        }
        else
        {
          PrintDBG("ME events ignored")
        }
      }

      PrintDBG("(%d) ---> %s", strlen((atcm_TYPE_CHAR_t *)found), (uint8_t *) found)
      found = (AT_CHAR_t *)strtok(NULL, " ");
    }

    /* Indicate that a +CGEV URC has been received */
    p_modem_ctxt->persist.urc_avail_pdn_event = AT_TRUE;
  }
  else if (element_infos->param_rank == 3U)
  {
    if ((p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW) &&
        (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
    {
      /* receive <PDP_addr> for:
      * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
      */

      /* analyze <IP_address> and try to find a matching user cid */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* recopy IP address value, ignore type */
      ip_addr_info.ip_addr_type = CS_IPAT_INVALID;
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PrintDBG("<PDP_addr>=%s", (AT_CHAR_t *)&ip_addr_info.ip_addr_value)

      /* find user cid matching this IP addr (if any) */
      p_modem_ctxt->persist.pdn_event.conf_id = find_user_cid_with_matching_ip_addr(&p_modem_ctxt->persist, &ip_addr_info);

    }
    else
    {
      PrintDBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    if ((p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW) &&
        (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
    {
      /* receive <cid> for:
      * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
      */
      /* CID not used: we could use it if problem with <PDP_addr> occured */
    }
    else
    {
      PrintDBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
    }
  }
  else
  {
    PrintDBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_CSQ(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CSQ()")

  /* analyze parameters for CSQ */
  /* for EXECUTION COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* 3GP TS27.007
    *  format: +CSQ: <rssi>,<ber>
    *
    *  <rssi>: integer type
    *          0  -113dBm or less
    *          1  -111dBm
    *          2...30  -109dBm to -53dBm
    *          31  -51dBm or greater
    *          99  unknown or not detectable
    *  <ber>: integer type (channel bit error rate in percent)
    *          0...7  as RXQUAL values in the table 3GPP TS 45.008
    *          99     not known ot not detectable
    */

    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t rssi = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CSQ rssi=%ld", rssi)
      p_modem_ctxt->SID_ctxt.signal_quality->rssi = (uint8_t)rssi;
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t ber = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CSQ ber=%ld", ber)
      p_modem_ctxt->SID_ctxt.signal_quality->ber = (uint8_t)ber;
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CGPADDR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_CGPADDR()")

  /* analyze parameters for CGPADDR */
  /* for WRITE COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* 3GP TS27.007
    *  format: +CGPADDR: <cid>[,<PDP_addr_1>[,>PDP_addr_2>]]]
    *
    *  <cid>: integer type
    *         specifies a particular PDP context definition
    *  <PDP_addr_1> and <PDP_addr_2>: string type
    *         format = a1.a2.a3.a4 for IPv4
    *         format = a1.a2.a3.a4.a5a.a6.a7.a8.a9.a10.a11.a12a.a13.a14.a15.a16 for IPv6
    */

    START_PARAM_LOOP()
    PrintDBG("+CGPADDR param_rank = %d", element_infos->param_rank)
    if (element_infos->param_rank == 2U)
    {
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+CGPADDR cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if ((element_infos->param_rank == 3U) || (element_infos->param_rank == 4U))
    {
      /* analyze <PDP_addr_1> and <PDP_addr_2> */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* retrive IP address value */
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PrintDBG("+CGPADDR addr=%s", (AT_CHAR_t *)&ip_addr_info.ip_addr_value)

      /* determine IP address type */
      ip_addr_info.ip_addr_type = atcm_get_ip_address_type((AT_CHAR_t *)&ip_addr_info.ip_addr_value);

      /* save IP address infos in modem_cid_table */
      if (element_infos->param_rank == 3U)
      {
        atcm_put_IP_address_infos(&p_modem_ctxt->persist, (uint8_t)p_modem_ctxt->CMD_ctxt.modem_cid, &ip_addr_info);
      }
    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/* ==========================  Analyze V.25ter commands ========================== */
at_action_rsp_t fRspAnalyze_GSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_GSN()")

  /* analyze parameters for +GSN */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PrintDBG("IMEI:")
    PrintBuf((uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.imei),
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t) element_infos->str_size);
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_IPR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
#if (USE_TRACE_ATCUSTOM_MODEM == 0U)
  UNUSED(p_msg_in); /* for MISRA-2012 */
#endif /* USE_TRACE_ATCUSTOM_MODEM */

  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_IPR()")

  /* analyze parameters for +IPR */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    PrintDBG("BAUD RATE:")
    if (element_infos->param_rank == 2U)
    {
      /* param trace only */
      PrintDBG("+IPR baud rate=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_DIRECT_CMD(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_DIRECT_CMD()")

  /* NOT IMPLEMENTED YET */

  return (retval);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
