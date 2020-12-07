/**
  ******************************************************************************
  * @file    cellular_init.c
  * @author  MCD Application Team
  * @brief   Initialisation of cellular components
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
#include "cellular_init.h"

#include "cmsis_os_misrac2012.h"

#include "plf_config.h"

#include "ipc_uart.h"
#include "com_sockets.h"
#include "cellular_service.h"
#include "ppposif.h"
#include "cellular_service_task.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "ppposif_client.h"
#endif  /*  (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
#include "nifman.h"
#include "dc_common.h"
#include "radio_mngt.h"

#if (USE_TRACE_TEST == 1U)
#include "trace_interface.h"
#endif  /* (USE_TRACE_TEST == 1U) */

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void dc_cellular_init(void);

/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t    DC_COM_CELLULAR       = -1;
dc_com_res_id_t    DC_COM_CELLULAR_DATA  = -1;
dc_com_res_id_t    DC_COM_NIFMAN         = -1;
dc_com_res_id_t    DC_COM_NFMC_TEMPO     = -1;
dc_com_res_id_t    DC_COM_SIM_INFO       = -1;
dc_com_res_id_t    DC_COM_CELLULAR_PARAM = -1;
dc_com_res_id_t    DC_CELLULAR_TARGET_STATE_CMD = -1;
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
dc_com_res_id_t    DC_COM_PPP_CLIENT     = -1;
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
/* Functions Definition ------------------------------------------------------*/

static void dc_cellular_init(void)
{
  static dc_cellular_info_t          dc_cellular_info;
  static dc_ppp_client_info_t        dc_ppp_client_info;
  static dc_cellular_data_info_t     dc_cellular_data_info;
  static dc_nifman_info_t            dc_nifman_info;
  static dc_nfmc_info_t              dc_nfmc_info;
  static dc_sim_info_t               dc_sim_info;
  static dc_cellular_params_t        dc_cellular_params;
  static dc_cellular_target_state_t  dc_cellular_target_state;

  (void)memset((void *)&dc_cellular_info,         0, sizeof(dc_cellular_info_t));
  (void)memset((void *)&dc_ppp_client_info,       0, sizeof(dc_ppp_client_info_t));
  (void)memset((void *)&dc_cellular_data_info,    0, sizeof(dc_cellular_data_info_t));
  (void)memset((void *)&dc_nifman_info,           0, sizeof(dc_nifman_info_t));
  (void)memset((void *)&dc_nfmc_info,             0, sizeof(dc_nfmc_info_t));
  (void)memset((void *)&dc_sim_info,              0, sizeof(dc_sim_info_t));
  (void)memset((void *)&dc_cellular_params,       0, sizeof(dc_cellular_params_t));
  (void)memset((void *)&dc_cellular_target_state, 0, sizeof(dc_cellular_target_state_t));

  DC_COM_CELLULAR       = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_info, (uint16_t)sizeof(dc_cellular_info_t));
  DC_COM_CELLULAR_DATA  = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_data_info, (uint16_t)sizeof(dc_cellular_data_info_t));
  DC_COM_NIFMAN         = dc_com_register_serv(&dc_com_db, (void *)&dc_nifman_info, (uint16_t)sizeof(dc_nifman_info_t));
  DC_COM_NFMC_TEMPO     = dc_com_register_serv(&dc_com_db, (void *)&dc_nfmc_info, (uint16_t)sizeof(dc_nfmc_info_t));
  DC_COM_SIM_INFO       = dc_com_register_serv(&dc_com_db, (void *)&dc_sim_info, (uint16_t)sizeof(dc_sim_info_t));
  DC_COM_CELLULAR_PARAM = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_params, (uint16_t)sizeof(dc_cellular_params_t));
  DC_CELLULAR_TARGET_STATE_CMD = dc_com_register_serv(&dc_com_db, (void *)&dc_cellular_target_state, (uint16_t)sizeof(dc_cellular_target_state_t));
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  DC_COM_PPP_CLIENT     = dc_com_register_serv(&dc_com_db, (void *)&dc_ppp_client_info, (uint16_t)sizeof(dc_ppp_client_info_t));
#endif   /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
}

void cellular_init(void)
{
#if (USE_PRINTF == 0U)
  traceIF_Init();
#endif /* (USE_PRINTF == 0U)  */

  (void)dc_com_init();
  dc_cellular_init();

  (void)com_init();
  (void)CST_cellular_service_init();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  (void)ppposif_client_init();
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */


  (void)nifman_init();
}


void cellular_start(void)
{
  dc_com_start();

  com_start();

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  (void)ppposif_client_start();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

  (void)nifman_start();

  /* dynamical init of components */
#if defined(AT_TEST)
  at_modem_start();
#endif   /* defined(AT_TEST) */

#if !defined(AT_TEST)
  (void)CST_cellular_service_start();
#endif  /* !defined(AT_TEST) */


#if !defined(AT_TEST)
  (void)radio_mngt_radio_on();
#endif   /*  !defined(AT_TEST) */
}

void cellular_modem_start(void)
{
  (void)CST_cellular_service_start();
  (void)CST_modem_power_on();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
