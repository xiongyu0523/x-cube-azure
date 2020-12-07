/**
  ******************************************************************************
  * @file    cellular_service_task.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Task
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
#include "dc_common.h"

#include "cellular_init.h"
#include "at_util.h"
#include "cellular_service.h"
#include "cellular_service_os.h"
#include "cellular_service_config.h"
#include "cellular_service_task.h"
#include "error_handler.h"
#include "plf_config.h"
#include "cellular_service_int.h"
#include "cellular_runtime_custom.h"

#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif  /* (USE_CMD_CONSOLE == 1) */

/* Private defines -----------------------------------------------------------*/
/* Test FOTA FEATURE */
#define CST_FOTA_TEST        (0)

/* Test NFMC activation */
#define CST_TEST_NFMC        (0)
#if (CST_TEST_NFMC == 1)
static uint32_t CST_nfmc_test_count = 0;
#define CST_NFMC_TEST_COUNT_MAX 9
#endif /* (CST_TEST_NFMC == 1) */

#define CST_TEST_REGISTER_FAIL        (0)
#if (CST_TEST_REGISTER_FAIL == 1)
static uint32_t CST_register_fail_test_count = 0;
#define CST_REGISTER_FAIL_TEST_COUNT_MAX 2
#endif /* (CST_TEST_REGISTER_FAIL == 1) */

#define CST_COUNT_FAIL_MAX (5U)

#define GOOD_PINCODE ((uint8_t *)"") /* SET PIN CODE HERE (for exple "1234"), if no PIN code, use an string empty "" */
#define CST_MODEM_POLLING_PERIOD_DEFAULT 5000U

#define CST_BAD_SIG_RSSI 99U

#define CST_POWER_ON_RESET_MAX      5U
#define CST_RESET_MAX               5U
#define CST_INIT_MODEM_RESET_MAX    5U
#define CST_CSQ_MODEM_RESET_MAX     5U
#define CST_GNS_MODEM_RESET_MAX     5U
#define CST_ATTACH_RESET_MAX        5U
#define CST_DEFINE_PDN_RESET_MAX    5U
#define CST_ACTIVATE_PDN_RESET_MAX  5U
#define CST_CELLULAR_DATA_RETRY_MAX 5U
#define CST_SIM_RETRY_MAX           (uint16_t)DC_SIM_SLOT_NB
#define CST_GLOBAL_RETRY_MAX        5U
#define CST_RETRY_MAX               5U
#define CST_CMD_RESET_MAX         100U

#define CST_PDN_ACTIVATE_RETRY_DELAY 30000U
#define CST_NETWORK_STATUS_DELAY     180000U

#define CST_FOTA_TIMEOUT      (360000U) /* 6 min (calibrated for cat-M1 network, increase it for cat-NB1) */


#define CST_SIM_POLL_COUNT     200U    /* 20s */

#define CST_RESET_DEBUG (0)

/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "", ## args)
#else
#include <stdio.h>
#define PrintForce(format, args...)                printf(format , ## args);
#endif  /* (USE_PRINTF == 1) */
#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#define PrintCellularService(format, args...)      TracePrint(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, format, ## args)
#define PrintCellularServiceErr(format, args...)   TracePrint(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_ERR, "ERROR " format, ## args)
#else
#define PrintCellularService(format, args...)      printf(format , ## args);
#define PrintCellularServiceErr(format, args...)   printf(format , ## args);
#endif /* (USE_PRINTF == 0U) */
#else
#define PrintCellularService(format, args...)   do {} while(0);
#define PrintCellularServiceErr(format, args...)  do {} while(0);
#endif /* (USE_TRACE_CELLULAR_SERVICE == 1U) */

/* Private typedef -----------------------------------------------------------*/

typedef enum
{
  CST_MESSAGE_CS_EVENT    = 0,
  CST_MESSAGE_DC_EVENT    = 1,
  CST_MESSAGE_URC_EVENT   = 2,
  CST_MESSAGE_TIMER_EVENT = 3,
  CST_MESSAGE_CMD         = 4
} CST_message_type_t;

typedef uint16_t CST_autom_event_t;


#define CST_OFF_EVENT                              0U
#define CST_INIT_EVENT                             1U
#define CST_MODEM_POWER_ON_EVENT                   2U
#define CST_MODEM_POWERED_ON_EVENT                 3U
#define CST_MODEM_INITIALIZED_EVENT                4U
#define CST_NETWORK_CALLBACK_EVENT                 5U
#define CST_SIGNAL_QUALITY_EVENT                   6U
#define CST_NW_REG_TIMEOUT_TIMER_EVENT             7U
#define CST_NETWORK_STATUS_EVENT                   8U
#define CST_NETWORK_STATUS_OK_EVENT                9U
#define CST_MODEM_ATTACHED_EVENT                  10U
#define CST_PDP_ACTIVATED_EVENT                   11U
#define CST_MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT  12U
#define CST_PDN_STATUS_EVENT                      13U
#define CST_CELLULAR_DATA_FAIL_EVENT              14U
#define CST_FAIL_EVENT                            15U
#define CST_POLLING_TIMER                         16U
#define CST_MODEM_URC                             17U
#define CST_NO_EVENT                              18U
#define CST_CMD_UNKWONW_EVENT                     19U
#define CST_CELLULAR_STATE_EVENT           20U


typedef enum
{
  CST_NO_FAIL,
  CST_MODEM_POWER_ON_FAIL,
  CST_MODEM_RESET_FAIL,
  CST_MODEM_CSQ_FAIL,
  CST_MODEM_GNS_FAIL,
  CST_MODEM_REGISTER_FAIL,
  CST_MODEM_ATTACH_FAIL,
  CST_MODEM_PDP_DEFINE_FAIL,
  CST_MODEM_PDP_ACTIVATION_FAIL,
  CST_MODEM_CELLULAR_DATA_FAIL,
  CST_MODEM_SIM_FAIL,
  CST_MODEM_CMD_FAIL
} CST_fail_cause_t;

typedef struct
{
  uint16_t  type ;
  uint16_t  id  ;
} CST_message_t;

typedef struct
{
  CST_state_t          current_state;
  CST_fail_cause_t     fail_cause;
  CS_PDN_event_t       pdn_status;
  CS_SignalQuality_t   signal_quality;
  CS_NetworkRegState_t current_EPS_NetworkRegState;
  CS_NetworkRegState_t current_GPRS_NetworkRegState;
  CS_NetworkRegState_t current_CS_NetworkRegState;
  uint16_t             activate_pdn_nfmc_tempo_count;
  uint16_t             register_retry_tempo_count;
  uint16_t             power_on_reset_count ;
  uint16_t             reset_reset_count ;
  uint16_t             init_modem_reset_count ;
  uint16_t             csq_reset_count ;
  uint16_t             gns_reset_count ;
  uint16_t             attach_reset_count ;
  uint16_t             activate_pdn_reset_count ;
  uint16_t             sim_reset_count ;
  uint16_t             cellular_data_retry_count ;
  uint16_t             cmd_reset_count ;
  uint16_t             reset_count ;
  uint16_t             global_retry_count ;
} CST_context_t;

typedef struct
{
  uint32_t  active;
  uint32_t  tempo[CST_NFMC_TEMPO_NB];
} CST_nfmc_context_t;

/* Private variables ---------------------------------------------------------*/
static osMessageQId      cst_queue_id;
static osTimerId         CST_pdn_activate_retry_timer_handle;
static osTimerId         CST_network_status_timer_handle;
static osTimerId         CST_register_retry_timer_handle;
static osTimerId         CST_fota_timer_handle;
static dc_cellular_info_t      cst_cellular_info;
static dc_sim_info_t           cst_sim_info;
static dc_cellular_data_info_t cst_cellular_data_info;
static dc_cellular_params_t    cst_cellular_params;
static uint8_t cst_sim_slot_index;

static CST_nfmc_context_t CST_nfmc_context;
static CST_context_t     cst_context = {    CST_MODEM_INIT_STATE, CST_NO_FAIL, CS_PDN_EVENT_NW_DETACH,    /* Automaton State, FAIL Cause,  */
  { 0, 0},                              /* signal quality */
  CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING, CS_NRS_NOT_REGISTERED_NOT_SEARCHING,
  0,                                     /* activate_pdn_nfmc_tempo_count */
  0,                                     /* register_retry_tempo_count */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0        /* Reset counters */
};

static CS_OperatorSelector_t    ctxt_operator =
{
  .mode = CS_NRM_AUTO,
  .format = CS_ONF_NOT_PRESENT,
  .operator_name = "00101",
};

static uint8_t CST_polling_timer_flag = 0U;
static uint8_t CST_csq_count_fail      = 0U;

/* Global variables ----------------------------------------------------------*/
CST_state_t CST_current_state;
uint8_t CST_polling_active;


