---
pagetitle: Release Notes for STM32F413H-Discovery BSP Drivers
lang: en
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

::: {.card .fluid}
::: {.sectione .dark}
<center>
# **Release Notes for STM32F413H-Discovery BSP Drivers**
Copyright &copy; \<2017\> STMicroelectronics\
    
[![ST logo](../../../_htmresc/st_logo.png)](https://www.st.com){.logo}
</center>
:::
:::

# __License__

This software component is licensed by ST under BSD 3-Clause license, the "License"; You may not use this component except in 
compliance with the License. You may obtain a copy of the License at:
<center>
[https://opensource.org/licenses/BSD-3-Clause](http://www.st.com/software_license_agreement_liberty_v2)
</center>

# __Purpose__

The BSP (Board Specific Package) drivers are parts of the STM32Cube package based on the HAL drivers and provide a set of high level APIs relative to the hardware components and features in the evaluation boards, discovery kits and nucleo boards coming with the STM32Cube package for a given STM32 serie.


The BSP drivers allow a quick access to the boards’ services using high level APIs and without any specific configuration as the link with the HAL and the external components is done in intrinsic within the drivers. 


From project settings points of view, user has only to add the necessary driver’s files in the workspace and call the needed functions from examples. However some low level configuration functions are weak and can be overridden by the applications if user wants to change some BSP drivers default behavior.
:::

::: {.col-sm-12 .col-lg-8}

# __Update History__

::: {.collapse}
<input type="checkbox" id="collapse-section3" checked aria-hidden="true">
<label for="collapse-section3" aria-hidden="true">__V2.0.0 / 22-Mars-2019__</label>
<div>			

## Main Changes

-	Drivers aligned with BSP V2.x BSP specification (UM2298) 

	-	Add stm32f413h_discovery_bus.c/.h drivers
	-	Add bus stm32f413h_discovery_errno.h file
	-	Add bus stm32f413h_discovery_conf_template.h file

## Backward Compatibility

This version breaks the compatibility with previous versions

## Dependencies

This software release is compatible with:

-   BSP components drivers aligned with V2.x BSP specification
-	BSP Common v6.0.0 or above

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section2" checked aria-hidden="true">
<label for="collapse-section2" aria-hidden="true">__V1.0.1 / 25-September-2017__</label>
<div>			

## Main Changes

-	Add general description of BSP drivers
-	Remove date & version
-	stm32f413h_discovery_lcd.c:
	-	Update  BSP_LCD_DrawBitmap() API to fix functional misbehaviour with SW4STM32 Toolchain

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section1" checked aria-hidden="true">
<label for="collapse-section1" aria-hidden="true">__V1.0.0 / 27-January-2017__</label>
<div>			

## Main Changes

-	First official release of the **STM32F413H-Discovery** BSP drivers

</div>
:::

:::
:::

<footer class="sticky">
For complete documentation on <mark>STM32F413H-Discovery</mark> ,
visit: [[www.st.com](http://www.st.com/STM32)]{style="font-color: blue;"}
</footer>
