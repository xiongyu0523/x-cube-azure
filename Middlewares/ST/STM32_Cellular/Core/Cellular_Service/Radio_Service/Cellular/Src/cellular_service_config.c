/**
  ******************************************************************************
  * @file    cellular_service_config.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Config
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

#include <string.h>
#include <stdbool.h>
#include "cellular_init.h"
#include "at_util.h"
#include "cellular_service.h"
#include "cellular_service_os.h"
#include "cellular_service_config.h"
#include "cellular_service_task.h"
#include "error_handler.h"
#include "plf_config.h"
#include "cellular_service_int.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"

#if (!USE_DEFAULT_SETUP == 1)
#include "menu_utils.h"
#include "setup.h"
#include "app_select.h"
#endif  /* (!USE_DEFAULT_SETUP == 1)*/

/* Private defines -----------------------------------------------------------*/
#define CST_SETUP_NFMC       (1)

#define CST_MODEM_POWER_ON_LABEL "Modem power on (without application)"

#define GOOD_PINCODE ((uint8_t *)"") /* SET PIN CODE HERE (for exple "1234"), if no PIN code, use an string empty "" */
#define CST_MODEM_POLLING_PERIOD_DEFAULT 5000U

#define CST_LABEL "Cellular Service"
#define CST_VERSION_APPLI     (uint16_t)4 /* V4: Adding sim parameters */

#define CST_TEMP_STRING_SIZE    32




/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "", ## args)
#else
#include <stdio.h>
#define PrintForce(format, args...)                printf(format , ## args);
#endif   /* (USE_PRINTF == 0U)*/
#if (USE_TRACE_CELLULAR_SERVICE == 1U)
#if (USE_PRINTF == 0U)
#define PrintCellularService(format, args...)      TracePrint(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, format, ## args)
#define PrintCellularServiceErr(format, args...)   TracePrint(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_ERR, "ERROR " format, ## args)
#else
#define PrintCellularService(format, args...)      printf(format , ## args);
#define PrintCellularServiceErr(format, args...)   printf(format , ## args);
#endif /* USE_PRINTF */
#else
#define PrintCellularService(format, args...)   do {} while(0);
#define PrintCellularServiceErr(format, args...)  do {} while(0);
#endif /* USE_TRACE_CELLULAR_SERVICE */

/* Private typedef -----------------------------------------------------------*/
#if (USE_DEFAULT_SETUP == 1)
typedef enum
{
  CST_PARAM_SIM_SLOT     = 0,
  CST_PARAM_APN          = 1,
  CST_PARAM_CID          = 2,
  CST_PARAM_USERNAME     = 3,
  CST_PARAM_PASSWORD     = 5,
  CST_PARAM_TARGET_STATE = 6,
  CST_PARAM_NFMC         = 7,
  CST_PARAM_NFMC_TEMPO   = 8,
} CST_setup_params_t;
#endif  /* (USE_DEFAULT_SETUP == 1) */


/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
uint8_t *CST_SimSlotName_p[3] =
{
  ((uint8_t *)"MODEM SOCKET"),
  ((uint8_t *)"MODEM EMBEDDED SIM"),
  ((uint8_t *)"STM32 EMBEDDED SIM")
};
/* Private function prototypes -----------------------------------------------*/

#if (USE_DEFAULT_SETUP == 1)
static dc_cs_sim_slot_type_t  cst_get_sim_socket_value(uint8_t sim_slot_value);
static void CST_local_setup_handler(void);
#endif  /* (USE_DEFAULT_SETUP == 1) */
static CS_PDN_conf_id_t  cst_get_cid_value(uint8_t cid_value);
static dc_cs_target_state_t  cst_get_target_state_value(uint8_t target_state_value);

/* Private function Definition -----------------------------------------------*/
static dc_cs_sim_slot_type_t  cst_get_sim_socket_value(uint8_t sim_slot_value)
{
  dc_cs_sim_slot_type_t enum_value;
  switch (sim_slot_value)
  {
    case DC_SIM_SLOT_MODEM_SOCKET:
    {
      enum_value = DC_SIM_SLOT_MODEM_SOCKET;
      break;
    }
    case DC_SIM_SLOT_MODEM_EMBEDDED_SIM:
    {
      enum_value = DC_SIM_SLOT_MODEM_EMBEDDED_SIM;
      break;
    }
    case DC_SIM_SLOT_STM32_EMBEDDED_SIM:
    {
      enum_value = DC_SIM_SLOT_STM32_EMBEDDED_SIM;
      break;
    }
    default:
    {
      enum_value = DC_SIM_SLOT_MODEM_SOCKET;
      break;
    }
  }
  return enum_value;
}