/* Private function prototypes -----------------------------------------------*/
static void  CST_send_message(CST_message_type_t  type, CST_autom_event_t cmd);
static void CST_reset_fail_count(void);
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event);
static void CST_network_reg_callback(void);
static void CST_modem_event_callback(CS_ModemEvent_t event);
static void CST_location_info_callback(void);
static void  CST_data_cache_set(dc_service_rt_state_t dc_service_state);
static void CST_location_info_callback(void);
static void CST_config_fail_mngt(const uint8_t *msg_fail, CST_fail_cause_t fail_cause, uint16_t *fail_count,
                                 uint16_t fail_max);
static void CST_modem_init(void);
static CS_Status_t CST_set_signal_quality(void);
static void CST_get_device_all_infos(dc_cs_target_state_t  target_state);
static void CST_subscribe_all_net_events(void);
static void CST_subscribe_modem_events(void);
static void CST_notif_cb(dc_com_event_id_t dc_event_id, const void *private_gui_data);
static CST_autom_event_t CST_get_autom_event(osEvent event);
static void CST_power_on_only_modem_mngt(void);
static void CST_power_on_modem_mngt(void);
static CS_Status_t CST_reset_modem(void);
static void CST_reset_modem_mngt(void);
static void CST_init_modem_mngt(void);
static void CST_net_register_mngt(void);
static void CST_signal_quality_test_mngt(void);
static void CST_network_status_test_mngt(void);
static void CST_network_event_mngt(void);
static void CST_attach_modem_mngt(void);
static void CST_modem_define_pdn(void);
static CS_Status_t CST_modem_activate_pdn(void);
static void CST_cellular_data_fail_mngt(void);
static void CST_pdn_event_mngt(void);
static void CST_init_state(CST_autom_event_t autom_event);
static void CST_reset_state(CST_autom_event_t autom_event);
static void CST_modem_on_state(CST_autom_event_t autom_event);
static void CST_modem_powered_on_state(CST_autom_event_t autom_event);
static void CST_waiting_for_signal_quality_ok_state(CST_autom_event_t autom_event);
static void CST_waiting_for_network_status_state(CST_autom_event_t autom_event);
static void CST_network_status_ok_state(CST_autom_event_t autom_event);
static void CST_modem_registered_state(CST_autom_event_t autom_event);
static void CST_modem_pdn_activate_state(CST_autom_event_t autom_event);
static void CST_data_ready_state(CST_autom_event_t autom_event);
static void CST_fail_state(CST_autom_event_t autom_event);
static void CST_timer_handler(void);
static void CST_cellular_service_task(void const *argument);
static void CST_polling_timer_callback(void const *argument);
static void CST_pdn_activate_retry_timer_callback(void const *argument);
static void CST_network_status_timer_callback(void const *argument);
static void CST_register_retry_timer_callback(void const *argument);
static void CST_fota_timer_callback(void const *argument);
static uint8_t CST_util_convertStringToInt64(const uint8_t *p_string, uint16_t size, uint32_t *high_part_value,
                                             uint32_t *low_part_value);
static uint32_t CST_calculate_tempo_value(uint32_t value, uint32_t imsi_high, uint32_t imsi_low);
static void CST_fill_nfmc_tempo(uint32_t imsi_high, uint32_t imsi_low);
static void CST_modem_start(void);
static CS_SimSlot_t cst_convert_sim_socket_type(dc_cs_sim_slot_type_t sim_slot_value);

/* Private function Definition -----------------------------------------------*/


static CS_SimSlot_t  cst_convert_sim_socket_type(dc_cs_sim_slot_type_t sim_slot_value)
{
  CS_SimSlot_t enum_value;
  switch (sim_slot_value)
  {
    case DC_SIM_SLOT_MODEM_SOCKET:
    {
      enum_value = CS_MODEM_SIM_SOCKET_0;
      break;
    }
    case DC_SIM_SLOT_MODEM_EMBEDDED_SIM:
    {
      enum_value = CS_MODEM_SIM_ESIM_1;
      break;
    }
    case DC_SIM_SLOT_STM32_EMBEDDED_SIM:
    {
      enum_value = CS_STM32_SIM_2;
      break;
    }
    default:
    {
      enum_value = CS_MODEM_SIM_SOCKET_0;
      break;
    }
  }
  return enum_value;
}

static void CST_reset_fail_count(void)
{
  cst_context.power_on_reset_count       = 0U;
  cst_context.reset_reset_count          = 0U;
  cst_context.init_modem_reset_count     = 0U;
  cst_context.csq_reset_count            = 0U;
  cst_context.gns_reset_count            = 0U;
  cst_context.attach_reset_count         = 0U;
  cst_context.activate_pdn_reset_count   = 0U;
  cst_context.cellular_data_retry_count  = 0U;
  cst_context.global_retry_count         = 0U;
}

/* PDN event callback */
static void CST_pdn_event_callback(CS_PDN_conf_id_t cid, CS_PDN_event_t pdn_event)
{
  UNUSED(cid);
  PrintCellularService("====================================CST_pdn_event_callback (cid=%d / event=%d)\n\r",
                       cid, pdn_event)
  cst_context.pdn_status = pdn_event;
  (void)CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDN_STATUS_EVENT);

}

/* URC callback */
static void CST_network_reg_callback(void)
{
  PrintCellularService("==================================CST_network_reg_callback\n\r")
  CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
}

static void CST_location_info_callback(void)
{
  PrintCellularService("CST_location_info_callback\n\r")
}

static void CST_modem_event_callback(CS_ModemEvent_t event)
{
  /* event is a bitmask, we can have more than one evt reported at the same time */
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_BOOT) != 0U)
  {
    PrintCellularService("Modem event received: CS_MDMEVENT_BOOT\n\r")
  }
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_POWER_DOWN) != 0U)
  {
    PrintCellularService("Modem event received:  CS_MDMEVENT_POWER_DOWN\n\r")
  }
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_FOTA_START) != 0U)
  {
    PrintCellularService("Modem event received:  CS_MDMEVENT_FOTA_START\n\r")
    (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                      sizeof(cst_cellular_data_info));
    cst_cellular_data_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
    CST_current_state = CST_MODEM_REPROG_STATE;
    (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                       sizeof(cst_cellular_data_info));
    (void)osTimerStart(CST_fota_timer_handle, CST_FOTA_TIMEOUT);
  }
  if (((uint16_t)event & (uint16_t)CS_MDMEVENT_FOTA_END) != 0U)
  {
    PrintCellularService("Modem event received:  CS_MDMEVENT_FOTA_END\n\r")

    /* TRIGGER PLATFORM RESET after a delay  */
    PrintCellularService("TRIGGER PLATFORM REBOOT AFTER FOTA UPDATE ...\n\r")
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 4, ERROR_FATAL);
  }
}

static void  CST_data_cache_set(dc_service_rt_state_t dc_service_state)
{
  (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(cst_cellular_data_info));
  cst_cellular_data_info.rt_state = dc_service_state;
  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                     sizeof(cst_cellular_data_info));
}

/* failing configuration management */
static void CST_config_fail_mngt(const uint8_t *msg_fail, CST_fail_cause_t fail_cause, uint16_t *fail_count,
                                 uint16_t fail_max)
{

#if (SW_DEBUG_VERSION == 0)
  UNUSED(msg_fail);
#endif  /* (SW_DEBUG_VERSION == 0) */

  PrintCellularService("=== %s Fail !!! === \r\n", msg_fail)
  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 7, ERROR_WARNING);

  *fail_count = *fail_count + 1U;
  cst_context.global_retry_count++;
  cst_context.reset_count++;

  CST_data_cache_set(DC_SERVICE_OFF);
  if ((*fail_count <= fail_max) && (cst_context.global_retry_count <= CST_GLOBAL_RETRY_MAX))
  {
    CST_current_state = CST_MODEM_RESET_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_INIT_EVENT);
    cst_context.fail_cause    = fail_cause;
  }
  else
  {
    CST_current_state = CST_MODEM_FAIL_STATE;
    cst_context.fail_cause    = CST_MODEM_POWER_ON_FAIL;

    PrintCellularServiceErr("=== CST_set_fail_state %d - count %d/%d FATAL !!! ===\n\r", fail_cause, cst_context.global_retry_count, *fail_count)
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 8, ERROR_FATAL);
  }
}

/* init modem processing */
static void CST_modem_init(void)
{
  (void)CS_init();
}

/* start modem processing */
static void CST_modem_start(void)
{
  if (atcore_task_start(ATCORE_THREAD_STACK_PRIO, ATCORE_THREAD_STACK_SIZE) != ATSTATUS_OK)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 9, ERROR_WARNING);
  }
}

