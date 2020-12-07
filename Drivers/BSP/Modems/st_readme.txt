  @verbatim
  ******************************************************************************                                                             
  * @file    readme.txt
  * @author  MCD Application Team
  * @brief   This file is about X-CUBE-CELLULAR 3.0.0 modems.
  ******************************************************************************
  @endverbatim

3.0.0
=====

+ Release integration date : May 25 th, 2019

+ MODEMS
  - Quectel module BG96 with Qualcomm modem (default setting : USE_MODEM_BG96)
  - Quectel module UG96 with Intel modem (selection by compilation switch : USE_MODEM_UG96)
  For more details on new modem implementation, refer to Application Note that would explain how to support a new modem.

+ FW MODEM VERSIONS
  - Quectel BG96 FW : Factory version is BG96MAR02A06M1G, this FW must be updated.
  Modem Firmware is within the Quectel xG96 SW upgrade zip file (with all instructions inside),
  available at https://stm32-c2c.com (in precompile demos repository)

  - Quectel UG96 FW : Factory version is UG96LNAR02A06E1G aka 02A06 it is the same used for tests.
  Your FW version should be this one so no need to update.
