/**
  ******************************************************************************
  * @file    data_init.c
  * @author  MCD Application Team
  * @brief   Data section (RW + ZI) initialization.
  *          This file provides set of firmware functions to manage SE low level
  *          interface functionalities.
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
/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

extern uint32_t Image$$SE_region_SRAM1$$RW$$Base;
extern uint32_t Load$$SE_region_SRAM1$$RW$$Base;
extern uint32_t Image$$SE_region_SRAM1$$RW$$Length;
extern uint32_t Image$$SE_region_SRAM1$$ZI$$Base;
extern uint32_t Image$$SE_region_SRAM1$$ZI$$Length;


#define data_ram Image$$SE_region_SRAM1$$RW$$Base
#define data_rom Load$$SE_region_SRAM1$$RW$$Base
#define data_rom_length Image$$SE_region_SRAM1$$RW$$Length
#define bss Image$$SE_region_SRAM1$$ZI$$Base
#define bss_length Image$$SE_region_SRAM1$$ZI$$Length;


/**
  * @brief  Copy initialized data from ROM to RAM.
  * @param  None.
  * @retval None.
  */
void LoopCopyDataInit(void) 
{
  uint32_t i;
	uint8_t* src = (uint8_t*)&data_rom;
	uint8_t* dst = (uint8_t*)&data_ram;
	uint32_t len = (uint32_t)&data_rom_length;

	for(i=0; i < len; i++)
  {  
		dst[i] = src[i];
  }
}
  
/**
  * @brief  Clear the zero-initialized data section.
  * @param  None.
  * @retval None.
  */
void LoopFillZerobss(void) 
{
  uint32_t i;
	uint8_t* dst = (uint8_t*)&bss;
	uint32_t len = (uint32_t)&bss_length;
	
	/* Clear the zero-initialized data section */
	for(i=0; i < len; i++)
  {  
		dst[i] = 0;
  }
}
	
/**
  * @brief  Data section initialization.
  * @param  None.
  * @retval None.
  */
 void __arm_data_init(void) {
	 	LoopFillZerobss();
	  LoopCopyDataInit();
} 
 