/* init modem processing */
static CS_Status_t CST_set_signal_quality(void)
{
  CS_Status_t cs_status = CELLULAR_ERROR;
  CS_SignalQuality_t sig_quality;
  if (osCS_get_signal_quality(&sig_quality) == CELLULAR_OK)
  {
    CST_csq_count_fail = 0U;
    if ((sig_quality.rssi != cst_context.signal_quality.rssi) || (sig_quality.ber != cst_context.signal_quality.ber))
    {
      cst_context.signal_quality.rssi = sig_quality.rssi;
      cst_context.signal_quality.ber  = sig_quality.ber;

      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));


      /* if((sig_quality.rssi == 0) || (sig_quality.rssi == CST_BAD_SIG_RSSI)) */
      if (sig_quality.rssi == CST_BAD_SIG_RSSI)
      {
        cst_cellular_info.cs_signal_level    = DC_NO_ATTACHED;
        cst_cellular_info.cs_signal_level_db = (int32_t)DC_NO_ATTACHED;
      }
      else
      {
        cs_status = CELLULAR_OK;
        cst_cellular_info.cs_signal_level     = sig_quality.rssi;             /*  range 0..99 */
        cst_cellular_info.cs_signal_level_db  = (-113 + (2 * (int32_t)sig_quality.rssi)); /* dBm */
      }
      (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
    }

    PrintCellularService(" -Sig quality rssi : %d\n\r", sig_quality.rssi)
    PrintCellularService(" -Sig quality ber  : %d\n\r", sig_quality.ber)
  }
  else
  {
    CST_csq_count_fail++;
    PrintCellularService("Modem signal quality error\n\r")
    if (CST_csq_count_fail >= CST_COUNT_FAIL_MAX)
    {
      PrintCellularService("Modem signal quality error max\n\r")
      CST_csq_count_fail = 0U;
      CST_config_fail_mngt(((uint8_t *)"CS_get_signal_quality"),
                           CST_MODEM_CSQ_FAIL,
                           &cst_context.csq_reset_count,
                           CST_CSQ_MODEM_RESET_MAX);
    }
  }
  return cs_status;
}

/* send message to cellular service task */
static void  CST_send_message(CST_message_type_t  type, CST_autom_event_t event)
{
  CST_message_t cmd_message;
  cmd_message.type = (uint16_t)type;
  cmd_message.id   = event;

  uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);
  (void)osMessagePut((osMessageQId)cst_queue_id, *cmd_message_p, 0U);
}


static uint32_t cst_modulo64(uint32_t div, uint32_t val_m, uint32_t val_l)
{
  uint32_t div_m;
  uint32_t div_l;
  uint32_t tmp_m;
  uint32_t tmp_l;

  div_m = div;
  div_l = 0;
  tmp_m = val_m % div_m;

  tmp_l = val_l;

  while (tmp_m > 0U)
  {
    if (
      (div_m > tmp_m)
      ||
      ((div_m == tmp_m) && (div_l > tmp_l))
    )
    {
      /* Nothing to do */
    }
    else if ((div_m <= tmp_m) && (div_l > tmp_l))
    {
      tmp_l = tmp_l - div_l;
      tmp_m--;
      tmp_m = tmp_m - div_m;
    }
    else if ((div_m <= tmp_m) && (div_l <= tmp_l))
    {
      tmp_m = tmp_m - div_m;
      tmp_l = tmp_l - div_l;
    }
    else
    {
      /* Nothing to do */
    }
    div_l = div_l >> 1;
    if ((div_m & 1U) == 1U)
    {
      div_l = div_l | 0x80000000U;
    }
    div_m = div_m >> 1;
  }
  tmp_l = tmp_l % div;
  return tmp_l;
}

static uint8_t CST_util_convertStringToInt64(const uint8_t *p_string, uint16_t size, uint32_t *high_part_value,
                                             uint32_t *low_part_value)
{
  return ATutil_convertHexaStringToInt64(p_string, size, high_part_value, low_part_value);
}

static uint32_t CST_calculate_tempo_value(uint32_t value, uint32_t imsi_high, uint32_t imsi_low)
{
  uint32_t temp_value32;
  if (value != 0U)
  {
    temp_value32 = cst_modulo64(value, imsi_high, imsi_low);
  }
  else
  {
    temp_value32 = imsi_low;
  }
  temp_value32 = temp_value32 + value;
  return (0xffffffffU & temp_value32);
}

static void CST_fill_nfmc_tempo(uint32_t imsi_high, uint32_t imsi_low)
{
  uint32_t i;
  dc_nfmc_info_t nfmc_info;

  if (cst_cellular_params.nfmc_active != 0U)
  {
    nfmc_info.activate = 1U;
    for (i = 0U ; i < CST_NFMC_TEMPO_NB; i++)
    {
      CST_nfmc_context.tempo[i] = CST_calculate_tempo_value(cst_cellular_params.nfmc_value[i], imsi_high, imsi_low);
      nfmc_info.tempo[i] = CST_nfmc_context.tempo[i];
      PrintCellularService("VALUE/TEMPO %ld/%ld\n\r",  cst_cellular_params.nfmc_value[i], CST_nfmc_context.tempo[i])
    }
    nfmc_info.rt_state = DC_SERVICE_ON;
  }
  else
  {
    nfmc_info.activate = 0U;
    nfmc_info.rt_state = DC_SERVICE_OFF;
  }
  (void)dc_com_write(&dc_com_db, DC_COM_NFMC_TEMPO, (void *)&nfmc_info, sizeof(nfmc_info));

}

/* update device info in data cache */
static void CST_get_device_all_infos(dc_cs_target_state_t  target_state)
{
  static CS_DeviceInfo_t cst_device_info;
  CS_Status_t            cs_status;
  uint16_t               sim_poll_count;
  uint16_t               end_of_loop;
  uint32_t               CST_IMSI_high;
  uint32_t               CST_IMSI_low;

  sim_poll_count = 0U;
  (void)memset((void *)&cst_device_info, 0, sizeof(CS_DeviceInfo_t));

  (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));


  cst_device_info.field_requested = CS_DIF_IMEI_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy(cst_cellular_info.imei, cst_device_info.u.imei, DC_MAX_SIZE_IMEI);
    PrintCellularService(" -IMEI: %s\n\r", cst_device_info.u.imei)
  }
  else
  {
    cst_cellular_info.imei[0] = 0U;
    PrintCellularService("IMEI error\n\r")
  }


  cst_device_info.field_requested = CS_DIF_MANUF_NAME_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.manufacturer_name,
                 (CRC_CHAR_t *)cst_device_info.u.manufacturer_name,
                 DC_MAX_SIZE_MANUFACT_NAME);
    PrintCellularService(" -MANUFACTURER: %s\n\r", cst_device_info.u.manufacturer_name)
  }
  else
  {
    cst_cellular_info.manufacturer_name[0] = 0U;
    PrintCellularService("Manufacturer Name error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_MODEL_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.model,
                 (CRC_CHAR_t *)cst_device_info.u.model,
                 DC_MAX_SIZE_MODEL);
    PrintCellularService(" -MODEL: %s\n\r", cst_device_info.u.model)
  }
  else
  {
    cst_cellular_info.model[0] = 0U;
    PrintCellularService("Model error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_REV_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.revision,
                 (CRC_CHAR_t *)cst_device_info.u.revision,
                 DC_MAX_SIZE_REV);
    PrintCellularService(" -REVISION: %s\n\r", cst_device_info.u.revision)
  }
  else
  {
    cst_cellular_info.revision[0] = 0U;
    PrintCellularService("Revision error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_SN_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.serial_number,
                 (CRC_CHAR_t *)cst_device_info.u.serial_number,
                 DC_MAX_SIZE_SN);
    PrintCellularService(" -SERIAL NBR: %s\n\r", cst_device_info.u.serial_number)
  }
  else
  {
    cst_cellular_info.serial_number[0] = 0U;
    PrintCellularService("Serial Number error\n\r")
  }

  cst_device_info.field_requested = CS_DIF_ICCID_PRESENT;
  if (osCDS_get_device_info(&cst_device_info) == CELLULAR_OK)
  {
    (void)memcpy((CRC_CHAR_t *)cst_cellular_info.iccid,
                 (CRC_CHAR_t *)cst_device_info.u.iccid,
                 DC_MAX_SIZE_ICCID);
    PrintCellularService(" -ICCID: %s\n\r", cst_device_info.u.iccid)
  }
  else
  {
    cst_cellular_info.serial_number[0] = 0U;
    PrintCellularService("Serial Number error\n\r")
  }

  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  end_of_loop = 1U;
  if (target_state == DC_TARGET_STATE_FULL)
  {
    (void)dc_com_read(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
    cst_sim_info.rt_state   = DC_SERVICE_ON;
    cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_CONNECTION_ON_GOING;
    (void)dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

    while (end_of_loop != 0U)
    {
      cst_device_info.field_requested = CS_DIF_IMSI_PRESENT;
      cs_status = osCDS_get_device_info(&cst_device_info);
      if (cs_status == CELLULAR_OK)
      {
        (void)CST_util_convertStringToInt64(cst_device_info.u.imsi, 15U, &CST_IMSI_high, &CST_IMSI_low);
        PrintCellularService(" -IMSI: %lx%lx\n\r", CST_IMSI_high, CST_IMSI_low)
        CST_fill_nfmc_tempo(CST_IMSI_high, CST_IMSI_low);

        (void)memcpy((CRC_CHAR_t *)cst_sim_info.imsi,
                     (CRC_CHAR_t *)cst_device_info.u.imsi,
                     DC_MAX_SIZE_IMSI);
        cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_OK;
        end_of_loop = 0U;
      }
      else if ((cs_status == CELLULAR_SIM_BUSY)
               || (cs_status == CELLULAR_SIM_ERROR))
      {
        (void)osDelay(100U);
        sim_poll_count++;
        if (sim_poll_count > CST_SIM_POLL_COUNT)
        {
          sim_poll_count = 0;
          dc_cs_sim_status_t sim_error;
          if (cs_status == CELLULAR_SIM_BUSY)
          {
            sim_error = DC_SIM_BUSY;
          }
          else
          {
            sim_error = DC_SIM_ERROR;
          }

          cst_sim_info.sim_status[cst_sim_slot_index] = sim_error;

          end_of_loop = 0U;
        }
      }
      else
      {
        if (cs_status == CELLULAR_SIM_NOT_INSERTED)
        {
          cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_NOT_INSERTED;
        }
        else if (cs_status == CELLULAR_SIM_PIN_OR_PUK_LOCKED)
        {
          cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_PIN_OR_PUK_LOCKED;
        }
        else if (cs_status == CELLULAR_SIM_INCORRECT_PASSWORD)
        {
          cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_INCORRECT_PASSWORD;
        }
        else
        {
          cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_ERROR;
        }
        end_of_loop = 0U;
      }
    }
    (void)dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  }
}