static dc_cs_target_state_t  cst_get_target_state_value(uint8_t target_state_value)
{
  dc_cs_target_state_t enum_value;
  switch (target_state_value)
  {
    case DC_TARGET_STATE_OFF:
    {
      enum_value = DC_TARGET_STATE_OFF;
      break;
    }
    case DC_TARGET_STATE_SIM_ONLY:
    {
      enum_value = DC_TARGET_STATE_SIM_ONLY;
      break;
    }
    case DC_TARGET_STATE_FULL:
    {
      enum_value = DC_TARGET_STATE_FULL;
      break;
    }
    default:
    {
      enum_value = DC_TARGET_STATE_FULL;
      break;
    }
  }
  return enum_value;
}

static CS_PDN_conf_id_t  cst_get_cid_value(uint8_t cid_value)
{
  CS_PDN_conf_id_t enum_value;
  switch (cid_value)
  {
    case CS_PDN_PREDEF_CONFIG:
    {
      enum_value = CS_PDN_PREDEF_CONFIG;
      break;
    }
    case CS_PDN_USER_CONFIG_1:
    {
      enum_value = CS_PDN_USER_CONFIG_1;
      break;
    }
    case CS_PDN_USER_CONFIG_2:
    {
      enum_value = CS_PDN_USER_CONFIG_2;
      break;
    }
    case CS_PDN_USER_CONFIG_3:
    {
      enum_value = CS_PDN_USER_CONFIG_3;
      break;
    }
    case CS_PDN_USER_CONFIG_4:
    {
      enum_value = CS_PDN_USER_CONFIG_4;
      break;
    }
    case CS_PDN_USER_CONFIG_5:
    {
      enum_value = CS_PDN_USER_CONFIG_5;
      break;
    }
    case CS_PDN_CONFIG_DEFAULT:
    {
      enum_value = CS_PDN_CONFIG_DEFAULT;
      break;
    }
    case CS_PDN_NOT_DEFINED:
    {
      enum_value = CS_PDN_NOT_DEFINED;
      break;
    }
    case CS_PDN_ALL:
    {
      enum_value = CS_PDN_ALL;
      break;
    }
    default:
    {
      enum_value = CS_PDN_PREDEF_CONFIG;
      break;
    }

  }
  return enum_value;
}

