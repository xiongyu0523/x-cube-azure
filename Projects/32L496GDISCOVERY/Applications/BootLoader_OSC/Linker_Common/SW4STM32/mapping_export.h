/**
  ******************************************************************************
  * @file    mapping_export.h
  * @author  MCD Application Team
  * @brief   This file contains the definitions exported from mapping linker files.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright(c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MAPPING_EXPORT_H
#define MAPPING_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_APP SFU Application Configuration
  * @{
  */

/** @defgroup SFU_APP_Exported_Types Exported Types
  * @{
  */
/** @defgroup SFU_CONFIG_SBSFU_MEMORY_MAPPING SBSFU Memory Mapping
  * @{
  */
#if defined (__ICCARM__) || defined(__GNUC__)
extern uint32_t __ICFEDIT_intvec_start__;
#define INTVECT_START ((uint32_t)& __ICFEDIT_intvec_start__)
extern uint32_t __ICFEDIT_SE_Startup_region_ROM_start__;
#define SE_STARTUP_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_Startup_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_Code_region_ROM_start__;
#define SE_CODE_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_Code_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_Code_region_ROM_end__;
#define SE_CODE_REGION_ROM_END ((uint32_t)& __ICFEDIT_SE_Code_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_IF_region_ROM_start__;
#define SE_IF_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_IF_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_IF_region_ROM_end__;
#define SE_IF_REGION_ROM_END ((uint32_t)& __ICFEDIT_SE_IF_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_Key_region_ROM_start__;
#define SE_KEY_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_Key_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_Key_region_ROM_end__;
#define SE_KEY_REGION_ROM_END ((uint32_t)& __ICFEDIT_SE_Key_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_CallGate_region_ROM_start__; 
#define SE_CALLGATE_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_CallGate_region_ROM_start__)
extern uint32_t __ICFEDIT_SB_region_ROM_start__;
#define SB_REGION_ROM_START ((uint32_t)& __ICFEDIT_SB_region_ROM_start__)
extern uint32_t __ICFEDIT_SB_region_ROM_end__;
#define SB_REGION_ROM_END ((uint32_t)& __ICFEDIT_SB_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_region_SRAM1_start__;
#define SE_REGION_SRAM1_START ((uint32_t)& __ICFEDIT_SE_region_SRAM1_start__)
extern uint32_t __ICFEDIT_SE_region_SRAM1_end__ ;
#define SE_REGION_SRAM1_END ((uint32_t)& __ICFEDIT_SE_region_SRAM1_end__)
extern uint32_t __ICFEDIT_SB_region_SRAM1_start__ ;
#define SB_REGION_SRAM1_START ((uint32_t)& __ICFEDIT_SB_region_SRAM1_start__)
extern uint32_t __ICFEDIT_SB_region_SRAM1_end__ ;
#define SB_REGION_SRAM1_END ((uint32_t)& __ICFEDIT_SB_region_SRAM1_end__)
extern uint32_t __ICFEDIT_SE_region_SRAM1_stack_top__;
#define SE_REGION_SRAM1_STACK_TOP ((uint32_t)& __ICFEDIT_SE_region_SRAM1_stack_top__)
#elif defined(__CC_ARM)
extern uint32_t Image$$vector_start$$Base;
#define  INTVECT_START ((uint32_t)& Image$$vector_start$$Base)
#endif


/**
  * @}
  */
  
/** @defgroup SFU_CONFIG_FW_MEMORY_MAPPING Firmware Slots Memory Mapping
  * @{
  */
#if defined (__ICCARM__) || defined(__GNUC__)
extern uint32_t __ICFEDIT_region_SLOT_0_start__;
#define REGION_SLOT_0_START ((uint32_t)& __ICFEDIT_region_SLOT_0_start__)
extern uint32_t __ICFEDIT_region_SLOT_0_end__;
#define REGION_SLOT_0_END ((uint32_t)& __ICFEDIT_region_SLOT_0_end__)
extern uint32_t __ICFEDIT_region_SLOT_1_start__;
#define REGION_SLOT_1_START ((uint32_t)& __ICFEDIT_region_SLOT_1_start__)
extern uint32_t __ICFEDIT_region_SLOT_1_end__;
#define REGION_SLOT_1_END ((uint32_t)& __ICFEDIT_region_SLOT_1_end__)
extern uint32_t __ICFEDIT_region_SWAP_start__;
#define REGION_SWAP_START ((uint32_t)& __ICFEDIT_region_SWAP_start__)
extern uint32_t __ICFEDIT_region_SWAP_end__;
#define REGION_SWAP_END ((uint32_t)& __ICFEDIT_region_SWAP_end__)
#endif

/**
  * @}
  */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* MAPPING_EXPORT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

