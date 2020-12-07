#ifdef USE_MODEM_UG96
/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the 'socket mode' code to the
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_custom_modem_socket_ug96.h"
#include "at_custom_modem_specific_ug96.h"
#include "at_datapack.h"
#include "at_util.h"
#include "plf_config.h"
#include "plf_modem_config_ug96.h"
#include "error_handler.h"

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

/* TWO MACROS USED TO SIMPLIFY CODE WHEN MULTIPLE PARAMETERS EXPECTED FOR AN AT-COMMAND ANSWER */
#define START_PARAM_LOOP()  PrintDBG("rank = %d",element_infos->param_rank)\
  uint8_t exitcode = 0U;\
  at_endmsg_t msg_end = ATENDMSG_NO;\
  do\
  {\
    if (msg_end == ATENDMSG_YES) {exitcode = 1U;}

#define END_PARAM_LOOP()    msg_end = atcc_extractElement(p_at_ctxt, p_msg_in, element_infos);\
  if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
  } while (exitcode == 0U);

/* Private defines -----------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt);

/* Functions Definition ------------------------------------------------------*/

/* Build command functions ------------------------------------------------------- */
at_status_t fCmdBuild_QIACT_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIACT_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PrintINFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      PrintINFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",  modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_QIOPEN_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  /* Array to convert AT+QIOPEN service type parameter to a string
  *  need to be aligned with ATCustom_UG96_QIOPENservicetype_t
  */
  static const AT_CHAR_t *const ug96_array_QIOPEN_service_type[] =
  {
    ((uint8_t *)"TCP"),             /* start TCP connnection as client */
    ((uint8_t *)"UDP"),             /* start UDP connection as client */
    ((uint8_t *)"TCP LISTENER"),    /* start TCP server to listen TCP connection */
    ((uint8_t *)"UDP SERVICE"),
  };  /* start UDP service */ /* all const */

  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIOPEN_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* IP AT Commands manual - LTE Module Series - V1.0
      * AT+QIOPEN=<contextID>,<connectId>,<service_type>,<IP_address>/<domain_name>,<remote_port>[,
      *           <local_port>[,<access_mode>]]
      *
      * <contextID> is the PDP context ID
      */
      /* convert user cid (CS_PDN_conf_id_t) to PDP modem cid (value) */
      uint8_t pdp_modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, p_modem_ctxt->socket_ctxt.socket_info->conf_id);
      PrintDBG("user cid = %d, PDP modem cid = %d", (uint8_t)p_modem_ctxt->socket_ctxt.socket_info->conf_id, pdp_modem_cid)
      uint8_t access_mode = 0U; /* 0=buffer acces mode, 1=direct push mode, 2=transparent access mode */

      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DIAL_COMMAND)
      {
        /* client mode: TODO is it only for client mode ? in this casr tcp listener and udp service should not be supported here ? */
        uint8_t service_type_index;
        if (strcmp((ug96_TYPE_CHAR_t const *)p_modem_ctxt->socket_ctxt.socket_info->ip_addr_value,
                   (ug96_TYPE_CHAR_t const *)CONFIG_MODEM_UDP_SERVICE_CONNECT_IP) == 0)
        {
          /* "TCP LISTENER" or "UDP SERVICE" */
          service_type_index = QIOPENSERVICETYPE_UDP_SERVICE;
        }
        else
        {
          /* "TCP" or "UDP" (client) */
          service_type_index = ((p_modem_ctxt->socket_ctxt.socket_info->protocol == CS_TCP_PROTOCOL) ? QIOPENSERVICETYPE_TCP_CLIENT : QIOPENSERVICETYPE_UDP_CLIENT);
        }

        (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%ld,\"%s\",\"%s\",%d,%d,%d",
                       pdp_modem_cid,
                       atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle),
                       ug96_array_QIOPEN_service_type[service_type_index],
                       p_modem_ctxt->socket_ctxt.socket_info->ip_addr_value,
                       p_modem_ctxt->socket_ctxt.socket_info->remote_port,
                       p_modem_ctxt->socket_ctxt.socket_info->local_port,
                       access_mode);

        /* waiting for +QIOPEN now */
        ug96_shared.QIOPEN_waiting = AT_TRUE;
      }
      /* QIOPEN for server mode (not supported yet)
      *  else if (curSID == ?) for server mode (corresponding to CDS_socket_listen)
      */
      else
      {
        /* error */
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_QICLOSE_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QICLOSE_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_info != NULL)
    {
      /* IP AT Commands manual - LTE Module Series - V1.0
      * AT+QICLOSE=connectId>[,<timeout>]
      */
      uint32_t connID = atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);
      (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld", connID);
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_QISEND_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QISEND_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /*IP AT Commands manual - LTE Module Series - V1.0
     * send data in command mode:
     * AT+QISEND=<connId>,<send_length><CR>
     * > ...DATA...
     *
     * DATA are sent using fCmdBuild_QISEND_WRITE_DATA_UG96()
     */
    if (p_modem_ctxt->SID_ctxt.socketSendData_struct.ip_addr_type == CS_IPAT_INVALID)
    {
      /* QISEND format for "TCP", "UDP" or "TCP INCOMING" :
       *   AT+QISEND=<connectID>,<send_length>
       */
      (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,%ld",
                     atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->SID_ctxt.socketSendData_struct.socket_handle),
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size
                    );
    }
    else
    {
      /* QISEND format for "UDP SERVICE"
       *   AT+QISEND=<connectID>,<send_length>,<remoteIP>,<remote_port>
       */
      (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,%ld,\"%s\",%d",
                     atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->SID_ctxt.socketSendData_struct.socket_handle),
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size,
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.ip_addr_value,
                     p_modem_ctxt->SID_ctxt.socketSendData_struct.remote_port
                    );
    }
  }

  return (retval);
}

