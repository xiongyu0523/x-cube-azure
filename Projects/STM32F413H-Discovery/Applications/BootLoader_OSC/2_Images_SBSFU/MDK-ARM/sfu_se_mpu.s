;/******************************************************************************
;* File Name          : sfu_se_mpu.s
;* Author             : MCD Application Team
;* Description        : Wrapper for SE isolation with MPU.
;********************************************************************************
;* @attention
;*
;* <h2><center>&copy; Copyright(c) 2017 STMicroelectronics International N.V.
;* All rights reserved.</center></h2>
;*
;* Redistribution and use in source and binary forms, with or without
;* modification, are permitted, provided that the following conditions are met:
;*
;* 1. Redistribution of source code must retain the above copyright notice,
;*    this list of conditions and the following disclaimer.
;* 2. Redistributions in binary form must reproduce the above copyright notice,
;*    this list of conditions and the following disclaimer in the documentation
;*    and/or other materials provided with the distribution.
;* 3. Neither the name of STMicroelectronics nor the names of other
;*    contributors to this software may be used to endorse or promote products
;*    derived from this software without specific written permission.
;* 4. This software, including modifications and/or derivative works of this
;*    software, must execute solely and exclusively on microcontroller or
;*    microprocessor devices manufactured by or for STMicroelectronics.
;* 5. Redistribution and use of this software other than as permitted under
;*    this license is void and will automatically terminate your rights under
;*    this license.
;*
;* THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
;* AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
;* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
;* PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
;* RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
;* SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
;* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
;* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
;* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
;* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;*
;******************************************************************************
;
;
; Cortex-M version
;
      PRESERVE8
      THUMB
      AREA    |.text|, CODE, READONLY
      EXPORT SVC_Handler
SVC_Handler
        IMPORT MPU_SVC_Handler
        MRS r0, PSP
        B MPU_SVC_Handler

      EXPORT jump_to_function
jump_to_function
        LDR SP, [R0]
        LDR PC, [R0,#4]

      EXPORT launch_application
launch_application
        ; return from exception to application launch function
        ; R0: application vector address
        ; R1: exit function address
        ; push interrupt context R0 R1 R2 R3 R12 LR PC xPSR
        MOV R2, #0x01000000 ; xPSR activate Thumb bit
        PUSH {R2} ; FLAGS=0
        MOV R2, #1
        BIC R1, R1, R2  ; clear least significant bit of exit function
        PUSH {R1}  ; return address = application entry point
        MOV R1, #0 ; clear other context registers
        PUSH {R1} ; LR =0
        PUSH {R1} ; R12 =0
        PUSH {R1} ; R3 = 0
        PUSH {R1} ; R2 = 0
        PUSH {R1} ; R1 = 0
        ; set R0 to application entry point
        PUSH {R0} ; R0 = application entry point
        ; set LR to return to thread mode with main stack
        MOV LR, #0xFFFFFFF9
        ; return from interrupt
        BX LR

        ALIGN
        END
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