/* subscribe to network event */
static void CST_subscribe_all_net_events(void)
{
  PrintCellularService("Subscribe URC events: Network registration\n\r")
  (void)osCDS_subscribe_net_event(CS_URCEVENT_CS_NETWORK_REG_STAT, CST_network_reg_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_GPRS_NETWORK_REG_STAT, CST_network_reg_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_EPS_NETWORK_REG_STAT, CST_network_reg_callback);
  PrintCellularService("Subscribe URC events: Location info\n\r")
  (void)osCDS_subscribe_net_event(CS_URCEVENT_EPS_LOCATION_INFO, CST_location_info_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_GPRS_LOCATION_INFO, CST_location_info_callback);
  (void)osCDS_subscribe_net_event(CS_URCEVENT_CS_LOCATION_INFO, CST_location_info_callback);
}

/* subscribe to modem event */
static void CST_subscribe_modem_events(void)
{
  PrintCellularService("Subscribe modems events\n\r")
  CS_ModemEvent_t events_mask = (CS_ModemEvent_t)((uint16_t)CS_MDMEVENT_BOOT |
                                                  (uint16_t)CS_MDMEVENT_POWER_DOWN |
                                                  (uint16_t)CS_MDMEVENT_FOTA_START |
                                                  (uint16_t)CS_MDMEVENT_FOTA_END);
  (void)osCDS_subscribe_modem_event(events_mask, CST_modem_event_callback);
}

/* Data cache callback */
static void CST_notif_cb(dc_com_event_id_t dc_event_id, const void *private_gui_data)
{
  UNUSED(private_gui_data);
  if (dc_event_id == DC_COM_CELLULAR_DATA_INFO)
  {
    CST_message_t cmd_message;
    uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);
    cmd_message.type = (uint16_t)CST_MESSAGE_DC_EVENT;
    cmd_message.id   = (uint16_t)dc_event_id;
    (void)osMessagePut(cst_queue_id, *cmd_message_p, 0U);
  }
  else if (dc_event_id == DC_CELLULAR_TARGET_STATE_CMD)
  {
    CST_message_t cmd_message;
    uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);
    cmd_message.type = (uint16_t)CST_MESSAGE_DC_EVENT;
    cmd_message.id   = (uint16_t)dc_event_id;
    (void)osMessagePut(cst_queue_id, *cmd_message_p, 0U);
  }
}

/* Convert received message to automation event */
static CST_autom_event_t CST_get_autom_event(osEvent event)
{
  static dc_cellular_target_state_t cst_target_state;
  CST_autom_event_t autom_event;
  CST_message_t  message;
  CST_message_t *message_p;
  autom_event = CST_NO_EVENT;
  message_p = (CST_message_t *) & (event.value.v);
  message   = *message_p;

  /* 4 types of possible messages: */
  /*
       -> CS automaton event
       -> CS CMD    (ON/OFF)
       -> DC EVENT  (DC_COM_CELLULAR_DATA_INFO: / FAIL)
       -> DC TIMER  (Polling handle)
  */
  if (message.type == (uint16_t)CST_MESSAGE_TIMER_EVENT)
  {
    autom_event = CST_POLLING_TIMER;
  }
  else if (message.type == (uint16_t)CST_MESSAGE_CS_EVENT)
  {
    autom_event = (CST_autom_event_t)message.id;
  }
  else if (message.type == (uint16_t)CST_MESSAGE_URC_EVENT)
  {
    autom_event = CST_MODEM_URC;
  }
  else if (message.type == (uint16_t)CST_MESSAGE_CMD)
  {
    switch (message.id)
    {
      case CST_INIT_EVENT:
      {
        autom_event = CST_INIT_EVENT;
        break;
      }
      case CST_MODEM_POWER_ON_EVENT:
      {
        autom_event = CST_MODEM_POWER_ON_EVENT;
        break;
      }
      default:
      {
        autom_event = CST_CMD_UNKWONW_EVENT;
        break;
      }
    }
  }
  else if (message.type == (uint16_t)CST_MESSAGE_DC_EVENT)
  {
    if (message.id == (uint16_t)DC_COM_CELLULAR_DATA_INFO)
    {
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                        sizeof(cst_cellular_data_info));
      if (DC_SERVICE_FAIL == cst_cellular_data_info.rt_state)
      {
        autom_event = CST_CELLULAR_DATA_FAIL_EVENT;
      }
    }
    if (message.id == (uint16_t)DC_CELLULAR_TARGET_STATE_CMD)
    {
      (void)dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&cst_target_state, sizeof(cst_target_state));
      if (DC_SERVICE_ON == cst_target_state.rt_state)
      {
        cst_cellular_params.target_state = cst_target_state.target_state;
        autom_event = CST_CELLULAR_STATE_EVENT;
      }
    }
  }
  else
  {
    PrintCellularService("CST_get_autom_event : No type matching\n\r")
  }

  return autom_event;
}

/* ===================================================================
   Automaton functions  Begin
   =================================================================== */

/* power on modem processing */
static void CST_power_on_only_modem_mngt(void)
{
  PrintCellularService("*********** CST_power_on_only_modem_mngt ********\n\r")
  (void)osCDS_power_on();
  PrintCellularService("*********** MODEM ON ********\n\r")
}

static void CST_power_on_modem_mngt(void)
{
  CS_Status_t cs_status;
  dc_cellular_info_t dc_cellular_info;

  PrintCellularService("*********** CST_power_on_modem_mngt ********\n\r")

  if (cst_cellular_params.target_state == DC_TARGET_STATE_OFF)
  {
    CST_current_state = CST_MODEM_OFF_STATE;
    /* Data Cache -> Radio ON */
    (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
    dc_cellular_info.modem_state = DC_MODEM_STATE_OFF;
    (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWERED_ON_EVENT);
  }
  else
  {
    CST_current_state = CST_MODEM_ON_STATE;
    cs_status = osCDS_power_on();

#if (CST_RESET_DEBUG == 1)
    if (cst_context.power_on_reset_count == 0)
    {
      cs_status = CELLULAR_ERROR;
    }
#endif /* (CST_RESET_DEBUG == 1) */

    if (cs_status != CELLULAR_OK)
    {
      CST_config_fail_mngt(((uint8_t *)"CST_cmd"),
                           CST_MODEM_POWER_ON_FAIL,
                           &cst_context.power_on_reset_count,
                           CST_POWER_ON_RESET_MAX);

    }
    else
    {
      /* Data Cache -> Radio ON */
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
      dc_cellular_info.rt_state = DC_SERVICE_RUN;
      dc_cellular_info.modem_state = DC_MODEM_STATE_POWERED_ON;
      (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWERED_ON_EVENT);
    }
  }
}

static CS_Status_t CST_reset_modem(void)
{
  return osCDS_reset(CS_RESET_AUTO);
}

/* power on modem processing */
static void CST_reset_modem_mngt(void)
{
  CS_Status_t cs_status;
  dc_cellular_info_t dc_cellular_info;

  PrintCellularService("*********** CST_power_on_modem_mngt ********\n\r")
  cs_status = CST_reset_modem();
#if (CST_RESET_DEBUG == 1)
  if (cst_context.reset_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif /* (CST_RESET_DEBUG == 1) */

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail_mngt(((uint8_t *)"CST_reset_modem_mngt"),
                         CST_MODEM_RESET_FAIL,
                         &cst_context.reset_reset_count,
                         CST_RESET_MAX);

  }
  else
  {
    /* Data Cache -> Radio ON */
    (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info_t));
    dc_cellular_info.rt_state = DC_SERVICE_ON;
    (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info_t));
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_POWERED_ON_EVENT);
  }
}

