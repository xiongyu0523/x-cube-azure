/**
  ******************************************************************************
  * @file    cellular_service_cmd.c
  * @author  MCD Application Team
  * @brief   This file defines cellular service console command
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
#include "cellular_service_os.h"
#include "cellular_service_task.h"
#include "error_handler.h"
#include "plf_config.h"
#include "cellular_runtime_custom.h"

#if (USE_CMD_CONSOLE == 1)

#if defined(USE_MODEM_BG96)
#define CST_CMD_USE_MODEM_CONFIG     (1)
#define CST_CMD_MODEM_BG96           (1)
#define CST_CMD_MODEM_TYPE1SC        (0)
#define CST_CMD_USE_MODEM_CELL_GM01Q (0)
#elif defined(USE_MODEM_TYPE1SC)
#define CST_CMD_USE_MODEM_CONFIG     (1)
#define CST_CMD_MODEM_BG96           (0)
#define CST_CMD_MODEM_TYPE1SC        (1)
#define CST_CMD_USE_MODEM_CELL_GM01Q (0)
#elif defined(HWREF_B_CELL_GM01Q)
#define CST_CMD_USE_MODEM_CONFIG     (1)
#define CST_CMD_MODEM_BG96           (0)
#define CST_CMD_MODEM_TYPE1SC        (0)
#define CST_CMD_USE_MODEM_CELL_GM01Q (1)
#else
#define CST_CMD_USE_MODEM_CONFIG     (0)
#define CST_CMD_MODEM_BG96           (0)
#define CST_CMD_MODEM_TYPE1SC        (0)
#define CST_CMD_USE_MODEM_CELL_GM01Q (0)
#endif  /* USE_MODEM_BG96 */

#if (CST_CMD_MODEM_BG96 == 1)

#include "at_custom_modem_specific_bg96.h"
#endif  /* CST_CMD_MODEM_BG96 */

#include "cmd.h"

/* Private defines -----------------------------------------------------------*/
#define CST_ATCMD_SIZE_MAX      100U
#define CST_CMS_PARAM_MAX        13U
#define CST_AT_TIMEOUT         5000U


#if (CST_CMD_MODEM_BG96 == 1)
#define CST_CMD_MAX_BAND        16U
#define CST_CMD_GSM_BAND_NUMBER  6U
#define CST_CMD_M1_BAND_NUMBER  16U
#define CST_CMD_NB1_BAND_NUMBER 15U
#define CST_CMD_SCANSEQ_NUMBER  16U

#define  CST_scanseq_NB1_M1  ((ATCustom_BG96_QCFGscanseq_t) 0x030202)
#define  CST_scanseq_NB1_GSM ((ATCustom_BG96_QCFGscanseq_t) 0x030101)
#define  CST_scanseq_M1_GSM  ((ATCustom_BG96_QCFGscanseq_t) 0x020101)
#define  CST_scanseq_M1_NB1  ((ATCustom_BG96_QCFGscanseq_t) 0x020303)
#define  CST_scanseq_GSM_M1  ((ATCustom_BG96_QCFGscanseq_t) 0x010202)
#define  CST_scanseq_GSM_NB1 ((ATCustom_BG96_QCFGscanseq_t) 0x010303)

#define  CST_scanseq_GSM     ((ATCustom_BG96_QCFGscanseq_t) 0x010101)
#define  CST_scanseq_M1      ((ATCustom_BG96_QCFGscanseq_t) 0x020202)
#define  CST_scanseq_NB1     ((ATCustom_BG96_QCFGscanseq_t) 0x030303)

#endif  /* (CST_CMD_MODEM_BG96 == 1)*/

#if (CST_CMD_MODEM_TYPE1SC == 1)
#define CST_BAND_MASK_13_LSB 1
#define CST_BAND_MASK_13_MSB 0

#define CST_BAND_MASK_20_LSB 2
#define CST_BAND_MASK_20_MSB 0
#endif /* defined(CST_CMD_MODEM_TYPE1SC == 1) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
#define CST_CMD_BAND_MAX 12
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */


/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PrintForce(format, args...)   printf("" format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t *name;
  uint32_t value_MSB; /* 32bits MSB part of the band: XXXXXXXX........ */
  uint32_t value_LSB; /* 32bits LSB part of the band: ........XXXXXXXX */
} CST_band_descr_t;

typedef struct
{
  uint8_t *name;
  uint32_t value;
} CST_seq_descr_t;

/* Private variables ---------------------------------------------------------*/
uint8_t *CST_SimSlotName_p[3] =
{
  ((uint8_t *)"MODEM SOCKET"),
  ((uint8_t *)"MODEM EMBEDDED SIM"),
  ((uint8_t *)"STM32 EMBEDDED SIM")
};
/* Global variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t *CST_cmd_label = ((uint8_t *)"cst");
static uint8_t *CST_cmd_at_label = ((uint8_t *)"atcmd");



#if (CST_CMD_USE_MODEM_CONFIG==1)
static uint8_t *CST_cmd_modem_label = ((uint8_t *)"modem");
#endif /* CST_CMD_USE_MODEM_CONFIG == 1 */

static uint32_t cst_at_timeout = CST_AT_TIMEOUT;

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
static uint8_t CST_CMD_band_tab[CST_CMD_BAND_MAX] = {3,12,0,0,0,0,0,0,0,0,0,0};
static uint8_t CST_CMD_band_count = 2;
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */

/* Private function prototypes -----------------------------------------------*/
static cmd_status_t CST_cmd(uint8_t *cmd_line_p);
static cmd_status_t cst_at_command_handle(uint8_t *cmd_line_p);
static void CST_HelpCmd(void);
static void cst_at_cmd_help(void);

#if (CST_CMD_USE_MODEM_CONFIG==1)
static void CST_ModemHelpCmd(void);
#if (( CST_CMD_MODEM_TYPE1SC == 1) || ( CST_CMD_MODEM_BG96 == 1 ))
static uint32_t CST_CMD_get_band(const CST_band_descr_t *band_descr,
                                 const uint8_t   *const *argv_p, uint32_t argc,
                                 uint32_t *band_result_MSB, uint32_t *band_result_LSB);
static void  CST_CMD_display_bitmap_name(uint32_t bitmap_MSB, uint32_t bitmap_LSB,
                                         const CST_band_descr_t *bitmap_descr);
#endif /* (( CST_CMD_MODEM_TYPE1SC == 1) || ( CST_CMD_MODEM_BG96 == 1 )) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
static void  CST_CMD_display_bitmap_name_sequans(void);
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */
#endif /* CST_CMD_USE_MODEM_CONFIG == 1 */


