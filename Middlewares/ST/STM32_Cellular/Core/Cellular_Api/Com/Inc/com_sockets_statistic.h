/**
  ******************************************************************************
  * @file    com_sockets_statistic.h
  * @author  MCD Application Team
  * @brief   This file implements Socket statistic
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
#if 0
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COM_SOCKETS_STATISTIC_H
#define COM_SOCKETS_STATISTIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

/* Exported constants --------------------------------------------------------*/

/* Type of sockets statistic to update */
typedef enum
{
  COM_SOCKET_STAT_CRE_OK = 0,
  COM_SOCKET_STAT_CRE_NOK,
  COM_SOCKET_STAT_CNT_OK,
  COM_SOCKET_STAT_CNT_NOK,
  COM_SOCKET_STAT_SND_OK,
  COM_SOCKET_STAT_SND_NOK,
  COM_SOCKET_STAT_RCV_OK,
  COM_SOCKET_STAT_RCV_NOK,
  COM_SOCKET_STAT_CLS_OK,
  COM_SOCKET_STAT_CLS_NOK,
#if (USE_DATACACHE == 1)
  COM_SOCKET_STAT_NWK_UP,
  COM_SOCKET_STAT_NWK_DWN
#endif /* USE_DATACACHE == 1 */
} com_sockets_stat_update_t;

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  None
  * @retval None
  */
void com_sockets_statistic_init(void);

/**
  * @brief  Managed com sockets statistic update and print
  * @note   -
  * @param  stat - to know what the function has to do
  * @note   statistic init, update or print
  * @retval None
  */
void com_sockets_statistic_update(com_sockets_stat_update_t stat);

/**
  * @brief  Display com sockets statistics
  * @note   Request com sockets statistics display
  * @param  None
  * @retval None
  */
void com_sockets_statistic_display(void);

#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_STATISTIC_H */
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