/* init modem management */
static void  CST_init_modem_mngt(void)
{
  CS_Status_t cs_status;
  dc_cellular_info_t dc_cellular_info;

  PrintCellularService("*********** CST_init_modem_mngt ********\n\r")
  (void)osCS_sim_select(cst_convert_sim_socket_type(cst_sim_info.active_slot));

  (void)osDelay(10);
  if (cst_cellular_params.set_pdn_mode != 0U)
  {
    /* we must first define Cellular context before activating the RF because
     * modem will immediately attach to network once RF is enabled
     */
    PrintCellularService("CST_modem_on_state : CST_modem_define_pdn\n\r")
    CST_modem_define_pdn();
  }

  if (cst_cellular_params.target_state == DC_TARGET_STATE_SIM_ONLY)
  {
    cs_status = osCDS_init_modem(CS_CMI_SIM_ONLY, CELLULAR_FALSE, GOOD_PINCODE);
  }
  else
  {
    cs_status = osCDS_init_modem(CS_CMI_FULL, CELLULAR_FALSE, GOOD_PINCODE);
  }
#if (CST_RESET_DEBUG == 1)
  if (cst_context.init_modem_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif /* (CST_RESET_DEBUG == 1) */
  if ((cs_status == CELLULAR_SIM_NOT_INSERTED) || (cs_status == CELLULAR_ERROR) || (cs_status == CELLULAR_SIM_ERROR))
  {
    (void)dc_com_read(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
    cst_sim_info.sim_status[cst_sim_slot_index] = DC_SIM_NOT_INSERTED;
    cst_sim_info.rt_state   = DC_SERVICE_ON;
    cst_sim_slot_index++;

    if ((uint16_t)cst_sim_slot_index  >= (uint16_t)cst_cellular_params.sim_slot_nb)
    {
      cst_sim_slot_index = 0 ;
      PrintCellularService("CST_modem_on_state : No SIM found\n\r")
    }

    cst_sim_info.active_slot = cst_cellular_params.sim_slot[cst_sim_slot_index].sim_slot_type;
    (void)dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

    CST_config_fail_mngt(((uint8_t *)"CST_init_modem_mngt"),
                         CST_MODEM_SIM_FAIL,
                         &cst_context.sim_reset_count,
                         CST_SIM_RETRY_MAX);

  }
  else
  {
    CST_subscribe_all_net_events();

    /* overwrite operator parameters */
    ctxt_operator.mode   = CS_NRM_AUTO;
    ctxt_operator.format = CS_ONF_NOT_PRESENT;
    CST_get_device_all_infos(cst_cellular_params.target_state);
    if (cst_cellular_params.target_state != DC_TARGET_STATE_SIM_ONLY)
    {
      CST_current_state = CST_MODEM_POWERED_ON_STATE;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_INITIALIZED_EVENT);
    }
    else
    {
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info_t));
      dc_cellular_info.modem_state = DC_MODEM_STATE_SIM_CONNECTED;
      (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info_t));
      CST_current_state = CST_MODEM_SIM_ONLY_STATE;
    }
  }
}


/* registration modem management */
static void  CST_net_register_mngt(void)
{
  CS_Status_t cs_status;
  CS_RegistrationStatus_t  cst_ctxt_reg_status;

  PrintCellularService("=== CST_net_register_mngt ===\n\r")
  cs_status = osCDS_register_net(&ctxt_operator, &cst_ctxt_reg_status);
  if (cs_status == CELLULAR_OK)
  {
    cst_context.current_EPS_NetworkRegState  = cst_ctxt_reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = cst_ctxt_reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = cst_ctxt_reg_status.CS_NetworkRegState;
    /*   to force to attach to PS domain by default (in case the Modem does not perform automatic PS attach.) */
    /*   need to check target state in future. */
    osCDS_attach_PS_domain();

    CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_EVENT);
  }
  else
  {
    PrintCellularService("===CST_net_register_mngt - FAIL !!! ===\n\r")
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 18, ERROR_WARNING);
  }
}

/* test if modem catch right signal */
static void  CST_signal_quality_test_mngt(void)
{
  PrintCellularService("*********** CST_signal_quality_test_mngt ********\n\r")

  if ((cst_context.signal_quality.rssi != 0U) && (cst_context.signal_quality.rssi != CST_BAD_SIG_RSSI))
  {
    (void)osTimerStart(CST_network_status_timer_handle, CST_NETWORK_STATUS_DELAY);
    PrintCellularService("-----> Start NW REG TIMEOUT TIMER   : %d\n\r", CST_NETWORK_STATUS_DELAY)

    CST_current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_EVENT);
  }

}

/* test if network status is OK */
static void  CST_network_status_test_mngt(void)
{
  PrintCellularService("*********** CST_network_status_test_mngt ********\n\r")

#if (CST_TEST_REGISTER_FAIL == 1)
  if (CST_register_fail_test_count >= CST_REGISTER_FAIL_TEST_COUNT_MAX)
#endif /* (CST_TEST_REGISTER_FAIL == 1) */
  {
    if ((cst_context.current_EPS_NetworkRegState  == CS_NRS_REGISTERED_HOME_NETWORK)
        || (cst_context.current_EPS_NetworkRegState  == CS_NRS_REGISTERED_ROAMING)
        || (cst_context.current_GPRS_NetworkRegState == CS_NRS_REGISTERED_HOME_NETWORK)
        || (cst_context.current_GPRS_NetworkRegState == CS_NRS_REGISTERED_ROAMING))
    {
      CST_current_state = CST_NETWORK_STATUS_OK_STATE;
      cst_context.register_retry_tempo_count = 0U;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_OK_EVENT);
    }
  }
}

