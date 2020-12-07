---
pagetitle: Release Notes for 32L496GDISCOVERY Board Drivers
lang: en
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

::: {.card .fluid}
::: {.sectione .dark}
<center>
# <small>Release Notes for</small> <mark>32L496GDISCOVERY Board Drivers</mark>
Copyright &copy; 2017 STMicroelectronics\
    
[![ST logo](../../../_htmresc/st_logo.png)](https://www.st.com){.logo}
</center>
:::
:::

# License

Licensed by ST under BSD 3-Clause license (the \"License\"). You may
not use this package except in compliance with the License. You may
obtain a copy of the License at:

[https://opensource.org/licenses/BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)

# Purpose

This directory contains the board drivers to demonstrate the capabilities of the 32L496GDISCOVERY Kit.

:::

::: {.col-sm-12 .col-lg-8}
# Update History

::: {.collapse}
<input type="checkbox" id="collapse-section19" checked aria-hidden="true">
<label for="collapse-section19" aria-hidden="true">V1.1.2 / 03-April-2019</label>
<div>			

**[[Main
Changes]{style="font-size: 10pt; font-family: Verdana; color: black;"}]{.underline}**

- stm32l496g_discovery.c
  - Correct logical test in DrawChar()
  - Comment minor correction

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section18" aria-hidden="true">
<label for="collapse-section18" aria-hidden="true">V1.1.1 / 27-July-2018</label>
<div>			

## Main Changes

- Release notes update to new format


</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section17" aria-hidden="true">
<label for="collapse-section17" aria-hidden="true">V1.1.0 / 25-August-2017</label>
<div>			

## Main Changes

- stm32l496g_discovery.h/.c
  - Add BSP_COM_Init()/BSP_COM_DeInit() APIs for ST-Link USB Virtual Com Port
- stm32l496g_discovery_audio.h/.c
  - Add INPUT_DEVICE_DIGITAL_MIC1 and INPUT_DEVICE_DIGITAL_MIC2 to provide the capability to record over a single digital microphone (respectively left and right microphones)
- stm32l496g_discovery_sd.c/.h
  - Add weak BSP SD functions
    - BSP_SD_MspInit(), BSP_SD_MspDeInit(), BSP_SD_WriteCpltCallback(), BSP_SD_ReadCpltCallback() and BSP_SD_AbortCallback()

</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section9" aria-hidden="true">
<label for="collapse-section9" aria-hidden="true">V1.0.0 / 17-February-2017</label>
<div>			

## Main Changes

- First official release of **STM32L496G-Discovery** board drivers for STM32Cube L4 FW package

</div>
:::

:::
:::

<footer class="sticky">
For complete documentation on <mark>STM32 Microcontrollers</mark> ,
visit: [http://www.st.com/STM32](http://www.st.com/STM32)
</footer>