/* Private function Definition -----------------------------------------------*/
static void CST_HelpCmd(void)
{
  CMD_print_help(CST_cmd_label);
  PrintForce("%s help", (CRC_CHAR_t *)CST_cmd_label)
  PrintForce("%s state   (Displays the cellular and SIM state)", CST_cmd_label)
  PrintForce("%s config  (Displays the cellular configuration used)", CST_cmd_label)
  PrintForce("%s info    (Displays modem information)", CST_cmd_label)
  PrintForce("%s targetstate [off|full] (set modem state)", CST_cmd_label)
  PrintForce("%s polling [on|off]  (enable/disable periodical modem polling)", CST_cmd_label)
}

static cmd_status_t CST_cmd(uint8_t *cmd_line_p)
{
  static uint8_t *CST_SimModeName_p[] =
  {
    ((uint8_t *)"OK"),
    ((uint8_t *)"NOT IMPLEMENTED"),
    ((uint8_t *)"SIM BUSY"),
    ((uint8_t *)"SIM NOT INSERTED"),
    ((uint8_t *)"SIM PIN OR PUK LOCKED"),
    ((uint8_t *)"SIM INCORRECT PASSWORD"),
    ((uint8_t *)"SIM ERROR"),
    ((uint8_t *)"SIM NOT USED"),
    ((uint8_t *)"SIM CONNECTION ON GOING")
  };

  static uint8_t *CST_ActivateName_p[] =
  {
    (uint8_t *)"Not active",
    (uint8_t *)"Active"
  };

  static uint8_t *CST_StateName_p[] =
  {
    ((uint8_t *)"CST_MODEM_INIT_STATE"),
    ((uint8_t *)"CST_MODEM_RESET_STATE"),
    ((uint8_t *)"CST_MODEM_OFF_STATE"),
    ((uint8_t *)"CST_MODEM_ON_STATE"),
    ((uint8_t *)"CST_MODEM_ON_ONLY_STATE"),
    ((uint8_t *)"CST_MODEM_POWERED_ON_STATE"),
    ((uint8_t *)"CST_WAITING_FOR_SIGNAL_QUALITY_OK_STATE"),
    ((uint8_t *)"CST_WAITING_FOR_NETWORK_STATUS_STATE"),
    ((uint8_t *)"CST_NETWORK_STATUS_OK_STATE"),
    ((uint8_t *)"CST_MODEM_REGISTERED_STATE"),
    ((uint8_t *)"CST_MODEM_PDN_ACTIVATE_STATE"),
    ((uint8_t *)"CST_MODEM_PDN_ACTIVATED_STATE"),
    ((uint8_t *)"CST_MODEM_DATA_READY_STATE"),
    ((uint8_t *)"CST_MODEM_REPROG_STATE"),
    ((uint8_t *)"CST_MODEM_FAIL_STATE"),
    ((uint8_t *)"CST_MODEM_NETWORK_STATUS_FAIL_STATE"),
    ((uint8_t *)"CST_MODEM_SIM_ONLY_STATE")
  };
  static uint8_t *CST_TargetStateName_p[] =
  {
    ((uint8_t *)"OFF"),
    ((uint8_t *)"SIM_ONLY"),
    ((uint8_t *)"FULL"),
  };

  static uint8_t *CST_ModemStateName_p[] =
  {
    ((uint8_t *)"OFF"),
    ((uint8_t *)"POWERED_ON"),
    ((uint8_t *)"SIM_CONNECTED"),
    ((uint8_t *)"DATA_ON"),
  };

  static dc_cellular_info_t      cst_cmd_cellular_info;
  static dc_sim_info_t           cst_cmd_sim_info;
  static dc_cellular_params_t    cst_cmd_cellular_params;
  static dc_nfmc_info_t          cst_cmd_nfmc_info;
  static dc_cellular_target_state_t target_state;
  uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t   argc;
  uint8_t    *cmd_p;
  uint32_t i;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PrintForce("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
  if (memcmp((CRC_CHAR_t *)cmd_p,
              (CRC_CHAR_t *)CST_cmd_label,
              crs_strlen(cmd_p))
      == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if (argc == 0U)
    {
      CST_HelpCmd();
    }
    else if(memcmp((CRC_CHAR_t *)argv_p[0],  "help",  crs_strlen(argv_p[0])) == 0)
    {
      CST_HelpCmd();
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "polling", crs_strlen(argv_p[0])) == 0)
    {
      if (argc == 2U)
      {
        if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
        {
          CST_polling_active = 0U;
        }
        else
        {
          CST_polling_active = 1U;
        }
      }

      if (CST_polling_active == 0U)
      {
        PrintForce("%s polling disable", CST_cmd_label)
      }
      else
      {
        PrintForce("%s polling enable", CST_cmd_label)
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0], "targetstate", crs_strlen(argv_p[0])) == 0)
    {
      if (argc == 2U)
      {
        if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
        {
          target_state.rt_state     = DC_SERVICE_ON;
          target_state.target_state = DC_TARGET_STATE_OFF;
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
        }
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "sim", crs_strlen(argv_p[1])) == 0)
        {
          target_state.rt_state     = DC_SERVICE_ON;
          target_state.target_state = DC_TARGET_STATE_SIM_ONLY;
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
        }
        else if (memcmp((CRC_CHAR_t *)argv_p[1], "full", crs_strlen(argv_p[1])) == 0)
        {
          target_state.rt_state     = DC_SERVICE_ON;
          target_state.target_state = DC_TARGET_STATE_FULL;
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
        }
        else
        {
          /* Nothing to do */
        }
        PrintForce("New modem target state   : %s", CST_TargetStateName_p[target_state.target_state])
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0],
                     "state",
                     crs_strlen(argv_p[0]))
             == 0)
    {
      /* cellular service state */
      PrintForce("Cellular Service Task State")
      (void)dc_com_read(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_cmd_sim_info, sizeof(cst_cmd_sim_info));
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cst_cmd_cellular_params, sizeof(cst_cmd_cellular_params));
      PrintForce("Current State  : %s", CST_StateName_p[CST_current_state])

      PrintForce("Sim Selected   : %s", CST_SimSlotName_p[cst_cmd_sim_info.active_slot])

      PrintForce("Sim %s         : %s", CST_SimSlotName_p[cst_cmd_cellular_params.sim_slot[0].sim_slot_type],
                 CST_SimModeName_p[cst_cmd_sim_info.sim_status[0]])
      PrintForce("Sim %s         : %s", CST_SimSlotName_p[cst_cmd_cellular_params.sim_slot[1].sim_slot_type],
                 CST_SimModeName_p[cst_cmd_sim_info.sim_status[1]])
      PrintForce("Sim %s         : %s", CST_SimSlotName_p[cst_cmd_cellular_params.sim_slot[2].sim_slot_type],
                 CST_SimModeName_p[cst_cmd_sim_info.sim_status[2]])

      if (cst_cmd_cellular_params.nfmc_active != 0U)
      {
        (void)dc_com_read(&dc_com_db, DC_COM_NFMC_TEMPO, (void *)&cst_cmd_nfmc_info, sizeof(cst_cmd_nfmc_info));
        for (i = 0U; i < DC_NFMC_TEMPO_NB ; i++)
        {
          PrintForce("nmfc tempo %ld   : %ld", i + 1U, cst_cmd_nfmc_info.tempo[i])
        }
      }
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0],
                     "info",
                     crs_strlen(argv_p[0])) == 0)
    {
      /* cellular service info */
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_INFO, (void *)&cst_cmd_cellular_info, sizeof(cst_cmd_cellular_info));
      (void)dc_com_read(&dc_com_db, DC_COM_SIM_INFO, (void *)&cst_cmd_sim_info, sizeof(cst_cmd_sim_info));
      PrintForce("Cellular Service Infos ")
      PrintForce("Modem state      : %d (%s)", cst_cmd_cellular_info.modem_state,
                 CST_ModemStateName_p[cst_cmd_cellular_info.modem_state])
      PrintForce("Signal Quality   : %ld", cst_cmd_cellular_info.cs_signal_level)
      PrintForce("Signal level(dBm): %ld", cst_cmd_cellular_info.cs_signal_level_db)

      PrintForce("Operator name   : %s", cst_cmd_cellular_info.mno_name)
      PrintForce("IMEI            : %s", cst_cmd_cellular_info.imei)
      PrintForce("Manuf name      : %s", cst_cmd_cellular_info.manufacturer_name)
      PrintForce("Model           : %s", cst_cmd_cellular_info.model)
      PrintForce("Revision        : %s", cst_cmd_cellular_info.revision)
      PrintForce("Serial Number   : %s", cst_cmd_cellular_info.serial_number)
      PrintForce("ICCID           : %s", cst_cmd_cellular_info.iccid)
      PrintForce("IMSI            : %s", cst_cmd_sim_info.imsi)
    }
    else if (memcmp((CRC_CHAR_t *)argv_p[0],
                     "config",
                     crs_strlen(argv_p[0]))
             == 0)
    {
      /* cellular service config */
      PrintForce("Cellular Service Task Config")
      (void)dc_com_read(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cst_cmd_cellular_params, sizeof(cst_cmd_cellular_params));
      for (i = 0 ; i < cst_cmd_cellular_params.sim_slot_nb ; i++)
      {
        PrintForce("Sim Slot     : %ld (%s)", i, CST_SimSlotName_p[cst_cmd_cellular_params.sim_slot[i].sim_slot_type])
        PrintForce("APN          : \"%s\"", cst_cmd_cellular_params.sim_slot[i].apn)
        PrintForce("CID          : %d", cst_cmd_cellular_params.sim_slot[i].cid)
        PrintForce("username     : %s", cst_cmd_cellular_params.sim_slot[i].username)
        PrintForce("password     : %s", cst_cmd_cellular_params.sim_slot[i].password)
      }

      PrintForce("Target state : %s", CST_TargetStateName_p[cst_cmd_cellular_params.target_state])

      PrintForce("nmfc mode    : %s", CST_ActivateName_p[cst_cmd_cellular_params.nfmc_active])
      if (cst_cmd_cellular_params.nfmc_active != 0U)
      {
        for (i = 0U; i < DC_NFMC_TEMPO_NB ; i++)
        {
          PrintForce("nmfc value %ld : %ld", i + 1U, cst_cmd_cellular_params.nfmc_value[i])
        }
      }
    }
    else
    {
      cmd_status = CMD_SYNTAX_ERROR;
      PrintForce("%s bad command. Usage:", cmd_p)
      CST_HelpCmd();
    }
  }
  return cmd_status;
}