static void  CST_network_event_mngt(void)
{
  CS_Status_t cs_status;
  CS_RegistrationStatus_t reg_status;

  PrintCellularService("*********** CST_network_event_mngt ********\n\r")
  (void)memset((void *)&reg_status, 0, sizeof(reg_status));
  cs_status = osCDS_get_net_status(&reg_status);
  if (cs_status == CELLULAR_OK)
  {
    cst_context.current_EPS_NetworkRegState  = reg_status.EPS_NetworkRegState;
    cst_context.current_GPRS_NetworkRegState = reg_status.GPRS_NetworkRegState;
    cst_context.current_CS_NetworkRegState   = reg_status.CS_NetworkRegState;

    if ((cst_context.current_EPS_NetworkRegState  != CS_NRS_REGISTERED_HOME_NETWORK)
        && (cst_context.current_EPS_NetworkRegState  != CS_NRS_REGISTERED_ROAMING)
        && (cst_context.current_GPRS_NetworkRegState != CS_NRS_REGISTERED_HOME_NETWORK)
        && (cst_context.current_GPRS_NetworkRegState != CS_NRS_REGISTERED_ROAMING))
    {
      CST_data_cache_set(DC_SERVICE_OFF);
      CST_current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
    }
    else /* device registered to network */
    {
      if (((uint16_t)reg_status.optional_fields_presence & (uint16_t)CS_RSF_FORMAT_PRESENT) != 0U)
      {
        (void)dc_com_read(&dc_com_db,  DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
        (void)memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME);
        cst_cellular_info.rt_state              = DC_SERVICE_ON;
        (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
        PrintCellularService(" ->operator_name = %s", reg_status.operator_name)
      }
      CST_current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
    }
  }
  else
  {
  }

}

/* attach modem management */
static void  CST_attach_modem_mngt(void)
{
  CS_Status_t              cs_status;
  CS_PSattach_t            cst_ctxt_attach_status;
  CS_RegistrationStatus_t  reg_status;

  PrintCellularService("*********** CST_attach_modem_mngt ********\n\r")

  (void)memset((void *)&reg_status, 0, sizeof(CS_RegistrationStatus_t));
  cs_status = osCDS_get_net_status(&reg_status);

  if (cs_status == CELLULAR_OK)
  {
    if (((uint16_t)reg_status.optional_fields_presence & (uint16_t)CS_RSF_FORMAT_PRESENT) != 0U)
    {
      (void)dc_com_read(&dc_com_db,  DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
      (void)memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME);
      cst_cellular_info.rt_state              = DC_SERVICE_ON;
      (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

      PrintCellularService(" ->operator_name = %s\n\r", reg_status.operator_name)
    }
  }

  cs_status = osCDS_get_attach_status(&cst_ctxt_attach_status);
#if (CST_RESET_DEBUG == 1)
  if (cst_context.attach_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif  /* (CST_RESET_DEBUG == 1) */
  if (cs_status != CELLULAR_OK)
  {
    PrintCellularService("*********** CST_attach_modem_mngt fail ********\n\r")
    CST_config_fail_mngt(((uint8_t *)"CS_get_attach_status FAIL"),
                         CST_MODEM_ATTACH_FAIL,
                         &cst_context.attach_reset_count,
                         CST_ATTACH_RESET_MAX);
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_FAIL_EVENT);
  }
  else
  {
    if (cst_ctxt_attach_status == CS_PS_ATTACHED)
    {
      PrintCellularService("*********** CST_attach_modem_mngt OK ********\n\r")
      CST_current_state = CST_MODEM_REGISTERED_STATE;
      CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
    }

    else
    {
      PrintCellularService("===CST_attach_modem_mngt - NOT ATTACHED !!! ===\n\r")
      /* Workaround waiting for Modem behaviour clarification */
      CST_config_fail_mngt(((uint8_t *)"CST_pdn_event_mngt"),
                           CST_MODEM_ATTACH_FAIL,
                           &cst_context.power_on_reset_count,
                           CST_POWER_ON_RESET_MAX);
#if 0
      cs_status = osCDS_attach_PS_domain();
      if (cs_status != CELLULAR_OK)
      {
        PrintCellularService("===CST_attach_modem_mngt - osCDS_attach_PS_domain FAIL !!! ===\n\r")
        CST_config_fail_mngt("CS_get_attach_status FAIL",
                             CST_MODEM_ATTACH_FAIL,
                             &cst_context.attach_reset_count,
                             CST_ATTACH_RESET_MAX);
        CST_send_message(CST_MESSAGE_CS_EVENT, CST_FAIL_EVENT);
      }
      else
      {
        osDelay(100);
        CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_OK_EVENT);
      }
#endif /* work around */
    }
  }
}

/* pdn definition modem management */
static void CST_modem_define_pdn(void)
{
  CS_PDN_configuration_t pdn_conf;

  CS_Status_t cs_status;
  /*
  #if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      cs_status = CELLULAR_OK;
  #else
  */
  /* define user PDN configurations */

  /* common user name and password */
  (void)memset((void *)&pdn_conf, 0, sizeof(CS_PDN_configuration_t));
  (void)memcpy((CRC_CHAR_t *)pdn_conf.username,
               (CRC_CHAR_t *)cst_cellular_params.sim_slot[cst_sim_slot_index].username,
                crs_strlen(cst_cellular_params.sim_slot[cst_sim_slot_index].username)+1);

  (void)memcpy((CRC_CHAR_t *)pdn_conf.password,
               (CRC_CHAR_t *)cst_cellular_params.sim_slot[cst_sim_slot_index].password,
                crs_strlen(cst_cellular_params.sim_slot[cst_sim_slot_index].password)+1);

  /* exemple for CS_PDN_USER_CONFIG_1 with access point name =  "PDN CONFIG 1" */
  cs_status = osCDS_define_pdn(cst_cellular_params.sim_slot[cst_sim_slot_index].cid, (const uint8_t *)cst_cellular_params.sim_slot[cst_sim_slot_index].apn, &pdn_conf);

  /* #endif */

#if (CST_RESET_DEBUG == 1)
  if (cst_context.activate_pdn_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif /* (CST_RESET_DEBUG == 1) */

  if (cs_status != CELLULAR_OK)
  {
    CST_config_fail_mngt(((uint8_t *)"CST_modem_define_pdn"),
                         CST_MODEM_PDP_DEFINE_FAIL,
                         &cst_context.activate_pdn_reset_count,
                         CST_DEFINE_PDN_RESET_MAX);
  }
  /*
      else
      {
          CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDP_ACTIVATED_EVENT);
      }
  */
}

/* pdn activation modem management */
static CS_Status_t CST_modem_activate_pdn(void)
{
  CS_Status_t cs_status;
  (void)osCDS_set_default_pdn(cst_cellular_params.sim_slot[cst_sim_slot_index].cid);

  /* register to PDN events for this CID*/
  (void)osCDS_register_pdn_event(cst_cellular_params.sim_slot[cst_sim_slot_index].cid, CST_pdn_event_callback);

  cs_status = osCDS_activate_pdn(CS_PDN_CONFIG_DEFAULT);
#if (CST_TEST_NFMC == 1)
  CST_nfmc_test_count++;
  if (CST_nfmc_test_count < CST_NFMC_TEST_COUNT_MAX)
  {
    cs_status = (CS_Status_t)1;
  }
#endif  /*  (CST_TEST_NFMC == 1)  */

#if (CST_RESET_DEBUG == 1)
  if (cst_context.activate_pdn_reset_count == 0)
  {
    cs_status = CELLULAR_ERROR;
  }
#endif  /*  (CST_RESET_DEBUG == 1)  */

  if (cs_status != CELLULAR_OK)
  {
    if (CST_nfmc_context.active == 0U)
    {
      (void)osTimerStart(CST_pdn_activate_retry_timer_handle, CST_PDN_ACTIVATE_RETRY_DELAY);
      PrintCellularService("-----> CST_modem_activate_pdn NOK - retry tempo  : %d\n\r", CST_PDN_ACTIVATE_RETRY_DELAY)
    }
    else
    {
      (void)osTimerStart(CST_pdn_activate_retry_timer_handle,
                         CST_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count]);
      PrintCellularService("-----> CST_modem_activate_pdn NOK - retry tempo %d : %ld\n\r",
                           cst_context.activate_pdn_nfmc_tempo_count + 1U,
                           CST_nfmc_context.tempo[cst_context.activate_pdn_nfmc_tempo_count])
    }

    cst_context.activate_pdn_nfmc_tempo_count++;
    if (cst_context.activate_pdn_nfmc_tempo_count >= CST_NFMC_TEMPO_NB)
    {
      cst_context.activate_pdn_nfmc_tempo_count = 0U;
    }
  }
  else
  {
    cst_context.activate_pdn_nfmc_tempo_count = 0U;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_PDP_ACTIVATED_EVENT);
  }
  return cs_status;
}

/*  pppos config fail */
static void CST_cellular_data_fail_mngt(void)
{
  CST_config_fail_mngt(((uint8_t *)"CST_cellular_data_fail_mngt"),
                       CST_MODEM_CELLULAR_DATA_FAIL,
                       &cst_context.cellular_data_retry_count,
                       CST_CELLULAR_DATA_RETRY_MAX);
}

static void CST_pdn_event_mngt(void)
{
  if (cst_context.pdn_status == CS_PDN_EVENT_NW_DETACH)
  {
    /* Workaround waiting for Modem behaviour clarification */
    CST_network_event_mngt();
  }
  else if (
    (cst_context.pdn_status == CS_PDN_EVENT_NW_DEACT)
    || (cst_context.pdn_status == CS_PDN_EVENT_NW_PDN_DEACT))
  {
    PrintCellularService("=========CST_pdn_event_mngt CS_PDN_EVENT_NW_DEACT\n\r")
    CST_current_state = CST_MODEM_REGISTERED_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_ATTACHED_EVENT);
  }
  else
  {
    CST_current_state = CST_WAITING_FOR_NETWORK_STATUS_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_CALLBACK_EVENT);
  }
}

static void CST_cellular_state_event_mngt(void)
{
  (void)CS_power_off();
  CST_current_state = CST_MODEM_INIT_STATE;

  (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
  cst_cellular_info.rt_state              = DC_SERVICE_UNAVAIL;
  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  (void)dc_com_read(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  cst_cellular_info.rt_state              = DC_SERVICE_UNAVAIL;
  (void)dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

  (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                    sizeof(cst_cellular_data_info));
  cst_cellular_data_info.rt_state = DC_SERVICE_SHUTTING_DOWN;
  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_DATA_INFO, (void *)&cst_cellular_data_info,
                     sizeof(cst_cellular_data_info));

  CST_send_message(CST_MESSAGE_CMD, CST_INIT_EVENT);
}



/* =================================================================
   Management automaton functions according current automation state
   ================================================================= */

static void CST_init_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_INIT_EVENT:
    {
      CST_power_on_modem_mngt();
      break;
    }
    case CST_MODEM_POWER_ON_EVENT:
    {
      CST_current_state = CST_MODEM_ON_ONLY_STATE;
      CST_power_on_only_modem_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 23, ERROR_WARNING);
      break;
    }
  }
  /* subscribe modem events after power ON */
  CST_subscribe_modem_events();
}

