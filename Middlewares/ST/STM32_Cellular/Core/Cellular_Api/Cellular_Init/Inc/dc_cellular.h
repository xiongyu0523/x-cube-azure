/**
  ******************************************************************************
  * @file    dc_cellular.h
  * @author  MCD Application Team
  * @brief   Data Dache definitions for cellular components
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
#ifndef DC_CELLULAR_H
#define DC_CELLULAR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dc_common.h"
#include "cellular_service.h"
#include "cellular_service_int.h"
#include "com_sockets.h"

/* Exported constants --------------------------------------------------------*/
#define DC_MAX_SIZE_MNO_NAME       32U
#define DC_MAX_SIZE_IMEI           32U
#define DC_MAX_SIZE_MANUFACT_NAME  32U
#define DC_MAX_SIZE_MODEL          32U
#define DC_MAX_SIZE_REV            32U
#define DC_MAX_SIZE_SN             32U
#define DC_MAX_SIZE_ICCID          32U
#define DC_MAX_SIZE_APN            32U


#define DC_MAX_IP_ADDR_SIZE        MAX_IP_ADDR_SIZE
#define DC_MAX_SIZE_IMSI           32U
#define DC_NFMC_TEMPO_NB           7U
#define DC_CST_USERNAME_SIZE      20U
#define DC_CST_PASSWORD_SIZE      20U

/* Exported types ------------------------------------------------------------*/

#define DC_NO_ATTACHED 0U

typedef uint8_t dc_cs_signal_level_t;   /*  range 0..99  (0: DC_NO_ATTACHED) */

typedef enum
{
  DC_SIM_SLOT_MODEM_SOCKET           = 0,
  DC_SIM_SLOT_MODEM_EMBEDDED_SIM     = 1,
  DC_SIM_SLOT_STM32_EMBEDDED_SIM     = 2,
  DC_SIM_SLOT_NB                     = 3
} dc_cs_sim_slot_type_t;

typedef enum
{
  DC_TARGET_STATE_OFF           = 0,
  DC_TARGET_STATE_SIM_ONLY      = 1,
  DC_TARGET_STATE_FULL          = 2,
  DC_TARGET_STATE_UNKNOWN       = 3
} dc_cs_target_state_t;

typedef enum
{
  DC_MODEM_STATE_OFF            = 0,     /* modem not started                 */
  DC_MODEM_STATE_POWERED_ON     = 1,     /* modem  */
  DC_MODEM_STATE_SIM_CONNECTED  = 2,     /* modem started with sim connected  */
  DC_MODEM_STATE_DATA_OK        = 3,     /* modem started with data avalaible */
} dc_cs_modem_state_t;

typedef enum
{
  DC_SIM_OK                  = 0,
  DC_SIM_NOT_IMPLEMENTED     = 1,
  DC_SIM_BUSY                = 2,
  DC_SIM_NOT_INSERTED        = 3,
  DC_SIM_PIN_OR_PUK_LOCKED   = 4,
  DC_SIM_INCORRECT_PASSWORD  = 5,
  DC_SIM_ERROR               = 6,
  DC_SIM_NOT_USED            = 7,
  DC_SIM_CONNECTION_ON_GOING  = 8
} dc_cs_sim_status_t;

typedef com_ip_addr_t dc_network_addr_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  dc_cs_modem_state_t  modem_state;

  uint32_t    cs_signal_level;
  int32_t     cs_signal_level_db;

  uint8_t imei[DC_MAX_SIZE_IMEI];
  uint8_t mno_name[DC_MAX_SIZE_MNO_NAME];
  uint8_t manufacturer_name[DC_MAX_SIZE_MANUFACT_NAME];
  uint8_t model[DC_MAX_SIZE_MODEL];
  uint8_t revision[DC_MAX_SIZE_REV];
  uint8_t serial_number[DC_MAX_SIZE_SN];
  uint8_t iccid[DC_MAX_SIZE_ICCID];
} dc_cellular_info_t;


typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  int8_t                 imsi[DC_MAX_SIZE_IMSI];
  dc_cs_sim_slot_type_t  active_slot;
  dc_cs_sim_status_t     sim_status[DC_SIM_SLOT_NB];
} dc_sim_info_t;

typedef struct
{
  dc_cs_sim_slot_type_t sim_slot_type;
  uint8_t               apn[DC_MAX_SIZE_APN];
  CS_PDN_conf_id_t      cid;
  uint8_t               username[DC_CST_USERNAME_SIZE];
  uint8_t               password[DC_CST_PASSWORD_SIZE];
} dc_sim_slot_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
} dc_cellular_data_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  dc_cs_target_state_t   target_state;
} dc_cellular_target_state_t;

typedef enum
{
  DC_NO_NETWORK              = 0,
  DC_CELLULAR_SOCKET_MODEM   = 1,
  DC_CELLULAR_SOCKETS_LWIP   = 2,
  DC_WIFI_NETWORK            = 3
} dc_nifman_network_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  dc_nifman_network_t    network;
  dc_network_addr_t      ip_addr;
} dc_nifman_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  dc_network_addr_t      ip_addr;
  dc_network_addr_t      netmask;
  dc_network_addr_t      gw;
} dc_ppp_client_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t rt_state;
  uint32_t activate;
  uint32_t tempo[DC_NFMC_TEMPO_NB];
} dc_nfmc_info_t;

typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
  uint8_t                set_pdn_mode;
  uint8_t                sim_slot_nb;
  dc_sim_slot_t          sim_slot[DC_SIM_SLOT_NB];
  dc_cs_target_state_t   target_state;
  uint8_t                nfmc_active;
  uint32_t               nfmc_value[DC_NFMC_TEMPO_NB];
} dc_cellular_params_t;

/* External variables --------------------------------------------------------*/
extern dc_com_res_id_t    DC_COM_CELLULAR      ;
extern dc_com_res_id_t    DC_COM_CELLULAR_DATA ;
extern dc_com_res_id_t    DC_COM_NIFMAN        ;
extern dc_com_res_id_t    DC_COM_NFMC_TEMPO    ;
extern dc_com_res_id_t    DC_COM_SIM_INFO      ;
extern dc_com_res_id_t    DC_COM_CELLULAR_PARAM;
extern dc_com_res_id_t    DC_CELLULAR_TARGET_STATE_CMD;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
extern dc_com_res_id_t    DC_COM_PPP_CLIENT    ;
#endif  /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#define DC_COM_CELLULAR_INFO              DC_COM_CELLULAR
#define DC_COM_PPP_CLIENT_INFO            DC_COM_PPP_CLIENT
#define DC_COM_CELLULAR_DATA_INFO         DC_COM_CELLULAR_DATA
#define DC_COM_NIFMAN_INFO                DC_COM_NIFMAN
#define DC_COM_NFMC_TEMPO_INFO            DC_COM_NFMC_TEMPO

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */



#ifdef __cplusplus
}
#endif

#endif /* DC_CELLULAR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
