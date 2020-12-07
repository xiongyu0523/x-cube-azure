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
#include "stdint-gcc.h"

#ifndef vu32
#	define vu32 volatile uint32_t
#endif

/**
  * @brief  Copy initialized data from ROM to RAM.
  * @param  None.
  * @retval None.
  */

void LoopCopyDataInit(void)
{
	extern uint8_t _sidata asm("_sidata");
	extern uint8_t _sdata asm("_sdata");
	extern uint8_t _edata asm("_edata");
	
	vu32* src = (vu32*) &_sidata;
	vu32* dst = (vu32*) &_sdata;
	
	vu32 len = (&_edata - &_sdata) / 4;
	
	for(vu32 i=0; i < len; i++)
		dst[i] = src[i];
}

/**
  * @brief  Clear the zero-initialized data section.
  * @param  None.
  * @retval None.
  */
void LoopFillZerobss(void) 
{
	extern uint8_t _sbss asm("_sbss");
	extern uint8_t _ebss asm("_ebss");
	
	vu32* dst = (vu32*) &_sbss;
	vu32 len = (&_ebss - &_sbss) / 4;
	
	for(vu32 i=0; i < len; i++)
		dst[i] = 0;
}

/**
  * @brief  Data section initialization.
  * @param  None.
  * @retval None.
  */
void __gcc_data_init(void) {
	LoopFillZerobss();
	LoopCopyDataInit();
}