static void CST_reset_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_INIT_EVENT:
    {
      CST_current_state = CST_MODEM_ON_STATE;
      CST_reset_modem_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 24, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_on_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_POWERED_ON_EVENT:
    {
      CST_init_modem_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 25, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_off_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_power_on_modem_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 25, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_sim_only_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_power_on_modem_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 25, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_powered_on_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_INITIALIZED_EVENT:
    {
      CST_current_state = CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE;
      CST_net_register_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 27, ERROR_WARNING);
      break;
    }
  }
}

static void CST_waiting_for_signal_quality_ok_state(CST_autom_event_t autom_event)
{
  PrintCellularService("\n\r ====> CST_waiting_for_signal_quality_ok_state <===== \n\r")

  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    case CST_SIGNAL_QUALITY_EVENT:
    {
      CST_signal_quality_test_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 28, ERROR_WARNING);
      break;
    }
  }
}

static void CST_waiting_for_network_status_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
    case CST_NETWORK_STATUS_EVENT:
    {
      CST_network_status_test_mngt();
      break;
    }
    case CST_NW_REG_TIMEOUT_TIMER_EVENT:
    {
      PrintCellularService("-----> NW REG TIMEOUT TIMER EXPIRY WE PWDN THE MODEM \n\r")
      CST_current_state = CST_MODEM_NETWORK_STATUS_FAIL_STATE;
      (void)CS_power_off();

      (void)osTimerStart(CST_register_retry_timer_handle, CST_nfmc_context.tempo[cst_context.register_retry_tempo_count]);
      PrintCellularService("-----> CST_waiting_for_network_status NOK - retry tempo %d : %ld\n\r",
                           cst_context.register_retry_tempo_count + 1U,
                           CST_nfmc_context.tempo[cst_context.register_retry_tempo_count])
#if (CST_TEST_REGISTER_FAIL == 1)
      CST_register_fail_test_count++;
#endif  /* (CST_TEST_REGISTER_FAIL == 1) */
      cst_context.register_retry_tempo_count++;
      if (cst_context.register_retry_tempo_count >= CST_NFMC_TEMPO_NB)
      {
        cst_context.register_retry_tempo_count = 0U;
      }
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 29, ERROR_WARNING);
      break;
    }
  }
}

static void CST_network_status_ok_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_STATUS_OK_EVENT:
    {
      CST_attach_modem_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 30, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_registered_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_MODEM_ATTACHED_EVENT:
    {
      CST_current_state = CST_MODEM_PDN_ACTIVATE_STATE;
      (void)CST_modem_activate_pdn();
      break;
    }
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 31, ERROR_WARNING);
      break;
    }
  }
}

static void CST_modem_pdn_activate_state(CST_autom_event_t autom_event)
{
  dc_cellular_info_t dc_cellular_info;
  switch (autom_event)
  {
    case CST_PDP_ACTIVATED_EVENT:
    {
      CST_reset_fail_count();
      CST_current_state       = CST_MODEM_DATA_READY_STATE;
      CST_data_cache_set(DC_SERVICE_ON);
      /* Data Cache -> Radio ON */
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));
      dc_cellular_info.modem_state = DC_MODEM_STATE_DATA_OK;
      (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&dc_cellular_info, sizeof(dc_cellular_info));

      break;
    }
    case CST_MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT:
    {
      (void)CST_modem_activate_pdn();
      break;
    }
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    case CST_PDN_STATUS_EVENT:
    {
      CST_pdn_event_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 32, ERROR_WARNING);
      break;
    }
  }
}

static void CST_data_ready_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_NETWORK_CALLBACK_EVENT:
    {
      CST_network_event_mngt();
      break;
    }
    case CST_CELLULAR_DATA_FAIL_EVENT:
    {
      CST_cellular_data_fail_mngt();
      break;
    }
    case CST_PDN_STATUS_EVENT:
    {
      CST_pdn_event_mngt();
      break;
    }
    case CST_CELLULAR_STATE_EVENT:
    {
      CST_cellular_state_event_mngt();
      break;
    }

    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 33, ERROR_WARNING);
      break;
    }
  }
}

static void CST_fail_state(CST_autom_event_t autom_event)
{
  switch (autom_event)
  {
    case CST_FAIL_EVENT:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, __LINE__, ERROR_WARNING);
      break;
    }
    default:
    {
      ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 26, ERROR_WARNING);
      break;
    }
  }
}

/* ===================================================================
   Automaton functions  END
   =================================================================== */


/* Timer handler
     During configuration :  Signal quality  and network status Polling
     After configuration  :  Modem monitoring (Signal quality checking)
 */
static void CST_timer_handler(void)
{
  CS_Status_t cs_status ;
  CS_RegistrationStatus_t reg_status;

  /*    PrintCellularService("-----> CST_timer_handler  <-----\n\r") */
  CST_polling_timer_flag = 0U;
  if (CST_current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
  {
    (void)memset((void *)&reg_status, 0, sizeof(CS_RegistrationStatus_t));
    cs_status = osCDS_get_net_status(&reg_status);
    if (cs_status == CELLULAR_OK)
    {
      cst_context.current_EPS_NetworkRegState  = reg_status.EPS_NetworkRegState;
      cst_context.current_GPRS_NetworkRegState = reg_status.GPRS_NetworkRegState;
      cst_context.current_CS_NetworkRegState   = reg_status.CS_NetworkRegState;
      PrintCellularService("-----> CST_timer_handler - CST_NETWORK_STATUS_EVENT <-----\n\r")

      CST_send_message(CST_MESSAGE_CS_EVENT, CST_NETWORK_STATUS_EVENT);

      if (((uint16_t)reg_status.optional_fields_presence & (uint16_t)CS_RSF_FORMAT_PRESENT) != 0U)
      {
        (void)dc_com_read(&dc_com_db,  DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));
        (void)memcpy(cst_cellular_info.mno_name, reg_status.operator_name, DC_MAX_SIZE_MNO_NAME);
        cst_cellular_info.rt_state              = DC_SERVICE_ON;
        (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

        PrintCellularService(" ->operator_name = %s\n\r", reg_status.operator_name)
      }
    }
    else
    {
      CST_config_fail_mngt(((uint8_t *)"osCDS_get_net_status"),
                           CST_MODEM_GNS_FAIL,
                           &cst_context.gns_reset_count,
                           CST_GNS_MODEM_RESET_MAX);
    }
  }

  if (CST_current_state == CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE)
  {
    (void)CST_set_signal_quality();
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_SIGNAL_QUALITY_EVENT);
  }

#if (CST_FOTA_TEST == 1)
  if (CST_current_state == CST_MODEM_DATA_READY_STATE)
  {
    CST_modem_event_callback(CS_MDMEVENT_FOTA_START);
  }
#endif  /* (CST_FOTA_TEST == 1) */

#if (CST_MODEM_POLLING_PERIOD != 0)
  if (CST_polling_active == 1U)
  {
    if (CST_current_state == CST_MODEM_DATA_READY_STATE)
    {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      (void)osCDS_suspend_data();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
      (void)CST_set_signal_quality();
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      /* CS_check_connection(); */
      (void)osCDS_resume_data();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
    }
  }
#endif  /* CST_MODEM_POLLING_PERIOD != 0) */
}

/* Cellular Service Task : autmaton management */
static void CST_cellular_service_task(void const *argument)
{
  osEvent event;
  CST_autom_event_t autom_event;

  for (;;)
  {
    event = osMessageGet(cst_queue_id, RTOS_WAIT_FOREVER);
    autom_event = CST_get_autom_event(event);
    /*    PrintCellularService("======== CST_cellular_service_task autom_event %d\n\r", autom_event) */

    if (autom_event == CST_POLLING_TIMER)
    {
      CST_timer_handler();
    }
    else if (autom_event != CST_NO_EVENT)
    {
      switch (CST_current_state)
      {
        case CST_MODEM_INIT_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_INIT_STATE <-----\n\r")
          CST_init_state(autom_event);
          break;
        }
        case CST_MODEM_RESET_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_RESET_STATE <-----\n\r")
          CST_reset_state(autom_event);
          break;
        }
        case CST_MODEM_ON_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_ON_STATE <-----\n\r")
          CST_modem_on_state(autom_event);
          break;
        }
        case CST_MODEM_OFF_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_OFF_STATE <-----\n\r")
          CST_modem_off_state(autom_event);
          break;
        }
        case CST_MODEM_SIM_ONLY_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_SIM_ONLY_STATE <-----\n\r")
          CST_modem_sim_only_state(autom_event);
          break;
        }
        case CST_MODEM_POWERED_ON_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_POWERED_ON_STATE <-----\n\r")
          CST_modem_powered_on_state(autom_event);
          break;
        }
        case CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE:
        {
          PrintCellularService("-----> State : CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE <-----\n\r")
          CST_waiting_for_signal_quality_ok_state(autom_event);
          break;
        }
        case CST_WAITING_FOR_NETWORK_STATUS_STATE:
        {
          PrintCellularService("-----> State : CST_WAITING_FOR_NETWORK_STATUS_STATE <-----\n\r")
          CST_waiting_for_network_status_state(autom_event);
          break;
        }

        case CST_NETWORK_STATUS_OK_STATE:
        {
          PrintCellularService("-----> State : CST_NETWORK_STATUS_OK_STATE <-----\n\r")
          CST_network_status_ok_state(autom_event);
          break;
        }

        case CST_MODEM_REGISTERED_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_REGISTERED_STATE <-----\n\r")
          CST_modem_registered_state(autom_event);
          break;
        }
        case CST_MODEM_PDN_ACTIVATE_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_PDN_ACTIVATE_STATE <-----\n\r")
          CST_modem_pdn_activate_state(autom_event);
          break;
        }

        case CST_MODEM_DATA_READY_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_DATA_READY_STATE <-----\n\r")
          CST_data_ready_state(autom_event);
          break;
        }
        case CST_MODEM_FAIL_STATE:
        {
          PrintCellularService("-----> State : CST_MODEM_FAIL_STATE <-----\n\r")
          CST_fail_state(autom_event);
          break;
        }
        default:
        {
          PrintCellularService("-----> State : Not State <-----\n\r")
          break;
        }
      }
    }
    else
    {
      PrintCellularService("============ CST_cellular_service_task : autom_event = no event \n\r")
    }
  }
}