#if (( CST_CMD_MODEM_TYPE1SC == 1) || ( CST_CMD_MODEM_BG96 == 1 ))
static uint32_t CST_CMD_get_band(const CST_band_descr_t *band_descr,
                                 const uint8_t   *const *argv_p, uint32_t argc,
                                 uint32_t *band_result_MSB, uint32_t *band_result_LSB)
{
  uint32_t i;
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint32_t band_value_MSB;
  uint32_t band_value_LSB;
  uint32_t ret;

  ret = 0;
  band_value_MSB = 0;
  band_value_LSB = 0;

  nb_band = argc - 2U;

  for (j = 0U; j < nb_band; j++)
  {
    current_arg = j + 2U;
    for (i = 0U ; band_descr[i].name != NULL ; i++)
    {
      if (memcmp((const CRC_CHAR_t *)argv_p[current_arg],
                  ( CRC_CHAR_t *)(band_descr[i].name),
                  crs_strlen((const uint8_t*)argv_p[current_arg]))
          == 0)
      {
        band_value_MSB = band_value_MSB | band_descr[i].value_MSB;
        band_value_LSB = band_value_LSB | band_descr[i].value_LSB;
        break;
      }
    }

    if (band_descr[i].name == NULL)
    {
      ret = 1;
    }
  }

  *band_result_MSB = band_value_MSB;
  *band_result_LSB = band_value_LSB;

  PrintForce("")
  return ret;
}

static void  CST_CMD_display_bitmap_name(uint32_t bitmap_MSB, uint32_t bitmap_LSB, const CST_band_descr_t *bitmap_descr)
{
  uint32_t i;
  uint32_t bmask_msb;
  uint32_t bmask_lsb;
  uint32_t bitmap_value_msb;
  uint32_t bitmap_value_lsb;
  uint8_t *bitmap_name;

  for (i = 0U; bitmap_descr[i].name != NULL ; i++)
  {
    bitmap_value_msb = bitmap_descr[i].value_MSB;
    bitmap_value_lsb = bitmap_descr[i].value_LSB;
    bitmap_name  = bitmap_descr[i].name;

    bmask_msb = bitmap_MSB & bitmap_value_msb;
    bmask_lsb = bitmap_LSB & bitmap_value_lsb;

    if (((bmask_msb != 0U) || (bmask_lsb != 0U)) &&
        (bmask_msb == bitmap_value_msb) &&
        (bmask_lsb == bitmap_value_lsb))
    {
      PrintForce("%s", bitmap_name)
    }
  }
}
#endif   /* (( CST_CMD_MODEM_TYPE1SC == 1) || ( CST_CMD_MODEM_BG96 == 1 )) */

#if (CST_CMD_MODEM_BG96 == 1)
static void  CST_CMD_display_seq_name(uint32_t bitmap, const CST_seq_descr_t *bitmap_descr)
{
  uint32_t i;

  for (i = 0U; bitmap_descr[i].name != NULL ; i++)
  {
    if (bitmap == bitmap_descr[i].value)
    {
      PrintForce("%s", bitmap_descr[i].name)
    }
  }
}

