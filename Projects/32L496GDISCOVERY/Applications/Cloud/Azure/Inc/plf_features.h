/**
  ******************************************************************************
  * @file    plf_features.h
  * @author  MCD Application Team
  * @brief   Includes feature list to include in firmware
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
#ifndef PLF_FEATURES_H
#define PLF_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* ================================================= */
/*          USER MODE                                */
/* ================================================= */

/* ===================================== */
/* BEGIN - Cellular data mode            */
/* ===================================== */
/* Possible values for USE_SOCKETS_TYPE */
#define USE_SOCKETS_LWIP   (0)  /* define value affected to LwIP sockets type */
#define USE_SOCKETS_MODEM  (1)  /* define value affected to Modem sockets type */
/* Sockets location */

#if !defined USE_SOCKETS_TYPE
#define USE_SOCKETS_TYPE   (USE_SOCKETS_MODEM)
#endif  /* !defined USE_SOCKETS_TYPE */

/* If activated then com_ping interfaces in com_sockets module are defined
   mandatory when USE_PING_CLIENT is defined */
#define USE_COM_PING       (1)  /* 0: not activated, 1: activated */


/* ===================================== */
/* END - Cellular data mode              */
/* ===================================== */

/* ===================================== */
/* BEGIN - Applications to include       */
/* ===================================== */
#define USE_ECHO_CLIENT    (0) /* 0: not activated, 1: activated */
#define USE_HTTP_CLIENT    (0) /* 0: not activated, 1: activated */
#define USE_PING_CLIENT    (0) /* 0: not activated, 1: activated */

/* USE_DC_EMUL enables sensor emulation (batery level, pedometer,...)
Note: MEMS are emulated by USE_SIMU_MEMS */
#define USE_DC_EMUL        (0) /* 0: not activated, 1: activated */

/* USE_DC_TEST activates data cache test */
#define USE_DC_TEST        (0) /* 0: not activated, 1: activated */

/* MEMS setup */
/* USE_DC_MEMS enables MEMS management */
#define USE_DC_MEMS        (0) /* 0: not activated, 1: activated */

/* USE_SIMU_MEMS enables MEMS simulation management */
#define USE_SIMU_MEMS      (0) /* 0: not activated, 1: activated */

/* if USE_DC_MEMS and USE_SIMU_MEMS are both defined, the behaviour of availability of MEMS board:
 if  MEMS board is connected, true values are returned
 if  MEMS board is not connected, simulated values are returned
 Note: USE_DC_MEMS and USE_SIMU_MEMS are independent
*/

/* use generic datacache entries */
#define USE_DC_GENERIC      (0) /* 0: not activated, 1: activated */

#if (( USE_PING_CLIENT == 1 ) && ( USE_COM_PING == 0 ))
#error USE_COM_PING must be set to 1 when Ping Client is activated.
#endif /* ( USE_PING_CLIENT == 1 ) && ( USE_COM_PING == 0 ) */

/* ===================================== */
/* END   - Applications to include       */
/* ===================================== */

/* ======================================= */
/* BEGIN -  Miscellaneous functionalities  */
/* ======================================= */
/* To configure some parameters of the software */
#define USE_CMD_CONSOLE       (0) /* 0: not activated, 1: activated */
#define USE_CELPERF           (0) /* 0: not activated, 1: activated */
#define USE_DEFAULT_SETUP     (0) /* 0: Use setup menu,
                                     1: Use default parameters, no setup menu */
/* use UART Communication between two boards */
#define USE_LINK_UART         (0) /* 0: not activated, 1: activated */

/* ======================================= */
/* END   -  Miscellaneous functionalities  */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_FEATURES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
