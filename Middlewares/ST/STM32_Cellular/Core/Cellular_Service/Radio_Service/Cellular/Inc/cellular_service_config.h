/**
  ******************************************************************************
  * @file    cellular_service_config.h
  * @author  MCD Application Team
  * @brief   Header for cellular task configuration
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
#ifndef CELLULAR_SERVICE_CONFIG_H
#define CELLULAR_SERVICE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#ifdef USE_MODEM_UG96
#include "plf_modem_config_ug96.h"
#endif
#ifdef USE_MODEM_BG96
#include "plf_modem_config_bg96.h"
#endif

/* Exported constants --------------------------------------------------------*/
#define CST_DEFAULT_SIM_SLOT_STRING         ((uint8_t*)"0")        /* CST_SIM_SLOT_MODEM_SOCKET */
#define CST_DEFAULT_APN_STRING              ((uint8_t*)PDP_CONTEXT_DEFAULT_APN)
#define CST_DEFAULT_CID_STRING              ((uint8_t*)PDP_CONTEXT_DEFAULT_MODEM_CID_STRING)
#define CST_DEFAULT_USERNAME_STRING         ((uint8_t*)"")         /* No Authentication */
#define CST_DEFAULT_PASSWORD_STRING         ((uint8_t*)"")         /* No Authentication */
#define CST_DEFAULT_TARGET_STATE_STRING     ((uint8_t*)"2")        /* full (cellular data */
#define CST_DEFAULT_NFMC_ACTIVATION_STRING  ((uint8_t*)"0")        /* NFMC DISABLED */
#define CST_DEFAULT_NFMC_TEMPO1_STRING      ((uint8_t*)"60000")
#define CST_DEFAULT_NFMC_TEMPO2_STRING      ((uint8_t*)"120000")
#define CST_DEFAULT_NFMC_TEMPO3_STRING      ((uint8_t*)"240000")
#define CST_DEFAULT_NFMC_TEMPO4_STRING      ((uint8_t*)"480000")
#define CST_DEFAULT_NFMC_TEMPO5_STRING      ((uint8_t*)"960000")
#define CST_DEFAULT_NFMC_TEMPO6_STRING      ((uint8_t*)"1920000")
#define CST_DEFAULT_NFMC_TEMPO7_STRING      ((uint8_t*)"3840000")
#define CST_DEFAULT_PARAMA_NB     14U

#define CST_NFMC_TEMPO_NB         7U

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
//CS_Status_t CST_config_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