#if (!USE_DEFAULT_SETUP == 1)
static void CST_setup_help(void)
{
  PrintSetup("\r\n")
  PrintSetup("===================================\r\n")
  PrintSetup("Cellular Service configuration help\r\n")
  PrintSetup("===================================\r\n")
  setup_version_help();
  PrintSetup("-------------\n\r")
  PrintSetup("Sim slot list\r\n")
  PrintSetup("-------------\n\r")
  PrintSetup("Sim slot list: specifies the list and order of SIM slots to use at boot time.\r\n")
  PrintSetup("- 0: socket slot.\r\n")
  PrintSetup("- 1: embedded SIM slot.\r\n")
  PrintSetup("- 2:host SIM slot (not implemented).\r\n")
  PrintSetup("The default value of Sim slot is '%s' (socket slot)\r\n", CST_DEFAULT_SIM_SLOT_STRING)
  PrintSetup("\r\n")
  PrintSetup("Example: If the list of SIM slots to use is:\r\n")
  PrintSetup("first 'embedded SIM slot' then 'socket slot'\r\n")
  PrintSetup("the value to set is '10' ('1' for 'embedded SIM slot' and '0' for 'socket slot'\r\n")
  PrintSetup("\r\n")
  PrintSetup("'APN' 'CID' 'username' and 'password' parameters are requested for each SIM slot used\r\n")
  PrintSetup("\r\n")
  PrintSetup("---\n\r")
  PrintSetup("APN\r\n")
  PrintSetup("---\n\r")
  PrintSetup("APN to associated with the CID for the selected SIM slot\r\n")
  PrintSetup("\r\n")
  PrintSetup("---\n\r")
  PrintSetup("CID\r\n")
  PrintSetup("---\n\r")
  PrintSetup("CID to associated with the APN for the selected SIM slot\r\n")
  PrintSetup("The default value is '%s'\r\n", CST_DEFAULT_CID_STRING)
  PrintSetup("\r\n")
  PrintSetup("--------\n\r")
  PrintSetup("username\r\n")
  PrintSetup("--------\n\r")
  PrintSetup("Username used for the selected SIM slot (optionnal)\r\n")
  PrintSetup("\r\n")
  PrintSetup("--------\n\r")
  PrintSetup("password\r\n")
  PrintSetup("--------\n\r")
  PrintSetup("Password used for the selected SIM slot (optionnal)\r\n")
  PrintSetup("\r\n")
  PrintSetup("------------------\n\r")
  PrintSetup("Modem Target state\r\n")
  PrintSetup("------------------\n\r")
  PrintSetup("Modem state to reach at initialisation\r\n")
  PrintSetup("- 0: modem off\r\n")
  PrintSetup("- 2: full cellular data enabled\r\n")
  PrintSetup("The default value is '%s'\r\n", CST_DEFAULT_TARGET_STATE_STRING)
  PrintSetup("\r\n")
  PrintSetup("---------------\n\r")
  PrintSetup("NFMC activation\r\n")
  PrintSetup("---------------\n\r")
  PrintSetup("Used for enabling of disabling the NFMC feature\r\n")
  PrintSetup("- 0: NFMC feature disabled\r\n")
  PrintSetup("- 1: NFMC feature enabled. Base-temporization parameters are used\r\n")
  PrintSetup("The default value of NFMC activation is '%s'\r\n", CST_DEFAULT_NFMC_ACTIVATION_STRING)
  PrintSetup("\r\n")
  PrintSetup("The default values of the base-temporization parameters are:\r\n")
  PrintSetup("NFMC value 1:   %s ms\r\n", CST_DEFAULT_NFMC_TEMPO1_STRING)
  PrintSetup("NFMC value 2:  %s ms\r\n", CST_DEFAULT_NFMC_TEMPO2_STRING)
  PrintSetup("NFMC value 3:  %s ms\r\n", CST_DEFAULT_NFMC_TEMPO3_STRING)
  PrintSetup("NFMC value 4:  %s ms\r\n", CST_DEFAULT_NFMC_TEMPO4_STRING)
  PrintSetup("NFMC value 5:  %s ms\r\n", CST_DEFAULT_NFMC_TEMPO5_STRING)
  PrintSetup("NFMC value 6: %s ms\r\n", CST_DEFAULT_NFMC_TEMPO6_STRING)
  PrintSetup("NFMC value 7: %s ms\r\n", CST_DEFAULT_NFMC_TEMPO7_STRING)
  PrintSetup("\r\n")
}