at_status_t fCmdBuild_QISEND_WRITE_DATA_UG96(atparser_context_t *p_atp_ctxt,
                                                    atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QISEND_WRITE_DATA_UG96()")

  /* after having send AT+QISEND and prompt received, now send DATA */
  /* only for raw command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    if (p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send != NULL)
    {
      uint32_t str_size = p_modem_ctxt->SID_ctxt.socketSendData_struct.buffer_size;
      (void) memcpy((void *)p_atp_ctxt->current_atcmd.params,
                    (const CS_CHAR_t *)p_modem_ctxt->SID_ctxt.socketSendData_struct.p_buffer_addr_send,
                    (size_t) str_size);

      /* set raw command size */
      p_atp_ctxt->current_atcmd.raw_cmd_size = str_size;
    }
    else
    {
      PrintErr("ERROR, send buffer is a NULL ptr !!!")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_QIRD_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIRD_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestSize)
    {
      /* requesting socket data size (set length = 0) */
      (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,0",
                     atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle));
    }
    else if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Header)
    {
      uint32_t requested_data_size;
#if defined(USE_C2C_CS_ADAPT)
      /* read only the requested size */
      requested_data_size = p_modem_ctxt->socket_ctxt.socketReceivedata.max_buffer_size;
#else
      requested_data_size = p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size;
#endif /* USE_C2C_CS_ADAPT */

      /* requesting socket data with correct size */
      (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld,%ld",
                     atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socketReceivedata.socket_handle),
                     requested_data_size);

      /* ready to start receive socket buffer */
      p_modem_ctxt->socket_ctxt.socket_RxData_state = SocketRxDataState_waiting_header;
    }
    else
    {
      PrintErr("Unexpected socket receiving state")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_QISTATE_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QISTATE_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+QISTATE=<query_type>,<connectID>
    *
    * <query_type> = 1
    */

    (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1,%ld",
                   atcm_socket_get_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_cnx_infos->socket_handle));
  }

  return (retval);
}


