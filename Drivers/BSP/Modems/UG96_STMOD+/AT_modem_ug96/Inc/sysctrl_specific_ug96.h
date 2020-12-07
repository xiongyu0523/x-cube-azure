#ifdef USE_MODEM_UG96
/**
  ******************************************************************************
  * @file    sysctrl_specific.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl_specific.c module
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
#ifndef SYSCTRL_UG96_H
#define SYSCTRL_UG96_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sysctrl.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
sysctrl_status_t SysCtrl_UG96_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list);
sysctrl_status_t SysCtrl_UG96_power_on(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_UG96_power_off(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_UG96_reset(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_UG96_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot);

#ifdef __cplusplus
}
#endif

#endif /* SYSCTRL_UG96_H */

#endif /* USE_MODEM_UG96 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