static uint32_t CST_setup_handler(void)
{
  static uint8_t CST_sim_string[DC_SIM_SLOT_NB + 1];
  static uint8_t CST_temp_string[CST_TEMP_STRING_SIZE];
  int16_t i;
  uint32_t j;
  uint32_t count;
  uint32_t ret;

  ret = 0U;
  int32_t default_sim_slot_nb;
  int32_t sim_slot_nb;
  dc_cs_sim_slot_type_t sim_slot_type;
  dc_cellular_params_t cellular_params;

  (void)memset((void *)&cellular_params, 0, sizeof(cellular_params));
  cellular_params.set_pdn_mode = 1U;
  (void)menu_utils_get_next_default_value(CST_temp_string, 0);
  default_sim_slot_nb = (int32_t)crs_strlen(CST_temp_string);

  menu_utils_get_string((uint8_t *)"Sim Slot List (0: socket / 1: embedded sim) (possible values (0 1 or 01)",
                        (uint8_t *)CST_sim_string, sizeof(CST_sim_string));
  sim_slot_nb = (int32_t)crs_strlen(CST_sim_string);
  if (((CST_sim_string[0]  != (uint8_t)'0') && (CST_sim_string[0] != (uint8_t)'1'))
      || ((CST_sim_string[1] !=  0U)  && (CST_sim_string[1] != (uint8_t)'1'))
      || (sim_slot_nb > 2))
  {
    PrintSetup("Bad 'SIM slot list'. Exit from config\r\n")
    ret = 1U;
    goto end;
  }

  for (i = 0 ; (i < sim_slot_nb) && (ret == 0U); i++)
  {
    sim_slot_type = cst_get_sim_socket_value(CST_sim_string[i] - (uint8_t)'0');
    PrintSetup("Sim slot %d (%s) config:\r\n", i, CST_SimSlotName_p[sim_slot_type])
    cellular_params.sim_slot[i].sim_slot_type = sim_slot_type;


    if (default_sim_slot_nb == 0)
    {
      menu_utils_get_string_without_default_value((uint8_t *)"APN: ", cellular_params.sim_slot[i].apn, DC_MAX_SIZE_APN);
      menu_utils_get_string_without_default_value((uint8_t *)"CID (1-9): ", (uint8_t *)CST_temp_string,
                                                  sizeof(CST_temp_string));
      if ((CST_temp_string[0]  < (uint8_t)'1') || (CST_temp_string[0] > (uint8_t)'9'))
      {
        PrintSetup("Bad 'CID' value. Exit from config\r\n")
        ret = 1U;
      }
      else
      {
        cellular_params.sim_slot[i].cid = cst_get_cid_value(CST_temp_string[0] - (uint8_t)'0');
        menu_utils_get_string_without_default_value((uint8_t *)"username: ", cellular_params.sim_slot[i].username,
                                                    DC_CST_USERNAME_SIZE);
        menu_utils_get_string_without_default_value((uint8_t *)"password: ", cellular_params.sim_slot[i].password,
                                                    DC_CST_PASSWORD_SIZE);
      }
    }
    else
    {
      menu_utils_get_string((uint8_t *)"APN ", cellular_params.sim_slot[i].apn, DC_MAX_SIZE_APN);
      menu_utils_get_string((uint8_t *)"CID (1-9)", (uint8_t *)CST_temp_string, sizeof(CST_temp_string));
      if ((CST_temp_string[0]  < (uint8_t)'1') || (CST_temp_string[0] > (uint8_t)'9'))
      {
        PrintSetup("Bad 'CID' value. Exit from config\r\n")
        ret = 1U;
      }
      else
      {
        cellular_params.sim_slot[i].cid = cst_get_cid_value(CST_temp_string[0] - (uint8_t)'0');
        menu_utils_get_string((uint8_t *)"username ", cellular_params.sim_slot[i].username, DC_CST_USERNAME_SIZE);
        menu_utils_get_string((uint8_t *)"password ", cellular_params.sim_slot[i].password, DC_CST_PASSWORD_SIZE);
        default_sim_slot_nb--;
        cellular_params.sim_slot_nb++;
      }
    }
  }

  if (ret != 0U)
  {
    goto end;
  }

  if (default_sim_slot_nb != 0)
  {
    menu_utils_flush_next_default_value((int16_t)(default_sim_slot_nb * 4));
  }

  menu_utils_get_string((uint8_t *)"cellular target state (0: modem off / 2: full cellular data)",
                        (uint8_t *)CST_temp_string, sizeof(CST_temp_string));
  if ((CST_temp_string[0]  != (uint8_t)'0') && (CST_temp_string[0] != (uint8_t)'2'))
  {
    PrintSetup("Bad 'target state' value. Exit from config\r\n")
    ret = 1U;
    goto end;
  }

  cellular_params.target_state = cst_get_target_state_value(CST_temp_string[0] - (uint8_t)'0');
  if (cellular_params.target_state == DC_TARGET_STATE_UNKNOWN)
  {
    PrintSetup("Unknown target state. Exit from config\r\n")
    ret = 1U;
    goto end;
  }

#if (CST_SETUP_NFMC == 1)
  menu_utils_get_string((uint8_t *)"NFMC activation (0: inactive / 1: active) ", (uint8_t *)CST_temp_string,
                        sizeof(CST_temp_string));
  if (CST_temp_string[0] == (uint8_t)'1')
  {
    cellular_params.nfmc_active = 1U;
    for (j = 0; j < CST_NFMC_TEMPO_NB ; j++)
    {
      (void)sprintf((CRC_CHAR_t *)CST_temp_string, "NFMC value %ld  ", j + 1U);
      menu_utils_get_string(CST_temp_string, (uint8_t *)CST_temp_string, sizeof(CST_temp_string));
      cellular_params.nfmc_value[j] = (uint32_t)crs_atoi(CST_temp_string);
    }
  }
  else if (CST_temp_string[0] == (uint8_t)'0')
  {
    cellular_params.nfmc_active = 0U;
    for (j = 0; j < CST_NFMC_TEMPO_NB ; j++)
    {
      count = menu_utils_get_next_default_value(CST_temp_string, 1);
      if ((count == 0U) || (count > 10U))
      {
        break;
      }
      menu_utils_set_sefault_value((uint8_t *)CST_temp_string);
      cellular_params.nfmc_value[j] = (uint32_t)crs_atoi(CST_temp_string);
    }
  }
  else
  {
    PrintSetup("Unknown 'NFMC activation' mode (0 or 1). Exit from config\r\n")
    ret = 1;
    goto end;
  }


#else  /* (CST_SETUP_NFMC == 1) */
  cellular_params.nmfc_active = 1U;
  cellular_params.nfmc_value[0] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO1_STRING);
  cellular_params.nfmc_value[1] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO2_STRING);
  cellular_params.nfmc_value[2] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO3_STRING);
  cellular_params.nfmc_value[3] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO4_STRING);
  cellular_params.nfmc_value[4] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO5_STRING);
  cellular_params.nfmc_value[5] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO6_STRING);
  cellular_params.nfmc_value[6] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO7_STRING);