at_status_t fCmdBuild_QIDNSCFG_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIDNSCFG_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    *  1- Configure DNS server address for specified PDP context
    *  *  AT+QIDNSCFG=<contextID>,<pridnsaddr>[,<secdnsaddr>]
    *
    *  2- Query DNS server address of specified PDP context
    *  AT+QIDNSCFG=<contextID>
    *  response:
    *  +QIDNSCFG: <contextID>,<pridnsaddr>,<secdnsaddr>
    *
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    /* configure DNS server address for the specfied PDP context */
    (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
                   modem_cid,
                   p_modem_ctxt->SID_ctxt.dns_request_infos->dns_conf.primary_dns_addr);
  }

  return (retval);
}

at_status_t fCmdBuild_QIDNSGIP_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QIDNSGIP_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    * AT+QIDNSGIP=<contextID>,<hostname>
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\"",
                   modem_cid,
                   p_modem_ctxt->SID_ctxt.dns_request_infos->dns_req.host_name);
  }
  return (retval);
}

at_status_t fCmdBuild_QPING_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PrintAPI("enter fCmdBuild_QPING_UG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf TCP/IP AT commands manual v1.0
    *  Ping a remote server:
    *  AT+QPING=<contextID>,<host<[,<timeout>[,<pingnum>]]
    */
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    (void) sprintf((ug96_TYPE_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",%d,%d",
                   modem_cid,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.host_addr,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.timeout,
                   p_modem_ctxt->SID_ctxt.ping_infos.ping_params.pingnum);
  }

  return (retval);
}

/* Analyze command functions ------------------------------------------------------- */
at_action_rsp_t fRspAnalyze_QIACT_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)

