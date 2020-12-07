#ifdef USE_MODEM_BG96
/**
  ******************************************************************************
  * @file    plf_modem_config.h
  * @author  MCD Application Team
  * @brief   This file contains the modem configuration for BG96
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
#ifndef PLF_MODEM_CONFIG_H
#define PLF_MODEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/* You can specify in project configuration the modem and hardware interface used.
*  If this is not specified, default configuration is specifed below.
*/
#if defined(HWREF_B_CELL_BG96_V2)
/* already explicitly defined:
 * using HWREF_B_CELL_BG96_V2 directly on STMOD+ connector
 */
#else
/* set default config */
#define HWREF_B_CELL_BG96_V2
#endif /* HWREF_B_CELL_BG96_V2 */ 

/* MODEM parameters */
//#define USE_MODEM_BG96
#define CONFIG_MODEM_UART_BAUDRATE (115200U)
#define CONFIG_MODEM_USE_STMOD_CONNECTOR

#define UDP_SERVICE_SUPPORTED                (1U)
#define CONFIG_MODEM_UDP_SERVICE_CONNECT_IP  ((uint8_t *)"127.0.0.1")
#define CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE ((uint32_t)1460U)
#define CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE ((uint32_t)1500U)

/* UART flow control settings */
#if defined(USER_FLAG_MODEM_FORCE_NO_FLOW_CTRL)
#define CONFIG_MODEM_UART_RTS_CTS  (0)
#elif defined(USER_FLAG_MODEM_FORCE_HW_FLOW_CTRL)
#define CONFIG_MODEM_UART_RTS_CTS  (1)
#else /* default FLOW CONTROL setting for BG96 */
#define CONFIG_MODEM_UART_RTS_CTS  (1)
#endif /* user flag for modem flow control */

/* PDP context parameters (for AT+CGDONT) */
#define PDP_CONTEXT_DEFAULT_MODEM_CID         ((uint8_t) 1U)   /* CID numeric value */
#define PDP_CONTEXT_DEFAULT_MODEM_CID_STRING  "1"  /* CID string value */
#define PDP_CONTEXT_DEFAULT_TYPE              "IP" /* defined in project config files */
#define PDP_CONTEXT_DEFAULT_APN               ""   /* defined in project config files */

/* Power saving mode settings (PSM)
*  should we send AT+CPSMS write command ?
*/
#define BG96_SEND_READ_CPSMS        (0)
#define BG96_SEND_WRITE_CPSMS       (0)
#if (BG96_SEND_WRITE_CPSMS == 1)
#define BG96_ENABLE_PSM             (1) /* 1 if enabled, 0 if disabled */
#define BG96_CPSMS_REQ_PERIODIC_TAU ("00000100")  /* refer to AT commands reference manual v2.0 to set timer values */
#define BG96_CPSMS_REQ_ACTIVE_TIME  ("00001111")  /* refer to AT commands reference manual v2.0 to set timer values */
#else
#define BG96_ENABLE_PSM             (0)
#endif /* BG96_SEND_WRITE_CPSMS */

/* e-I-DRX setting (extended idle mode DRX)
* should we send AT+CEDRXS write command ?
*/
#define BG96_SEND_READ_CEDRXS       (0)
#define BG96_SEND_WRITE_CEDRXS      (0)
#if (BG96_SEND_WRITE_CEDRXS == 1)
#define BG96_ENABLE_E_I_DRX         (1) /* 1 if enabled, 0 if disabled */
#define BG96_CEDRXS_ACT_TYPE        (5) /* 1 for Cat.M1, 5 for Cat.NB1 */
#else
#define BG96_ENABLE_E_I_DRX         (0)
#endif /* BG96_SEND_WRITE_CEDRXS */

/* Network Information */
#define BG96_OPTION_NETWORK_INFO    (1)  /* 1 if enabled, 0 if disabled */

/* Engineering Mode */
#define BG96_OPTION_ENGINEERING_MODE    (0)  /* 1 if enabled, 0 if disabled */

#ifdef __cplusplus
}
#endif

#endif /*_PLF_MODEM_CONFIG_H */

#endif /* USE_MODEM_BG96 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