#endif /* (CST_SETUP_NFMC == 1) */
  cellular_params.rt_state = DC_SERVICE_ON;
  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cellular_params, sizeof(cellular_params));

end:
  return ret;
}

static void CST_setup_dump(void)
{
  uint32_t i;

  dc_cellular_params_t cellular_params;
  (void)dc_com_read(&dc_com_db,  DC_COM_CELLULAR_PARAM, (void *)&cellular_params, sizeof(cellular_params));

  PrintForce("\n\r")

  for (i = 0 ; i < cellular_params.sim_slot_nb ; i++)
  {
    PrintForce("Sim Slot %ld: %d (%s)\n\r", i, cellular_params.sim_slot[i].sim_slot_type,
               CST_SimSlotName_p[cellular_params.sim_slot[i].sim_slot_type]);
    PrintForce("APN: %s\n\r",  cellular_params.sim_slot[i].apn)
    PrintForce("CID: %d\n\r",  cellular_params.sim_slot[i].cid)
    PrintForce("username: %s\n\r",  cellular_params.sim_slot[i].username)
    PrintForce("password: %s\n\r",  cellular_params.sim_slot[i].password)
  }
  PrintForce("modem target_state: %d\n\r",  cellular_params.target_state)

#if (CST_SETUP_NFMC == 1)
  PrintForce("NFMC activation : %d\n\r",  cellular_params.nfmc_active)

  if (cellular_params.nfmc_active == 1U)
  {
    for (i = 0U; i < CST_NFMC_TEMPO_NB ; i++)
    {
      PrintForce("NFMC value %ld : %ld\n\r", i + 1U, cellular_params.nfmc_value[i])
    }
  }
#endif  /* (CST_SETUP_NFMC == 1) */
}
#else /* (!USE_DEFAULT_SETUP == 1) */