/* CST_polling_timer_callback function */
static void CST_polling_timer_callback(void const *argument)
{
  UNUSED(argument);
  if (CST_current_state != CST_MODEM_INIT_STATE)
  {
    CST_message_t cmd_message;
    uint32_t *cmd_message_p = (uint32_t *)(&cmd_message);

    cmd_message.type = (uint16_t)CST_MESSAGE_TIMER_EVENT;
    cmd_message.id   = CST_NO_EVENT;

    if (CST_polling_timer_flag == 0U)
    {
      CST_polling_timer_flag = 1U;
      (void)osMessagePut(cst_queue_id, *cmd_message_p, 0U);
    }
  }
}

/* CST_pdn_activate_retry_timer_callback */
static void CST_pdn_activate_retry_timer_callback(void const *argument)
{
  UNUSED(argument);
  if (CST_current_state == CST_MODEM_PDN_ACTIVATE_STATE)
  {
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_MODEM_PDN_ACTIVATE_RETRY_TIMER_EVENT);
  }
}

/* CST_pdn_activate_retry_timer_callback */
static void CST_register_retry_timer_callback(void const *argument)
{
  UNUSED(argument);
  if (CST_current_state == CST_MODEM_NETWORK_STATUS_FAIL_STATE)
  {
    CST_current_state = CST_MODEM_INIT_STATE;
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_INIT_EVENT);
  }
}

static void CST_network_status_timer_callback(void const *argument)
{
  UNUSED(argument);
  if (CST_current_state == CST_WAITING_FOR_NETWORK_STATUS_STATE)
  {
    CST_send_message(CST_MESSAGE_CS_EVENT, CST_NW_REG_TIMEOUT_TIMER_EVENT);
  }
}


/* CST_fota_timer_callback */
/* FOTA Timeout expired : restart board */
static void CST_fota_timer_callback(void const *argument)
{
  UNUSED(argument);
  PrintCellularService("CST FOTA FAIL : Timeout expired RESTART\n\r")

  ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 20, ERROR_FATAL);
}


/* Functions Definition ------------------------------------------------------*/
CS_Status_t  CST_radio_on(void)
{
  CST_send_message(CST_MESSAGE_CMD, CST_INIT_EVENT);
  return CELLULAR_OK;
}

CS_Status_t  CST_modem_power_on(void)
{
  CST_send_message(CST_MESSAGE_CMD, CST_MODEM_POWER_ON_EVENT);
  return CELLULAR_OK;
}

CS_Status_t CST_get_dev_IP_address(CS_IPaddrType_t *ip_addr_type, CS_CHAR_t *p_ip_addr_value)
{
  return CS_get_dev_IP_address(cst_cellular_params.sim_slot[cst_sim_slot_index].cid, ip_addr_type, p_ip_addr_value);
}


CS_Status_t CST_cellular_service_init(void)
{
  CST_current_state = CST_MODEM_INIT_STATE;
  CST_modem_init();

  (void)osCDS_cellular_service_init();
  CST_csq_count_fail = 0U;
  CST_polling_active = 1;

  //(void)CST_config_init();

  osMessageQDef(cst_queue_id, 10, sizeof(CST_message_t));
  cst_queue_id = osMessageCreate(osMessageQ(cst_queue_id), NULL);
  if (cst_queue_id == NULL)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 18, ERROR_FATAL);
  }
  return CELLULAR_OK;
}

CST_state_t CST_get_state(void)
{
  return CST_current_state;
}

CS_Status_t CST_cellular_service_start(void)
{
  static osThreadId CST_cellularServiceThreadId = NULL;
  dc_nfmc_info_t nfmc_info;
  uint32_t cst_polling_period;

  osTimerId         cst_polling_timer_handle;

#if (USE_CMD_CONSOLE == 1)
  (void)CST_cmd_cellular_service_start();
#endif  /*  (USE_CMD_CONSOLE == 1) */

  (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cst_cellular_params, sizeof(cst_cellular_params));

  (void)dc_com_read(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));
  cst_sim_info.sim_status[DC_SIM_SLOT_MODEM_SOCKET]       = DC_SIM_NOT_USED;
  cst_sim_info.sim_status[DC_SIM_SLOT_MODEM_EMBEDDED_SIM] = DC_SIM_NOT_USED;
  cst_sim_info.sim_status[DC_SIM_SLOT_STM32_EMBEDDED_SIM] = DC_SIM_NOT_USED;
  cst_sim_slot_index = 0;
  cst_sim_info.active_slot = cst_cellular_params.sim_slot[cst_sim_slot_index].sim_slot_type;
  (void)dc_com_write(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_sim_info, sizeof(cst_sim_info));

  CST_modem_start();

  (void)dc_com_register_gen_event_cb(&dc_com_db, CST_notif_cb, (const void *)NULL);
  cst_cellular_info.mno_name[0]           = 0U;
  cst_cellular_info.rt_state              = DC_SERVICE_UNAVAIL;


  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR, (void *)&cst_cellular_info, sizeof(cst_cellular_info));

  nfmc_info.rt_state = DC_SERVICE_UNAVAIL;
  (void)dc_com_write(&dc_com_db, DC_COM_NFMC_TEMPO, (void *)&nfmc_info, sizeof(nfmc_info));

  osTimerDef(cs_polling_timer, CST_polling_timer_callback);
  cst_polling_timer_handle = osTimerCreate(osTimer(cs_polling_timer), osTimerPeriodic, NULL);
#if (CST_MODEM_POLLING_PERIOD == 0)
  cst_polling_period = CST_MODEM_POLLING_PERIOD_DEFAULT;
#else
  cst_polling_period = CST_MODEM_POLLING_PERIOD;
#endif  /*  (CST_MODEM_POLLING_PERIOD == 1) */
  (void)osTimerStart(cst_polling_timer_handle, cst_polling_period);

  osTimerDef(CST_pdn_activate_retry_timer, CST_pdn_activate_retry_timer_callback);
  CST_pdn_activate_retry_timer_handle = osTimerCreate(osTimer(CST_pdn_activate_retry_timer), osTimerOnce, NULL);

  osTimerDef(CST_waiting_for_network_status_timer, CST_network_status_timer_callback);
  CST_network_status_timer_handle = osTimerCreate(osTimer(CST_waiting_for_network_status_timer), osTimerOnce, NULL);

  osTimerDef(CST_register_retry_timer, CST_register_retry_timer_callback);
  CST_register_retry_timer_handle = osTimerCreate(osTimer(CST_register_retry_timer), osTimerOnce, NULL);

  osTimerDef(CST_fota_timer, CST_fota_timer_callback);
  CST_fota_timer_handle = osTimerCreate(osTimer(CST_fota_timer), osTimerOnce, NULL);

  osThreadDef(cellularServiceTask, CST_cellular_service_task, CELLULAR_SERVICE_THREAD_PRIO, 0,
              CELLULAR_SERVICE_THREAD_STACK_SIZE);
  CST_cellularServiceThreadId = osThreadCreate(osThread(cellularServiceTask), NULL);

  if (CST_cellularServiceThreadId == NULL)
  {
    ERROR_Handler(DBG_CHAN_CELLULAR_SERVICE, 19, ERROR_FATAL);
  }
  else
  {
#if (STACK_ANALYSIS_TRACE == 1)
    stackAnalysis_addStackSizeByHandle(CST_cellularServiceThreadId, CELLULAR_SERVICE_THREAD_STACK_SIZE);
#endif /* (STACK_ANALYSIS_TRACE==1) */
  }

  return CELLULAR_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

