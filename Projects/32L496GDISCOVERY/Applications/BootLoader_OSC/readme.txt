
/**
  @page BootLoader support
  
  @verbatim
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @brief   Boot-loader project.
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. All rights reserved.
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
  @endverbatim

@par Application Description 

BootLoader is based on Secure Boot (SB) and Secure Firmware Update (SFU) solution.
These features are supported by XCUBE-SBSFU package. The below folders are directly
copied from the XCUBE-SBSFU package version 2.1.0.

	-2_Images_SBSFU,
	-2_Images_SECoreBin 


BootLoader is provided a a pre-build executable. Executable name depends on the selected
tools chain:
	Iar		:	./2_Images_SBSFU/EWARM/<BOARDNAME>/Exe/Project.out
	Keil 	: 	./2_Images_SBSFU/MDK-ARM/<BOARDNAME>_2_Images_SBSFU/SBSFU.axf
	SW4STM32:	./2_Images_SBSFU/SW4STM32/<BOARDNAME>_2_Images_SBSFU/Debug/SBSFU.elf

Rebuilding Boot-Loader is not required unless memory mappings or Boot-Loader configuration is
 changed.
	+ Linker files are located in folder: ./Linker_Common
	+ Configuration is located in file : ./2_Images_SBSFU/SBSFU/App/app_sfu.h
	
Current Boot-Loader is defined with following main set up:

	+ SFU_ENCRYPTED_IMAGE
	+ SFU_DEBUG_MODE
	+ SECBOOT_DISABLE_SECURITY_IPS

Please read XCUBS-SBSFU package documentation to get more information about the different setting security

Re-building Boot-Loader 

 
1) Compile projects in the following order (as each component requests the previous one):
  - Applications/BootLoader/2_Images_SECoreBin
  - Applications/BootLoader/2_Images_SBSFU
  
2) Compile AWS application 
	Due to a dependancy issue, touch a source file to ensure build process is run to
	the end and BootLoader modifications are taken into account.
	-Applications/Cloud/AWS


 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