static void CST_local_setup_handler(void)
{
  static uint8_t *CST_default_setup_table[CST_DEFAULT_PARAMA_NB] =
  {
    CST_DEFAULT_SIM_SLOT_STRING,
    CST_DEFAULT_APN_STRING,
    CST_DEFAULT_CID_STRING,
    CST_DEFAULT_USERNAME_STRING,
    CST_DEFAULT_PASSWORD_STRING,
    CST_DEFAULT_TARGET_STATE_STRING,
    CST_DEFAULT_NFMC_ACTIVATION_STRING,
    CST_DEFAULT_NFMC_TEMPO1_STRING,
    CST_DEFAULT_NFMC_TEMPO2_STRING,
    CST_DEFAULT_NFMC_TEMPO3_STRING,
    CST_DEFAULT_NFMC_TEMPO4_STRING,
    CST_DEFAULT_NFMC_TEMPO5_STRING,
    CST_DEFAULT_NFMC_TEMPO6_STRING,
    CST_DEFAULT_NFMC_TEMPO7_STRING
  };
  const uint8_t *tmp_string;
  dc_cellular_params_t cellular_params;

  (void)memset((void *)&cellular_params, 0, sizeof(cellular_params));

  cellular_params.set_pdn_mode = 1U;
  cellular_params.sim_slot_nb  = 1U;

  /* SIM slot 0 parameters BEGIN*/
  tmp_string = CST_default_setup_table[CST_PARAM_SIM_SLOT];
  cellular_params.sim_slot[0].sim_slot_type = cst_get_sim_socket_value(tmp_string[0] - (uint8_t)'0');
  (void)memcpy((CRC_CHAR_t *)cellular_params.sim_slot[0].apn,
               (CRC_CHAR_t *)CST_default_setup_table[CST_PARAM_APN],
               crs_strlen(CST_default_setup_table[CST_PARAM_APN])+1);

  tmp_string = CST_default_setup_table[CST_PARAM_CID];
  cellular_params.sim_slot[0].cid = cst_get_cid_value(tmp_string[0] - (uint8_t)'0');

  (void)memcpy((CRC_CHAR_t *)cellular_params.sim_slot[0].username,
               (CRC_CHAR_t *)CST_default_setup_table[CST_PARAM_USERNAME],
                crs_strlen(CST_default_setup_table[CST_PARAM_USERNAME])+1);
  (void)memcpy((CRC_CHAR_t *)cellular_params.sim_slot[0].password,
               (CRC_CHAR_t *)CST_default_setup_table[CST_PARAM_PASSWORD],
                crs_strlen(CST_default_setup_table[CST_PARAM_PASSWORD])+1);
  /* SIM slot 0 parameters END*/

  tmp_string = CST_default_setup_table[CST_PARAM_TARGET_STATE];
  cellular_params.target_state = cst_get_target_state_value(tmp_string[0] - (uint8_t)'0');

#if (CST_SETUP_NFMC == 1)
  tmp_string = CST_default_setup_table[CST_PARAM_NFMC];
  if (tmp_string[0] == (uint8_t)'1')
  {
    cellular_params.nfmc_active = 1U;
  }
  else
  {
    cellular_params.nfmc_active = 0U;
  }

  for (uint32_t i = 0U; i < CST_NFMC_TEMPO_NB ; i++)
  {
    cellular_params.nfmc_value[i] = (uint32_t)crs_atoi(CST_default_setup_table[CST_PARAM_NFMC_TEMPO]);
  }
#else /* CST_SETUP_NFMC == 1 */
  cellular_params.nfmc_active = 1;
  cellular_params.nfmc_value[0] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO1_STRING);
  cellular_params.nfmc_value[1] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO2_STRING);
  cellular_params.nfmc_value[2] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO3_STRING);
  cellular_params.nfmc_value[3] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO4_STRING);
  cellular_params.nfmc_value[4] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO5_STRING);
  cellular_params.nfmc_value[5] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO6_STRING);
  cellular_params.nfmc_value[6] = (uint32_t)crs_atoi(CST_DEFAULT_NFMC_TEMPO7_STRING);
#endif /* (CST_SETUP_NFMC == 1) */
  cellular_params.rt_state = DC_SERVICE_ON;
  (void)dc_com_write(&dc_com_db, DC_COM_CELLULAR_PARAM, (void *)&cellular_params, sizeof(cellular_params));
}
#endif /* (!USE_DEFAULT_SETUP == 1) */

CS_Status_t CST_config_init(void)
{
#if (!USE_DEFAULT_SETUP == 1)
  static uint8_t *CST_default_setup_table[CST_DEFAULT_PARAMA_NB] =
  {
    CST_DEFAULT_SIM_SLOT_STRING,
    CST_DEFAULT_APN_STRING,
    CST_DEFAULT_CID_STRING,
    CST_DEFAULT_USERNAME_STRING,
    CST_DEFAULT_PASSWORD_STRING,
    CST_DEFAULT_TARGET_STATE_STRING,
    CST_DEFAULT_NFMC_ACTIVATION_STRING,
    CST_DEFAULT_NFMC_TEMPO1_STRING,
    CST_DEFAULT_NFMC_TEMPO2_STRING,
    CST_DEFAULT_NFMC_TEMPO3_STRING,
    CST_DEFAULT_NFMC_TEMPO4_STRING,
    CST_DEFAULT_NFMC_TEMPO5_STRING,
    CST_DEFAULT_NFMC_TEMPO6_STRING,
    CST_DEFAULT_NFMC_TEMPO7_STRING
  };
#endif  /* (!USE_DEFAULT_SETUP == 1) */

#if (!USE_DEFAULT_SETUP == 1)
  (void)app_select_record((uint8_t *)CST_MODEM_POWER_ON_LABEL, APP_SELECT_CST_MODEM_START);
  (void)setup_record(SETUP_APPLI_CST, CST_VERSION_APPLI,
                     (uint8_t *)CST_LABEL, CST_setup_handler,
                     CST_setup_dump,
                     CST_setup_help,
                     CST_default_setup_table, CST_DEFAULT_PARAMA_NB);
#else
  CST_local_setup_handler();
#endif   /* (!USE_DEFAULT_SETUP == 1) */

  return CELLULAR_OK;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

