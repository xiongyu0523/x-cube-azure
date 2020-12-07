#ifdef USE_MODEM_BG96
/**
  ******************************************************************************
  * @file    sysctrl_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code for System control of
  *          BG96 Quectel modem
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
#include "sysctrl.h"
#include "sysctrl_specific_bg96.h"
#include "ipc_common.h"
#include "plf_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_SYSCTRL == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P0, "SysCtrl_BG96:" format "\n\r", ## args)
#define PrintDBG(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P1, "SysCtrl_BG96:" format "\n\r", ## args)
#define PrintAPI(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_P2, "SysCtrl_BG96 API:" format "\n\r", ## args)
#define PrintErr(format, args...)  TracePrint(DBG_CHAN_ATCMD, DBL_LVL_ERR, "SysCtrl_BG96 ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PrintINFO(format, args...)  printf("SysCtrl_BG96:" format "\n\r", ## args);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   printf("SysCtrl_BG96 ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintAPI(format, args...)   do {} while(0);
#define PrintErr(format, args...)   do {} while(0);
#endif /* USE_TRACE_SYSCTRL */

/* Private defines -----------------------------------------------------------*/
#define BG96_BOOT_TIME (5500U) /* Time in ms allowed to complete the modem boot procedure
                                *  according to spec, time = 13 sec
                                *  but pratically, it seems that about 5 sec is acceptable
                                */

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static sysctrl_status_t SysCtrl_BG96_setup(void);

/* Functions Definition ------------------------------------------------------*/
sysctrl_status_t SysCtrl_BG96_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  if (p_devices_list == NULL)
  {
    return (SCSTATUS_ERROR);
  }

  /* check type */
  if (type == DEVTYPE_MODEM_CELLULAR)
  {
#if defined(USE_MODEM_BG96)
    p_devices_list->type          = DEVTYPE_MODEM_CELLULAR;
    p_devices_list->ipc_device    = USER_DEFINED_IPC_DEVICE_MODEM;
    p_devices_list->ipc_interface = IPC_INTERFACE_UART;

    (void) IPC_init(p_devices_list->ipc_device, p_devices_list->ipc_interface, &MODEM_UART_HANDLE);
    retval = SCSTATUS_OK;
#endif /* USE_MODEM_BG96 */
  }

  if (retval != SCSTATUS_ERROR)
  {
    retval = SysCtrl_BG96_setup();
  }

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_power_on(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96&EG95&UG96&M95 R2.0_Compatible_Design_V1.0
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Turn ON module sequence (cf paragraph 4.2)
  *
  *          PWRKEY  RESET_N  modem_state
  * init       0       0        OFF
  * T=0        1       1        OFF
  * T1=100     0       1        BOOTING
  * T1+100     1       1        BOOTING
  * T1+13000   1       1        RUNNING
  */

  /* Re-enale the UART IRQn */
  HAL_NVIC_EnableIRQ(MODEM_UART_IRQn);

  /* POWER ON sequence
  *    RST init state = 1
  *    PWR_EN init state = 1
  *    DTR init state = 0
  */
  /* reset RST and PWR_EN to 0 */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MODEM_RST_GPIO_Port, MODEM_RST_Pin, GPIO_PIN_RESET);
  /* wait at least 30ms (100ms) then PWR_EN = 1 */
  SysCtrl_delay(100U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_SET);
  /* wait at least 100ms (200ms) then PWR_EN = 0 */
  SysCtrl_delay(200U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_RESET);

  /* wait for Modem to complete its booting procedure */
  printf("Waiting for BG96 modem running\n");
  SysCtrl_delay(BG96_BOOT_TIME);
  PrintINFO("...done")

  /* set DTR to 1 */
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_Port, MODEM_DTR_Pin, GPIO_PIN_SET);

  /* POWER ON sequence
  *    RST final state = 0
  *    PWR_EN final state = 0
  *    DTR final state = 1
  */

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_power_off(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96&EG95&UG96&M95 R2.0_Compatible_Design_V1.0
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Turn OFF module sequence (cf paragraph 4.3)
  *
  * Need to use AT+QPOWD command
  * reset GPIO pins to initial state only after completion of previous command (between 2s and 40s)
  */

  /* POWER OFF sequence
  *    RST initial state = 0
  *    PWR_EN initial state = 0
  *    DTR initial state = 1
  */

  /* set PWR_EN and wait at least 650ms (800ms) then PWR_EN = 0 */
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_SET);
  SysCtrl_delay(800U);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_RESET);
  /* wait at least 2000ms (2500ms) */
  SysCtrl_delay(2500U);
  /* reset pin state to initial state */
  HAL_GPIO_WritePin(MODEM_RST_GPIO_Port, MODEM_RST_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_Port, MODEM_DTR_Pin, GPIO_PIN_RESET);

  /* POWER OFF sequence
  *    RST final state = 1
  *    PWR_EN final state = 1
  *    DTR final state = 0
  */

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_reset(sysctrl_device_type_t type)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  /* Reference: Quectel BG96&EG95&UG96&M95 R2.0_Compatible_Design_V1.0
  *  PWRKEY   connected to MODEM_PWR_EN (inverse pulse)
  *  RESET_N  connected to MODEM_RST    (inverse)
  *
  * Reset module sequence (cf paragraph 4.4)
  *
  * Can be done using RESET_N pin to low voltage for 100ms minimum
  *
  *          RESET_N  modem_state
  * init       1        RUNNING
  * T=0        0        OFF
  * T=150      1        BOOTING
  * T>=460     1        RUNNING
  */
  PrintINFO("!!! Hardware Reset triggered !!!")

  /* set RST to 1 for a time between 150ms and 460ms (200) */
  HAL_GPIO_WritePin(MODEM_RST_GPIO_Port, MODEM_RST_Pin, GPIO_PIN_SET);
  SysCtrl_delay(200U);
  HAL_GPIO_WritePin(MODEM_RST_GPIO_Port, MODEM_RST_Pin, GPIO_PIN_RESET);

  /* wait for Modem to complete its restart procedure */
  PrintINFO("Waiting %d millisec for modem running...", BG96_BOOT_TIME)
  SysCtrl_delay(BG96_BOOT_TIME);
  PrintINFO("...done")

  return (retval);
}

