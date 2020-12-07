#ifdef USE_MODEM_UG96
/**
  ******************************************************************************
  * @file    at_custom_modem_signalling.h
  * @author  MCD Application Team
  * @brief   Header for at_custom_modem_signalling.c module for UG96
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_CUSTOM_SIGNALLING_UG96_H
#define AT_CUSTOM_SIGNALLING_UG96_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"
#include "at_parser.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_custom_modem_specific_ug96.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/* UG96 build commands overriding common function  or specific */
at_status_t fCmdBuild_ATD_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_CGSN_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QPOWD_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QCFG_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);
at_status_t fCmdBuild_QICSGP_UG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt);

/* UG96 specific analyze commands */
at_action_rsp_t fRspAnalyze_Error_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CPIN_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_CFUN_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QIND_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QCFG_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QIURC_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QCCID_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
at_action_rsp_t fRspAnalyze_QUSIM_UG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos);
#ifdef __cplusplus
}
#endif

#endif /* AT_CUSTOM_SIGNALLING_UG96_H */

#endif /* USE_MODEM_UG96 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