static void CST_ModemHelpCmd(void)
{
  CMD_print_help(CST_cmd_modem_label);

  PrintForce("Modem configuration commands are used to modify the modem band configuration.")
  PrintForce("Setting a new configuration is performed in two steps:")
  PrintForce("\n\r");

  PrintForce("- 1st step: enter the configuration parameters using the following commands:");
  PrintForce("%s config iotopmode [M1|NB1|ALL]  sets iotop mode)", CST_cmd_modem_label)
  PrintForce("%s config nwscanmode [GSM|LTE|AUTO]  (sets scan mode)", CST_cmd_modem_label)
  PrintForce("%s config gsmband [900] [1800] [850] [1900] [nochange] [any]   (sets the list of GSM bands to use)",
             CST_cmd_modem_label)
  PrintForce("%s config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28] [B39] [nochange] [any]  (sets the list of M1 bands to use)",
             CST_cmd_modem_label)
  PrintForce("%s config nb1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28] [nochange] [any]  (sets the list of NB1 bands to use)",
             CST_cmd_modem_label)
  PrintForce("%s config scanseq GSM_NB1_M1|GSM_M1_NB1|M1_GSM_NB1|M1_NB1_GSM|NB1_GSM_M1|NB1_M1_GSM|GSM|M1|NB1 (sets the sequence order to scan)",
             CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("- 2nd step: send the new configuration to the modem");
  PrintForce("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("Other commands:");
  PrintForce("%s config get (get current config from modem)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PrintForce("    (Note: the result of this command displays trace of modem response)")
  PrintForce("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("Notes:");
  PrintForce("- To use these commands, it is advised to start firmware in 'Modem power on' mode (option '2' of the boot menu).");
  PrintForce("- The new modem configuration is taken into account only after target reboot.");
}

static cmd_status_t CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint32_t           cst_cmd_nwscanmode_default     = QCFGSCANMODE_AUTO;
  static uint32_t           cst_cmd_iotopmode_default      = QCFGIOTOPMODE_CATM1;
  static uint32_t           cst_cmd_gsmband_MSB_default    = 0;
  static uint32_t           cst_cmd_gsmband_LSB_default    = QCFGBANDGSM_ANY;
  static uint32_t           cst_cmd_m1band_MSB_default     = QCFGBANDCATM1_B4_MSB | QCFGBANDCATM1_B13_MSB;
  static uint32_t           cst_cmd_m1band_LSB_default     = QCFGBANDCATM1_B4_LSB | QCFGBANDCATM1_B13_LSB;
  static uint32_t           cst_cmd_nb1band_MSB_default    = QCFGBANDCATNB1_ANY_MSB;
  static uint32_t           cst_cmd_nb1band_LSB_default    = QCFGBANDCATNB1_ANY_LSB;
  static uint32_t           cst_cmd_scanseq_default        = 0x020301;
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];

  static const uint8_t *CST_ScanmodeName_p[] =
  {
    ((uint8_t *)"AUTO"),
    ((uint8_t *)"GSM"),
    ((uint8_t *)"LTE")
  };

  static const uint8_t *CST_IotopmodeName_p[] =
  {
    ((uint8_t *)"M1"),
    ((uint8_t *)"NB1"),
    ((uint8_t *)"ALL")
  };


  static const CST_band_descr_t CST_Nb1band[] =
  {
    /* name                   value_MSB                     value_LSB    */
    {((uint8_t *)"B1"),       QCFGBANDCATNB1_B1_MSB,        QCFGBANDCATNB1_B1_LSB},
    {((uint8_t *)"B2"),       QCFGBANDCATNB1_B2_MSB,        QCFGBANDCATNB1_B2_LSB},
    {((uint8_t *)"B3"),       QCFGBANDCATNB1_B3_MSB,        QCFGBANDCATNB1_B3_LSB},
    {((uint8_t *)"B4"),       QCFGBANDCATNB1_B4_MSB,        QCFGBANDCATNB1_B4_LSB},
    {((uint8_t *)"B5"),       QCFGBANDCATNB1_B5_MSB,        QCFGBANDCATNB1_B5_LSB},
    {((uint8_t *)"B8"),       QCFGBANDCATNB1_B8_MSB,        QCFGBANDCATNB1_B8_LSB},
    {((uint8_t *)"B12"),      QCFGBANDCATNB1_B12_MSB,       QCFGBANDCATNB1_B12_LSB},
    {((uint8_t *)"B13"),      QCFGBANDCATNB1_B13_MSB,       QCFGBANDCATNB1_B13_LSB},
    {((uint8_t *)"B18"),      QCFGBANDCATNB1_B18_MSB,       QCFGBANDCATNB1_B18_LSB},
    {((uint8_t *)"B19"),      QCFGBANDCATNB1_B19_MSB,       QCFGBANDCATNB1_B19_LSB},
    {((uint8_t *)"B20"),      QCFGBANDCATNB1_B20_MSB,       QCFGBANDCATNB1_B20_LSB},
    {((uint8_t *)"B26"),      QCFGBANDCATNB1_B26_MSB,       QCFGBANDCATNB1_B26_LSB},
    {((uint8_t *)"B28"),      QCFGBANDCATNB1_B28_MSB,       QCFGBANDCATNB1_B28_LSB},
    {((uint8_t *)"nochange"), QCFGBANDCATNB1_NOCHANGE_MSB,  QCFGBANDCATNB1_NOCHANGE_LSB},
    {((uint8_t *)"any"),      QCFGBANDCATNB1_ANY_MSB,       QCFGBANDCATNB1_ANY_LSB},
    {NULL,      0, 0}   /* Mandatory: End of table */
  };

  static const CST_band_descr_t CST_M1band[] =
  {
    /* name                   value_MSB                    value_LSB    */
    {((uint8_t *)"B1"),       QCFGBANDCATM1_B1_MSB,        QCFGBANDCATM1_B1_LSB},
    {((uint8_t *)"B2"),       QCFGBANDCATM1_B2_MSB,        QCFGBANDCATM1_B2_LSB},
    {((uint8_t *)"B3"),       QCFGBANDCATM1_B3_MSB,        QCFGBANDCATM1_B3_LSB},
    {((uint8_t *)"B4"),       QCFGBANDCATM1_B4_MSB,        QCFGBANDCATM1_B4_LSB},
    {((uint8_t *)"B5"),       QCFGBANDCATM1_B5_MSB,        QCFGBANDCATM1_B5_LSB},
    {((uint8_t *)"B8"),       QCFGBANDCATM1_B8_MSB,        QCFGBANDCATM1_B8_LSB},
    {((uint8_t *)"B12"),      QCFGBANDCATM1_B12_MSB,       QCFGBANDCATM1_B12_LSB},
    {((uint8_t *)"B13"),      QCFGBANDCATM1_B13_MSB,       QCFGBANDCATM1_B13_LSB},
    {((uint8_t *)"B18"),      QCFGBANDCATM1_B18_MSB,       QCFGBANDCATM1_B18_LSB},
    {((uint8_t *)"B19"),      QCFGBANDCATM1_B19_MSB,       QCFGBANDCATM1_B19_LSB},
    {((uint8_t *)"B20"),      QCFGBANDCATM1_B20_MSB,       QCFGBANDCATM1_B20_LSB},
    {((uint8_t *)"B26"),      QCFGBANDCATM1_B26_MSB,       QCFGBANDCATM1_B26_LSB},
    {((uint8_t *)"B28"),      QCFGBANDCATM1_B28_MSB,       QCFGBANDCATM1_B28_LSB},
    {((uint8_t *)"B39"),      QCFGBANDCATM1_B39_MSB,       QCFGBANDCATM1_B39_LSB},
    {((uint8_t *)"nochange"), QCFGBANDCATM1_NOCHANGE_MSB,  QCFGBANDCATM1_NOCHANGE_LSB},
    {((uint8_t *)"any"),      QCFGBANDCATM1_ANY_MSB,       QCFGBANDCATM1_ANY_LSB},
    {NULL,      0, 0}  /* Mandatory: End of table */
  };

  static const CST_band_descr_t CST_GSMband[] =
  {
    /* name                   value_MSB      value_LSB    */
    {((uint8_t *)"900"),      0,             QCFGBANDGSM_900,},
    {((uint8_t *)"1800"),     0,             QCFGBANDGSM_1800},
    {((uint8_t *)"850"),      0,             QCFGBANDGSM_850},
    {((uint8_t *)"1900"),     0,             QCFGBANDGSM_1900},
    {((uint8_t *)"nochange"), 0,             QCFGBANDGSM_NOCHANGE},
    {((uint8_t *)"any"),      0,             QCFGBANDGSM_ANY},
    {NULL,      0, 0}  /* Mandatory: End of table */
  };

  static const CST_seq_descr_t CST_Scanseq[CST_CMD_SCANSEQ_NUMBER] =
  {
    /* name                        */
    {((uint8_t *)"GSM_M1_NB1"),    QCFGSCANSEQ_GSM_M1_NB1},
    {((uint8_t *)"GSM_NB1_M1"),    QCFGSCANSEQ_GSM_NB1_M1},
    {((uint8_t *)"M1_GSM_NB1"),    QCFGSCANSEQ_M1_GSM_NB1},
    {((uint8_t *)"M1_NB1_GSM"),    QCFGSCANSEQ_M1_NB1_GSM},
    {((uint8_t *)"NB1_GSM_M1"),    QCFGSCANSEQ_NB1_GSM_M1},
    {((uint8_t *)"NB1_M1_GSM"),    QCFGSCANSEQ_NB1_M1_GSM},
    {((uint8_t *)"NB1_M1"),    CST_scanseq_NB1_M1    },
    {((uint8_t *)"NB1_GSM"),    CST_scanseq_NB1_GSM   },
    {((uint8_t *)"M1_GSM"),    CST_scanseq_M1_GSM    },
    {((uint8_t *)"M1_NB1"),    CST_scanseq_M1_NB1    },
    {((uint8_t *)"GSM_M1"),    CST_scanseq_GSM_M1    },
    {((uint8_t *)"GSM_NB1"),    CST_scanseq_GSM_NB1   },
    {((uint8_t *)"NB1"),    CST_scanseq_NB1       },
    {((uint8_t *)"M1"),    CST_scanseq_M1        },
    {((uint8_t *)"GSM"),    CST_scanseq_GSM       },
    {NULL,    0}   /* Mandatory: End of table */
  };

  static const uint8_t *CST_ScanseqName_p[CST_CMD_SCANSEQ_NUMBER] =
  {
    ((uint8_t *)"GSM_M1_NB1"),
    ((uint8_t *)"GSM_NB1_M1"),
    ((uint8_t *)"M1_GSM_NB1"),
    ((uint8_t *)"M1_NB1_GSM"),
    ((uint8_t *)"NB1_GSM_M1"),
    ((uint8_t *)"NB1_M1_GSM"),
    ((uint8_t *)"NB1_M1"),
    ((uint8_t *)"NB1_GSM"),
    ((uint8_t *)"M1_GSM"),
    ((uint8_t *)"M1_NB1"),
    ((uint8_t *)"GSM_M1"),
    ((uint8_t *)"GSM_NB1"),
    ((uint8_t *)"NB1"),
    ((uint8_t *)"M1"),
    ((uint8_t *)"GSM"),
    NULL
  };

  uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  uint32_t    ret;
  uint8_t    *cmd_p;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PrintForce("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (memcmp((CRC_CHAR_t *)cmd_p,
              (CRC_CHAR_t *)CST_cmd_modem_label,
              crs_strlen(cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if (argc == 0U)
    {
      CST_ModemHelpCmd();
    }
    else if(memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen((uint8_t*)argv_p[0])) == 0)
    {
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                     "config",
                     crs_strlen((uint8_t*)argv_p[0]))
             == 0)
    {
      /* nwscanmode */
      if (argc == 1U)
      {
        PrintForce("scanmode  : (mask=0x%08lx) %s", cst_cmd_nwscanmode_default, CST_ScanmodeName_p[cst_cmd_nwscanmode_default])
        PrintForce("iotopmode : (mask=0x%08lx) %s", cst_cmd_iotopmode_default, CST_IotopmodeName_p[cst_cmd_iotopmode_default])
        PrintForce("GSM bands : (mask=0x%lx%08lx)", cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default)
        CST_CMD_display_bitmap_name(cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default, CST_GSMband);
        PrintForce("M1 bands  : (mask=0x%lx%08lx)", cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default)
        CST_CMD_display_bitmap_name(cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default, CST_M1band);
        PrintForce("NB1 bands : (mask=0x%lx%08lx)", cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default)
        CST_CMD_display_bitmap_name(cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default, CST_Nb1band);

        PrintForce("Scan seq : (mask=0x%06lx)", cst_cmd_scanseq_default)
        CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "nwscanmode",
                       crs_strlen((uint8_t*)argv_p[1])) == 0)
      {
        if (argc == 3U)
        {
          if (memcmp((const CRC_CHAR_t *)argv_p[2],
                      "AUTO",
                      crs_strlen((uint8_t*)argv_p[2]))
              == 0)
          {
            cst_cmd_nwscanmode_default = QCFGSCANMODE_AUTO;
          }
          else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                           "GSM",
                           crs_strlen((uint8_t*)argv_p[2]))
                   == 0)
          {
            cst_cmd_nwscanmode_default = QCFGSCANMODE_GSMONLY;
          }
          else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                           "LTE",
                           crs_strlen((uint8_t*)argv_p[2])) == 0)
          {
            cst_cmd_nwscanmode_default = QCFGSCANMODE_LTEONLY;
          }
          else
          {
            PrintForce("%s %s Bad parameter: %s", (CRC_CHAR_t *)CST_cmd_modem_label, argv_p[1], argv_p[2])
            PrintForce("Usage: %s config nwscanmode [GSM|LTE|AUTO]", CST_cmd_modem_label)
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
        PrintForce("scanmode: %s\n\r", CST_ScanmodeName_p[cst_cmd_nwscanmode_default])
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "iotopmode",
                       crs_strlen((uint8_t*)argv_p[1])) == 0)
      {
        if (argc == 3U)
        {
          if (memcmp((const CRC_CHAR_t *)argv_p[2],
                      "M1",
                      crs_strlen((uint8_t*)argv_p[2]))
              == 0)
          {
            cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATM1;
          }
          else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                           "NB1",
                           crs_strlen((uint8_t*)argv_p[2]))
                   == 0)
          {
            cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATNB1;
          }
          else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                           "ALL",
                           crs_strlen((uint8_t*)argv_p[2]))
                   == 0)
          {
            cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATM1CATNB1;
          }
          else
          {
            PrintForce("%s %s Bad parameter: %s", (CRC_CHAR_t *)CST_cmd_modem_label, argv_p[1], argv_p[2])
            PrintForce("Usage: %s config iotopmode [M1|NB1|ALL]", CST_cmd_modem_label)
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
        PrintForce("iotopmode: %s\n\r", CST_IotopmodeName_p[cst_cmd_iotopmode_default])
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "gsmband",
                       crs_strlen((uint8_t*)argv_p[1]))
               == 0)
      {
        if (argc >= 3U)
        {
          uint32_t gsmband_value_msb, gsmband_value_lsb;
          ret = CST_CMD_get_band(CST_GSMband, argv_p,  argc, &gsmband_value_msb, &gsmband_value_lsb);
          if (ret != 0U)
          {
            cmd_status = CMD_SYNTAX_ERROR;
            PrintForce("%s Bad parameter", CST_cmd_modem_label)
            PrintForce("Usage:%s config gsmband [900] [1800] [850] [1900] [nochange] [any]", CST_cmd_modem_label)
          }
          else
          {
            cst_cmd_gsmband_MSB_default = gsmband_value_msb;
            cst_cmd_gsmband_LSB_default = gsmband_value_lsb;
          }
        }
        PrintForce("Gsm Bands: (mask=0x%lx%08lx)\n\r", cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default)
        CST_CMD_display_bitmap_name(cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default, CST_GSMband);
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "m1band",
                       crs_strlen((uint8_t*)argv_p[1]))
               == 0)
      {
        if (argc >= 3U)
        {
          uint32_t m1band_value_msb, m1band_value_lsb;
          ret = CST_CMD_get_band(CST_M1band, argv_p,  argc, &m1band_value_msb, &m1band_value_lsb);
          if (ret != 0U)
          {
            PrintForce("%s Bad parameter", CST_cmd_modem_label)
            PrintForce("Usage:%s config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28] [B39] [nchanche] [any]",
                       CST_cmd_modem_label)
            cmd_status = CMD_SYNTAX_ERROR;
          }
          else
          {
            cst_cmd_m1band_MSB_default = m1band_value_msb;
            cst_cmd_m1band_LSB_default = m1band_value_lsb;
          }
        }
        PrintForce("M1 Bands: (mask=0x%lx%08lx)\n\r", cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default)
        CST_CMD_display_bitmap_name(cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default, CST_M1band);
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "nb1band",
                       crs_strlen((uint8_t*)argv_p[1]))
               == 0)
      {
        if (argc >= 3U)
        {
          uint32_t nb1band_value_msb, nb1band_value_lsb;
          ret = CST_CMD_get_band(CST_Nb1band, argv_p, argc, &nb1band_value_msb, &nb1band_value_lsb);
          if (ret != 0U)
          {
            PrintForce("%s Bad parameter", CST_cmd_modem_label)
            PrintForce("Usage: modem config nb1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28] [nchanche] [any]")
            cmd_status = CMD_SYNTAX_ERROR;
          }
          else
          {
            cst_cmd_nb1band_MSB_default = nb1band_value_msb;
            cst_cmd_nb1band_LSB_default = nb1band_value_lsb;
          }
        }
        PrintForce("NB1 bands: (mask=0x%lx%08lx)", cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default)
        CST_CMD_display_bitmap_name(cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default, CST_Nb1band);
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "scanseq",
                       crs_strlen((uint8_t*)argv_p[1]))
               == 0)
      {
        if (argc == 2U)
        {
          PrintForce("Scan Seq : (%06lx)", cst_cmd_scanseq_default)
          CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
        }
        else if (argc == 3U)
        {
          uint32_t i;
          for (i = 0U ; i < CST_CMD_SCANSEQ_NUMBER ; i++)
          {
            if (memcmp((const CRC_CHAR_t *)argv_p[2],
                        (const CRC_CHAR_t *)CST_ScanseqName_p[i],
                        crs_strlen(CST_ScanseqName_p[i]))
                == 0)
            {
              cst_cmd_scanseq_default = CST_Scanseq[i].value;
              break;
            }
          }

          if (i == CST_CMD_SCANSEQ_NUMBER)
          {
            cmd_status = CMD_SYNTAX_ERROR;
            PrintForce("bad command: %s %s %s\n\r", cmd_p, argv_p[1], argv_p[2])
          }
          else
          {
            PrintForce("Scan seq : (0x%06lx)", cst_cmd_scanseq_default)
            CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
          }
        }
        else
        {
          cmd_status = CMD_SYNTAX_ERROR;
          PrintForce("Too many parameters command: %s %s \n\r", cmd_p, argv_p[1])
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen((uint8_t*)argv_p[1])) == 0)
      {
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanmode\",%ld,1", cst_cmd_nwscanmode_default);
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"iotopmode\",%ld,1", cst_cmd_iotopmode_default);
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanseq\",%06lx,1", cst_cmd_scanseq_default);
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"band\",%lx%08lx,%lx%08lx,%lx%08lx,1",
                      cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default,
                      cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default,
                      cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default);
        ret  |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
        if (ret != 0U)
        {
          PrintForce("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen((uint8_t*)argv_p[1])) == 0)
      {
        PrintForce("nwscanmode:")
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanmode\"");
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        PrintForce("iotopmode:")
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"iotopmode\"");
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        PrintForce("GSM Bands:")
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"band\"");
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        PrintForce("Scan Seq:")
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanseq\"");
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
        if (ret != 0U)
        {
          PrintForce("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else
      {
        cmd_status = CMD_SYNTAX_ERROR;
        PrintForce("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      cmd_status = CMD_SYNTAX_ERROR;
      PrintForce("bad command: %s %s\n\r", cmd_p, argv_p[0])
    }
  }

  PrintForce("")
  return cmd_status;

}
#endif /* defined(CST_CMD_MODEM_BG96 == 1) */

#if (CST_CMD_MODEM_TYPE1SC == 1)

static void CST_ModemHelpCmd(void)
{
  CMD_print_help(CST_cmd_modem_label);

  PrintForce("Modem configuration commands are used to modify the modem band configuration.")
  PrintForce("Setting a new configuration is performed in two steps:")
  PrintForce("\n\r");

  PrintForce("- 1st step: enter the configuration parameters using the following commands:");
  PrintForce("%s config band [B13] [B20]  (gets/sets the bands to use)",
             CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("- 2nd step: send the new configuration to the modem");
  PrintForce("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("Other commands:");
  PrintForce("%s config get (get current config from modem)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PrintForce("    (Note: the result of this command displays trace of modem response)")
  PrintForce("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("Notes:");
  PrintForce("- To use these commands, it is advised to start firmware in 'Modem power on' mode (option '2' of the boot menu).");
  PrintForce("- The new modem configuration is taken into account only after target reboot.");
}


static cmd_status_t CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint32_t           cst_cmd_band_LSB_default     = CST_BAND_MASK_13_LSB ;
  static uint32_t           cst_cmd_band_MSB_default     = CST_BAND_MASK_13_MSB ;
  static const CST_band_descr_t CST_band[] =
  {
    /* name                   value_MSB      value_LSB    */
    {((uint8_t *)"B13"), CST_BAND_MASK_13_MSB, CST_BAND_MASK_13_LSB},
    {((uint8_t *)"B20"), CST_BAND_MASK_20_MSB, CST_BAND_MASK_20_LSB},
    {NULL,      0, 0}  /* Mandatory: End of table */
  };

  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];
  static uint8_t *cst_cmd_bands;

  const uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  uint32_t    ret;
  uint8_t    *cmd_p;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PrintForce("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (memcmp((CRC_CHAR_t *)cmd_p,
              (CRC_CHAR_t *)CST_cmd_modem_label,
              crs_strlen(cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if ((argc == 0U)
        || (memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0))
    {
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                     "config",
                     crs_strlen(argv_p[0]))
             == 0)
    {
      /* nwscanmode */
      if (argc == 1U)
      {
        PrintForce("bands : ")
        CST_CMD_display_bitmap_name(cst_cmd_band_MSB_default, cst_cmd_band_LSB_default, CST_band);
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "bands",
                       crs_strlen(argv_p[1]))
               == 0)
      {
        if (argc >= 3U)
        {
          uint32_t band_value_msb, band_value_lsb;
          ret = CST_CMD_get_band(CST_band, argv_p,  argc, &band_value_msb, &band_value_lsb);
          if (ret != 0U)
          {
            PrintForce("%s Bad parameter", CST_cmd_modem_label)
            PrintForce("Usage:%s config m1band [B13] [B20]",
                       CST_cmd_modem_label)
            cmd_status = CMD_SYNTAX_ERROR;
          }
          else
          {
            cst_cmd_band_MSB_default = band_value_msb;
            cst_cmd_band_LSB_default = band_value_lsb;
          }
        }
        CST_CMD_display_bitmap_name(cst_cmd_band_MSB_default, cst_cmd_band_LSB_default, CST_band);
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
      {
        if (cst_cmd_band_LSB_default == 1)
        {
          cst_cmd_bands = "\"13\"";
        }
        else if (cst_cmd_band_LSB_default == 2)
        {
          cst_cmd_bands = "\"20\"";
        }
        else
        {
          cst_cmd_bands = "\"13,\"20\"";
        }


        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETCFG=\"BAND\",%s", (CRC_CHAR_t *)cst_cmd_bands);
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        if (ret != 0)
        {
          PrintForce("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
      {

        PrintForce("GSM Bands:")
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETCFG=\"BAND\"");
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
        if (ret != 0)
        {
          PrintForce("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else
      {
        cmd_status = CMD_SYNTAX_ERROR;
        PrintForce("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      cmd_status = CMD_SYNTAX_ERROR;
      PrintForce("bad command: %s %s\n\r", cmd_p, argv_p[0])
    }
  }

  PrintForce("")
  return cmd_status;

}
#endif /* defined(CST_CMD_MODEM_TYPE1SC == 1) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)

static void CST_ModemHelpCmd(void)
{
  CMD_print_help(CST_cmd_modem_label);

  PrintForce("Modem configuration commands are used to modify the modem band configuration.")
  PrintForce("Setting a new configuration is performed in two steps:")
  PrintForce("\n\r");

  PrintForce("- 1st step: enter the configuration parameters using the following commands:");
  PrintForce("%s config band  <1-256>...<1-256>   (gets/sets the bands to use)",
                           CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("- 2nd step: send the new configuration to the modem");
  PrintForce("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("Other command:");
  PrintForce("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PrintForce("\n\r");

  PrintForce("Notes:");
  PrintForce("- To use these commands, it is advised to start firmware in 'Modem power on' mode (option '2' of the boot menu).");
  PrintForce("- The new modem configuration is taken into account only after target reboot.");
}

/* Default bands: 4/12 */

static uint32_t CST_CMD_get_band_sequans(const uint8_t *const *argv_p, uint32_t argc)
{
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint8_t  band_value;
  uint32_t ret;
  ret = 0U;

  nb_band = argc - 2U;

  CST_CMD_band_count = 0U;
  (void)memset(CST_CMD_band_tab, 0, sizeof(CST_CMD_band_tab));

  for (j = 0U; j < nb_band; j++)
  {
    current_arg = j + 2U;
    band_value = (uint8_t)crs_atoi(argv_p[current_arg]);
    if (band_value == 0U)
    {
      CST_CMD_band_count = 0;
      break;
    }
    CST_CMD_band_tab[CST_CMD_band_count] = (band_value-1U);
    CST_CMD_band_count++;
  }

  if (CST_CMD_band_count == 0U)
  {
    ret = 1U;
  }

  return ret;
}

static void  CST_CMD_display_bitmap_name_sequans(void)
{
  uint32_t i;

  for (i = 0U; i<CST_CMD_band_count  ; i++)
  {
      PrintForce("%d", CST_CMD_band_tab[i]+1U)
  }
}

static cmd_status_t CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];

  const uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  uint32_t    ret;
  uint8_t    *cmd_p;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;
  uint32_t i;

  PrintForce("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
  if (memcmp((CRC_CHAR_t *)cmd_p,
              (CRC_CHAR_t *)CST_cmd_modem_label,
              crs_strlen(cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if (argc == 0U)
    {
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
    {
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                     "config",
                     crs_strlen(argv_p[0]))
             == 0)
    {
      /* nwscanmode */
      if (argc == 1U)
      {
        PrintForce("bands : ")
        CST_CMD_display_bitmap_name_sequans();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                       "bands",
                       crs_strlen(argv_p[1]))
               == 0)
      {
        if(argc >= 3U)
        {
           ret = CST_CMD_get_band_sequans(argv_p,  argc);
           if(ret != 0U)
           {
             PrintForce("%s Bad parameter", CST_cmd_modem_label )
             PrintForce("Usage:%s config m1band <1-256>...<1-256>   (12 bands max)",
                           CST_cmd_modem_label)
             cmd_status = CMD_SYNTAX_ERROR;
           }
        }
        CST_CMD_display_bitmap_name_sequans();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
      {
        ret = (uint32_t)cst_at_command_handle((uint8_t*)"AT!=\"clearscanconfig\"");
        if(ret != 0U)
        {
          PrintForce("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }

        for(i=0; i<CST_CMD_band_count; i++)
        {
          (void)sprintf((CRC_CHAR_t*)CST_CMD_Command,"AT!=\"addScanBand band=%d\"", CST_CMD_band_tab[i]+1U);
          ret = (uint32_t)cst_at_command_handle((uint8_t*)CST_CMD_Command);

          if(ret != 0U)
          {
            PrintForce("command fail\n\r")
            cmd_status = CMD_PROCESS_ERROR;
            break;
          }
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
      {

        PrintForce("GSM Bands:")
        (void)sprintf((CRC_CHAR_t*)CST_CMD_Command,"AT!=addScanBand");
        ret = (uint32_t)cst_at_command_handle((uint8_t*)CST_CMD_Command);
        if(ret != 0U)
        {
          PrintForce("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else
      {
        cmd_status = CMD_SYNTAX_ERROR;
        PrintForce("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      cmd_status = CMD_SYNTAX_ERROR;
      PrintForce("bad command: %s %s\n\r", cmd_p, argv_p[0])
    }
  }

  PrintForce("")
  return cmd_status;

}
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */

static void CST_cellular_direct_cmd_callback(CS_direct_cmd_rx_t direct_cmd_rx)
{
  UNUSED(direct_cmd_rx);
}


static void cst_at_cmd_help(void)
{
  CMD_print_help(CST_cmd_at_label);

  PrintForce("%s timeout [<modem response timeout(ms) (default %d)>]", CST_cmd_at_label, CST_AT_TIMEOUT)
  PrintForce("%s <at command> (send an AT comand to modem ex:atcmd AT+CSQ)", CST_cmd_at_label)
}

static cmd_status_t cst_at_command_handle(uint8_t *cmd_line_p)
{
  cmd_status_t cmd_status;
  CS_Status_t cs_status ;
  static CS_direct_cmd_tx_t CST_direct_cmd_tx;

  cmd_status = CMD_OK;

  (void)memcpy(&CST_direct_cmd_tx.cmd_str[0],
               (CRC_CHAR_t *)cmd_line_p,
               crs_strlen(cmd_line_p)+1U);
  CST_direct_cmd_tx.cmd_size    = (uint16_t)crs_strlen(cmd_line_p);
  CST_direct_cmd_tx.cmd_timeout = cst_at_timeout;

  cs_status = osCDS_direct_cmd(&CST_direct_cmd_tx, CST_cellular_direct_cmd_callback);
  if (cs_status != CELLULAR_OK)
  {
    PrintForce("\n\r%s command FAIL\n\r", cmd_line_p)
    cmd_status = CMD_PROCESS_ERROR;
  }
  return cmd_status;
}

static cmd_status_t CST_AtCmd(uint8_t *cmd_line_p)
{
  uint8_t  *argv_p[CST_CMS_PARAM_MAX];
  uint32_t i;
  uint32_t cmd_len;
  cmd_status_t cmd_status ;
  const uint8_t *cmd_p;

  cmd_status = CMD_OK;

  /* find an AT command */
  cmd_len = crs_strlen(cmd_line_p);
  for(i=0U ; i<cmd_len ; i++)
  {
    if(cmd_line_p[i] == (uint8_t)' ')
    {
      break;
    }
  }
  i++;

  if (
       (i < cmd_len)
         &&
       (
         (memcmp((const CRC_CHAR_t *)&cmd_line_p[i],(const CRC_CHAR_t *)"at",2) == 0)
           ||
         (memcmp((const CRC_CHAR_t *)&cmd_line_p[i],(const CRC_CHAR_t *)"AT",2) == 0)
       )
     )
  {
    /* AT command to process */
    cmd_status = cst_at_command_handle(&cmd_line_p[i]);
  }
  else
  {
    /* Not an AT command */
    cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
    if (memcmp((const CRC_CHAR_t *)cmd_p,
                (const CRC_CHAR_t *)CST_cmd_at_label,
                crs_strlen(cmd_p))
        == 0)
    {
      /* parameters parsing                     */
      argv_p[0] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[0] != NULL)
      {
        if (memcmp((CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
        {
          cst_at_cmd_help();
        }
        else if (memcmp((CRC_CHAR_t *)argv_p[0],
                         "timeout",
                         crs_strlen(argv_p[0]))
                 == 0)
        {
          argv_p[1] = (uint8_t *)strtok(NULL, " \t");
          if (argv_p[1] != NULL)
          {
            cst_at_timeout = (uint32_t)crs_atoi(argv_p[1]);
          }
          PrintForce("at timeout : %ld\n\r", cst_at_timeout)
        }
  #if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
        else if (memcmp((CRC_CHAR_t *)argv_p[0],
                         "suspenddata",
                         crs_strlen(argv_p[0]))
                 == 0)
        {
          CS_Status_t cs_status ;
          cs_status = osCDS_suspend_data();
          if (cs_status != CELLULAR_OK)
          {
            PrintForce("\n\rsuspend data FAIL")
            cmd_status = CMD_PROCESS_ERROR;
          }
          else
          {
            PrintForce("\n\rsuspend data OK")
          }
        }
        else if (memcmp((CRC_CHAR_t *)argv_p[0],
                         "resumedata",
                         crs_strlen(argv_p[0]))
                 == 0)
        {
          CS_Status_t cs_status ;
          cs_status = osCDS_resume_data();
          if (cs_status != CELLULAR_OK)
          {
            PrintForce("\n\rresume data FAIL")
            cmd_status = CMD_PROCESS_ERROR;
          }
          else
          {
            PrintForce("\n\rresume data OK")
          }
        }
  #endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
        else
        {
          if ((memcmp(argv_p[0], "at", 2) == 0) || (memcmp(argv_p[0], "AT", 2) == 0))
          {
            cmd_status = cst_at_command_handle(argv_p[0]);
          }
          else
          {
            PrintForce("\n\r%s bad commande. Usage:\n\r", cmd_line_p);
            cst_at_cmd_help();
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
      }
    }
    else
    {
      cst_at_cmd_help();
    }
  }
  return cmd_status;
}



CS_Status_t CST_cmd_cellular_service_start(void)
{
  CMD_Declare(CST_cmd_label, CST_cmd, (uint8_t *)"cellular service task management");
  CMD_Declare(CST_cmd_at_label, CST_AtCmd, (uint8_t *)"send an at command");
#if (CST_CMD_USE_MODEM_CONFIG == 1)
  CMD_Declare(CST_cmd_modem_label, CST_ModemCmd, (uint8_t *)"modem configuration managment");
#endif  /* CST_CMD_USE_MODEM_CONFIG == 1 */

  return CELLULAR_OK;
}

#endif  /* (USE_CMD_CONSOLE == 1) */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