sysctrl_status_t SysCtrl_BG96_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot)
{
  UNUSED(type);
  sysctrl_status_t retval = SCSTATUS_OK;

  switch (sim_slot)
  {
    case SC_MODEM_SIM_SOCKET_0:
      HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MDM_SIM_SELECT_1_GPIO_Port, MDM_SIM_SELECT_1_Pin, GPIO_PIN_RESET);
      PrintINFO("MODEM SIM SOCKET SELECTED")
      break;

    case SC_MODEM_SIM_ESIM_1:
      HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(MDM_SIM_SELECT_1_GPIO_Port, MDM_SIM_SELECT_1_Pin, GPIO_PIN_RESET);
      PrintINFO("MODEM SIM ESIM SELECTED")
      break;

    case SC_STM32_SIM_2:
      HAL_GPIO_WritePin(MDM_SIM_SELECT_0_GPIO_Port, MDM_SIM_SELECT_0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(MDM_SIM_SELECT_1_GPIO_Port, MDM_SIM_SELECT_1_Pin, GPIO_PIN_SET);
      PrintINFO("STM32 SIM SELECTED")
      break;

    default:
      PrintErr("Invalid SIM %d selected", sim_slot)
      retval = SCSTATUS_ERROR;
      break;
  }

  return (retval);
}

/* Private function Definition -----------------------------------------------*/
static sysctrl_status_t SysCtrl_BG96_setup(void)
{
  sysctrl_status_t retval = SCSTATUS_OK;

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO config
   * Initial pins state:
   *  PWR_EN initial state = 1
   *  RST initial state = 1
   *  DTR initial state = 0
   */
  HAL_GPIO_WritePin(MODEM_RST_GPIO_Port, MODEM_RST_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MODEM_PWR_EN_GPIO_Port, MODEM_PWR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MODEM_DTR_GPIO_Port, MODEM_DTR_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = MODEM_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MODEM_RST_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODEM_DTR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MODEM_DTR_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MODEM_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MODEM_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /* UART configuration */
  MODEM_UART_HANDLE.Instance = MODEM_UART_INSTANCE;
  MODEM_UART_HANDLE.Init.BaudRate = MODEM_UART_BAUDRATE;
  MODEM_UART_HANDLE.Init.WordLength = MODEM_UART_WORDLENGTH;
  MODEM_UART_HANDLE.Init.StopBits = MODEM_UART_STOPBITS;
  MODEM_UART_HANDLE.Init.Parity = MODEM_UART_PARITY;
  MODEM_UART_HANDLE.Init.Mode = MODEM_UART_MODE;
  MODEM_UART_HANDLE.Init.HwFlowCtl = MODEM_UART_HWFLOWCTRL;
  MODEM_UART_HANDLE.Init.OverSampling = UART_OVERSAMPLING_16;
  MODEM_UART_HANDLE.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;

  /* do not activate autobaud (not compatible with current implementation) */
   MODEM_UART_HANDLE.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  /* UART initialization */
  if (HAL_UART_Init(&MODEM_UART_HANDLE) != HAL_OK)
  {
    PrintErr("HAL_UART_Init error")
    retval = SCSTATUS_ERROR;
  }

  PrintINFO("BG96 UART config: BaudRate=%d / HW flow ctrl=%d", MODEM_UART_BAUDRATE,
            ((MODEM_UART_HWFLOWCTRL == UART_HWCONTROL_NONE) ? 0 : 1))

  return (retval);
}
#endif /* USE_MODEM_BG96 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