{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIACT_UG96()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    /* Answer to AT+QIACT?
    *  Returns the list of current activated contexts and their IP addresses
    *  format: +QIACT: <cid>,<context_state>,<context_type>,[,<IP_address>]
    *
    *  where:
    *  <cid>: context ID, range is 1 to 16
    *  <context_state>: 0 for deactivated, 1 for activated
    *  <context_type>: 1 for IPV4, 2 for IPV6
    *  <IP_address>: string type
    */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* analyze <cid> */
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+QIACT cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if (element_infos->param_rank == 3U)
    {
      /* analyze <context_state> */
      uint32_t context_state = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintDBG("+QIACT context_state=%ld", context_state)

      /* If we are trying to activate a PDN and we see that it is already active, do not request the activation to avoid an error */
      if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_ACTIVATE_PDN) &&
          (context_state == 1U))
      {
        /* is it the required PDN to activate ? */
        CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
        uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
        if (p_modem_ctxt->CMD_ctxt.modem_cid == modem_cid)
        {
          PrintDBG("Modem CID to activate (%d) is already activated", modem_cid)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
          ug96_shared.pdn_already_active = AT_TRUE;
#endif /* USE_SOCKETS_TYPE */
        }
      }
    }
    else if (element_infos->param_rank == 4U)
    {
      /* analyze <context_type> */
      PrintDBG("+QIACT context_type=%ld",
               ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 5U)
    {
      /* analyze <IP_address> */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* retrive IP address value */
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PrintDBG("+QIACT addr=%s", (AT_CHAR_t *)&ip_addr_info.ip_addr_value)

      /* determine IP address type */
      ip_addr_info.ip_addr_type = atcm_get_ip_address_type((AT_CHAR_t *)&ip_addr_info.ip_addr_value);

      /* save IP address infos in modem_cid_table */
      atcm_put_IP_address_infos(&p_modem_ctxt->persist, (uint8_t)p_modem_ctxt->CMD_ctxt.modem_cid, &ip_addr_info);
    }
    else
    {
      /* param ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_QIOPEN_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                               const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIOPEN_UG96()")
  uint32_t ug96_current_qiopen_connectId = 0U;

  /* are we waiting for QIOPEN ? */
  if (ug96_shared.QIOPEN_waiting == AT_TRUE)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      uint32_t connectID;
      connectID = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      ug96_current_qiopen_connectId = connectID;
      ug96_shared.QIOPEN_current_socket_connected = 0U;
    }
    else if (element_infos->param_rank == 3U)
    {
      uint32_t err_value;
      err_value = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);

      /* compare QIOPEN connectID with the value requested by user (ie in current SID)
      *  and check if err=0
      */
      if ((ug96_current_qiopen_connectId == atcm_socket_get_modem_cid(p_modem_ctxt,
                                                                      p_modem_ctxt->socket_ctxt.socket_info->socket_handle)) &&
          (err_value == 0U))
      {
        PrintINFO("socket (connectId=%ld) opened", ug96_current_qiopen_connectId)
        ug96_shared.QIOPEN_current_socket_connected = 1U;
        ug96_shared.QIOPEN_waiting = AT_FALSE;
        retval = ATACTION_RSP_FRC_END;
      }
      else
      {
        if (err_value != 0U)
        {
          PrintErr("+QIOPEN returned error #%ld", err_value)
        }
        else
        {
          PrintErr("+QIOPEN problem")
        }
        retval = ATACTION_RSP_ERROR;
      }
    }
    else
    {
      /* param ignored */
    }
    END_PARAM_LOOP()
  }
  else
  {
    PrintINFO("+QIOPEN not expected, ignore it")
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_QIRD_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIRD_UG96()")

  /*IP AT Commands manual - LTE Module Series - V1.0
   *
   * Received after having send AT+QIRD=<connectID>[,<read_length>]
   *
   * if <read_length> was present and equal to 0, it was a request to get status:
   * +QIRD:<total_receive_length>,<have_read_length>,<unread_length>
   *
   * if <read_length> was absent or != 0
   * +QIRD:<read_actual_length><CR><LF><data>
   *
   */
  if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestSize)
  {
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      /* <total_receive_length> */
      PrintINFO("+QIRD: total_receive_length = %ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <have_read_length> */
      PrintINFO("+QIRD: have_read_length = %ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <unread_length> */
      uint32_t buff_in = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
      PrintINFO("+QIRD: unread_length = %ld", buff_in)
      /* this is the size we have to request to read */
      p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size = buff_in;
    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }
  else if (p_modem_ctxt->socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Header)
  {
    START_PARAM_LOOP()
    /* Receiving socket DATA HEADER which include data size received */
    if (element_infos->param_rank == 2U)
    {
      /* <read_actual_length> */
      PrintINFO("+QIRD: received data size = %ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      /* NOTE !!! the size is purely informative in current implementation
      *  indeed, due to real time constraints, the socket data header is analyzed directly in ATCustom_UG96_checkEndOfMsgCallback()
      */
      /* data header analyzed, ready to analyze data payload */
      p_modem_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Payload;
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <remoteIP> */
      (void) memset((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value[0],
                    0, MAX_IP_ADDR_SIZE);
      (void) memcpy((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
      PrintINFO("+QIRD: remote IP address = %s", p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value)

      /* determine IP address type */
      p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_type = atcm_get_ip_address_type((AT_CHAR_t *)&p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value);
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <remotePort> */
      p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port = (uint16_t) ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                            element_infos->str_size);
      PrintINFO("+QIRD: remote port = %d", p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port)
    }
    else { /* nothing to do */ }
    END_PARAM_LOOP()
  }
  else
  {
    PrintErr("+QIRD: should not fall here")
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_QIRD_data_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QIRD_data_UG96()")

  PrintDBG("DATA received: size=%ld vs %d", p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size,
           element_infos->str_size)
  if (p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv != NULL)
  {
    /* recopy data to client buffer */
    (void) memcpy((void *)p_modem_ctxt->socket_ctxt.socketReceivedata.p_buffer_addr_rcv,
                  (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                  (size_t) element_infos->str_size);
    p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size = element_infos->str_size;
  }
  else
  {
    PrintErr("ERROR, receive buffer is a NULL ptr !!!")
    retval = ATACTION_RSP_ERROR;
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_QISTATE_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PrintAPI("enter fRspAnalyze_QISTATE_UG96()")
  at_bool_t ug96_qistate_for_requested_socket = AT_FALSE;

  /* format:
   * +QISTATE:<connectID>,<service_type>,<IP_adress>,<remote_port>,<local_port>,<socket_state>,<contextID>,<serverID>,<access_mode>,<AT_port>
   *
   * where:
   *
   * exple: +QISTATE: 0,“TCP”,“220.180.239.201”,8705,65514,2,1,0,0,“usbmodem”
   */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* <connId> */
    uint32_t connId = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    socket_handle_t sockHandle = atcm_socket_get_socket_handle(p_modem_ctxt, connId);
    /* if this connection ID corresponds to requested socket handle, we will report the following infos */
    if (sockHandle == p_modem_ctxt->socket_ctxt.socket_cnx_infos->socket_handle)
    {
      ug96_qistate_for_requested_socket = AT_TRUE;
    }
    else
    {
      ug96_qistate_for_requested_socket = AT_FALSE;
    }
    PrintDBG("+QISTATE: <connId>=%ld (requested=%d)", connId, ((ug96_qistate_for_requested_socket == AT_TRUE) ? 1 : 0))
  }
  else if (element_infos->param_rank == 3U)
  {
    /* <service_type> */
    AT_CHAR_t serviceType[16] = {0};
    (void) memcpy((void *)&serviceType[0],
                  (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                  (size_t) element_infos->str_size);
    PrintDBG("+QISTATE: <service_type>=%s", serviceType)
  }
  else if (element_infos->param_rank == 4U)
  {
    /* <IP_adress> */
    AT_CHAR_t remIP[MAX_IP_ADDR_SIZE];
    (void) memset((void *)remIP, 0, MAX_IP_ADDR_SIZE);
    (void) memcpy((void *)&remIP[0],
                  (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                  (size_t) element_infos->str_size);
    PrintDBG("+QISTATE: <remote IP_adress>=%s", remIP)
    if (ug96_qistate_for_requested_socket == AT_TRUE)
    {
      uint16_t src_idx;
      uint16_t dest_idx = 0U;
      for (src_idx = element_infos->str_start_idx; src_idx <= element_infos->str_end_idx; src_idx++)
      {
        if (p_msg_in->buffer[src_idx] != 0x22U) /* remove quotes from the string */
        {
          p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_ip_addr_value[dest_idx] = p_msg_in->buffer[src_idx];
          dest_idx++;
        }
      }
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    /* <remote_port> */
    uint32_t remPort = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <remote_port>=%ld", remPort)
    if (ug96_qistate_for_requested_socket == AT_TRUE)
    {
      p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_port = (uint16_t) remPort;
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    /* <local_port> */
    uint32_t locPort = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("+QISTATE: <local_port>=%ld", locPort)
    if (ug96_qistate_for_requested_socket == AT_TRUE)
    {
      p_modem_ctxt->socket_ctxt.socket_cnx_infos->infos->rem_port = (uint16_t) locPort;
    }
  }
  else if (element_infos->param_rank == 7U)
  {
    /*<socket_state> */
    PrintDBG("+QISTATE: <socket_state>=%ld",
             ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
  }
  else if (element_infos->param_rank == 8U)
  {
    /* <contextID> */
    PrintDBG("+QISTATE: <contextID>=%ld",
             ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
  }
  else if (element_infos->param_rank == 9U)
  {
    /* <serverID> */
    PrintDBG("+QISTATE: <serverID>=%ld",
             ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
  }
  else if (element_infos->param_rank == 10U)
  {
    /* <access_mode> */
    PrintDBG("+QISTATE: <access_mode>=%ld",
             ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
  }
  else if (element_infos->param_rank == 11U)
  {
    /* <AT_port> */
    AT_CHAR_t _ATport[16] = {0};
    (void) memcpy((void *)&_ATport[0],
                  (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                  (size_t) element_infos->str_size);
    PrintDBG("+QISTATE: <AT_port>=%s", _ATport)
  }
  else
  {
    /* parameter ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_QPING_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);

  at_action_rsp_t retval = ATACTION_RSP_URC_FORWARDED;
  PrintAPI("enter fRspAnalyze_QPING_UG96()")

  /* intermediate ping response format:
  * +QPING: <result>[,<IP_address>,<bytes>,<time>,<ttl>]
  *
  * last ping reponse format:
  * +QPING: <finresult>[,<sent>,<rcvd>,<lost>,<min>,<max>,<avg>]
  */

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* new ping response: clear ping response structure */
    clear_ping_resp_struct(p_modem_ctxt);

    /* <result> or <finresult> */
    uint32_t result = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
    PrintDBG("Ping result = %ld", result)

    p_modem_ctxt->persist.urc_avail_ping_rsp = AT_TRUE;
    p_modem_ctxt->persist.ping_resp_urc.index++;
    p_modem_ctxt->persist.ping_resp_urc.ping_status = (result == 0U) ? CELLULAR_OK : CELLULAR_ERROR;
  }
  else if (element_infos->param_rank == 3U)
  {
    /* check if this is an intermediate report or the final report */
    if (p_msg_in->buffer[element_infos->str_start_idx] == 0x22U)
    {
      /* this is an IP address: intermediate ping response */
      if (p_modem_ctxt->persist.ping_resp_urc.is_final_report == CELLULAR_TRUE)
      {
        PrintDBG("intermediate ping report")
        p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_FALSE;
      }
    }
    else
    {
      /* this is not an IP adress: final ping response */
      if (p_modem_ctxt->persist.ping_resp_urc.is_final_report == CELLULAR_FALSE)
      {
        PrintDBG("final ping report")
        p_modem_ctxt->persist.ping_resp_urc.is_final_report = CELLULAR_TRUE;
      }
    }

    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <IP_address> */
      AT_CHAR_t pingIP[MAX_IP_ADDR_SIZE];
      (void) memset((void *)pingIP, 0, MAX_IP_ADDR_SIZE);
      (void) memcpy((void *)&pingIP[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
      PrintDBG("+QIPING: <ping IP_adress>=%s", pingIP)
      uint16_t src_idx, dest_idx = 0U;
      for (src_idx = element_infos->str_start_idx; src_idx <= element_infos->str_end_idx; src_idx++)
      {
        if (p_msg_in->buffer[src_idx] != 0x22U) /* remove quotes from the string */
        {
          p_modem_ctxt->persist.ping_resp_urc.ping_addr[dest_idx] = p_msg_in->buffer[src_idx];
          dest_idx++;
        }
      }
    }
    else
    {
      /* <sent> */
      uint32_t packets_sent = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.packets_sent = (uint8_t)packets_sent;
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <bytes> */
      uint32_t bytes = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                 element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.bytes = (uint16_t)bytes;
    }
    else
    {
      /* <rcvd> */
      uint32_t packets_rcvd = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.packets_rcvd = (uint8_t)packets_rcvd;
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <time>*/
      uint32_t timeval = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                   element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.time = (uint16_t)timeval;
    }
    else
    {
      /* <lost> */
      uint32_t packets_lost = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.packets_lost = (uint8_t)packets_lost;
    }
  }
  else if (element_infos->param_rank == 6U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_FALSE)
    {
      /* <ttl>*/
      uint32_t ttl = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.ttl = (uint16_t)ttl;
    }
    else
    {
      /* <min_time>*/
      uint32_t min_time = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.min_time = (uint16_t)min_time;
    }
  }
  else if (element_infos->param_rank == 7U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_TRUE)
    {
      /* <max_time>*/
      uint32_t max_time = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.max_time = (uint16_t)max_time;
    }
  }
  else if (element_infos->param_rank == 8U)
  {
    if (p_modem_ctxt->persist.ping_resp_urc.is_final_report  == CELLULAR_TRUE)
    {
      /* <avg_time>*/
      uint32_t avg_time = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
      p_modem_ctxt->persist.ping_resp_urc.avg_time = (uint16_t)avg_time;
    }
  }
  else
  {
    /* parameter ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

/* Private function Definition ---------------------------------------------- */
static void clear_ping_resp_struct(atcustom_modem_context_t *p_modem_ctxt)
{
  /* clear all parameters exxcept the index:
   * save the index, reset the structure and recopy saved index
   */
  uint8_t saved_idx = p_modem_ctxt->persist.ping_resp_urc.index;
  (void) memset((void *)&p_modem_ctxt->persist.ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  p_modem_ctxt->persist.ping_resp_urc.index = saved_idx;
}

#endif /* USE_MODEM_UG96 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

