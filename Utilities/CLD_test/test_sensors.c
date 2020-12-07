/**
  ******************************************************************************
  * @file    test_sensors.c
  * @author  MCD Application Team
  * @brief   test sensors of STM32L475 IoT board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
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

#include "main.h"
#include "vl53l0x_def.h"
#include "vl53l0x_api.h"
#include "test_sensors.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define TEST_ITERATIONS 10
#define TEST_PAUSE 1000
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/


int test_sensors()
{
  int ret = 0, res = 0;
  
  printf("\n");
  printf("**************************************************************************\n");
  printf("***                    STM32L475-Discovery IoT node                    ***\n");
  printf("***                           Sensors tests                            ***\n");
  printf("**************************************************************************\n");
  printf("\n");
  printf("Press User button (blue button) to exit from tests.\n\n");
  
  ret = test_hygrometer();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
  
  ret = test_thermometer();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
  
  ret = test_barometer();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
    
  ret = test_magnetometer();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
    
  ret = test_gyrometer();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
    
  ret = test_accelerometer();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
   
  ret = test_tof();
  printf("test returns %d\n", ret);
  if (ret != 0) {
    res = -1;
  }
  
  printf("\nEnd of sensors tests.\n");
  
  return res;
}

/**
  * @brief  test hygrometer
  * @param  none
  * @retval 0 in case of success
  */
int test_hygrometer()
{
  int ret = 0;
  int i = 0;
  float hum_value = 0;
  
  printf("Hygrometer Test Starting.\n");
  
  ret = BSP_HSENSOR_Init();
  printf("BSP_HSENSOR_Init() returns %d\n", ret);

  for (i = 0; i < TEST_ITERATIONS; i++)
  {
    hum_value = BSP_HSENSOR_ReadHumidity();
    printf("%f\n", hum_value);
    if (Button_WaitForPush(TEST_PAUSE)) break;
  }

  printf("End of hygrometer test.\n");
  return 0;
}

/**
  * @brief  test thermometer
  * @param  none
  * @retval 0 in case of success
  */
int test_thermometer()
{
  int ret = 0;
  int i = 0;
  float temp_value = 0;
  
  printf("\nThermometer Test Starting.\n");
  
  ret = BSP_TSENSOR_Init();
  printf("BSP_TSENSOR_Init() returns %d\n", ret);
  
  for (i = 0; i < TEST_ITERATIONS ; i++)
  {
    temp_value = BSP_TSENSOR_ReadTemp();
    printf("%f\n", temp_value);
    if (Button_WaitForPush(TEST_PAUSE)) break;
  }


  /*
  ret = BSP_TSENSOR_DeInit(&handle); 
  printf("BSP_TSENSOR_DeInit() returns %d\n", ret);
  */

  printf("End of Thermometer test.\n");
  return 0;
}

/**
  * @brief  test barometer
  * @param  none
  * @retval 0 in case of success
  */
int test_barometer()
{
  int ret = 0;
  int i = 0;
  float baro_value = 0;
  
  printf("\nBarometer Test Starting.\n");
  
  ret = BSP_PSENSOR_Init();
  printf("BSP_PSENSOR_Init() returns %d\n", ret);
  
  for (i = 0; i < TEST_ITERATIONS ; i++)
  {
    baro_value = BSP_PSENSOR_ReadPressure();
    printf("%f\n", baro_value);
    if (Button_WaitForPush(TEST_PAUSE)) break;
  }
  
  /*
  BSP_PSENSOR_DeInit();
  */
  printf("End of Barometer test.\n");
  return 0;
}


/**
  * @brief  test magnetometer
  * @param  none
  * @retval 0 in case of success
  */
int test_magnetometer()
{
  int ret = 0;
  int i = 0;
  int16_t axes_values[3];
  
  printf("\nMagnetometer Test Starting.\n");
  
  ret = BSP_MAGNETO_Init();
  printf("BSP_MAGNETO_Init() returns %d\n", ret);

  for (i = 0; i < TEST_ITERATIONS; i++)
  {
    BSP_MAGNETO_GetXYZ(axes_values);
    printf("%d %d %d\n", axes_values[0], axes_values[1], axes_values[2]);
    if (Button_WaitForPush(TEST_PAUSE)) break;
  }
  
  BSP_MAGNETO_DeInit();
  
  printf("End of Magnetometer test.\n");
  return 0;
}

/**
  * @brief  test gyrometer
  * @param  none
  * @retval 0 in case of success
  */
int test_gyrometer()
{
  int ret = 0;
  int i = 0;
  float axes_values[3];
  
  printf("\nGyrometer Test Starting.\n");
  
  ret = BSP_GYRO_Init();
  printf("BSP_GYRO_Initialize() returns %d\n", ret);
  
  for (i = 0; i < TEST_ITERATIONS; i++)
  {
    BSP_GYRO_GetXYZ(axes_values);
    printf("%f %f %f\n", axes_values[0], axes_values[1], axes_values[2]);
    if (Button_WaitForPush(TEST_PAUSE)) break;
  }
  
  BSP_GYRO_DeInit();

  printf("End of Gyrometer test.\n");
  return 0;
}

/**
  * @brief  test accelerometer
  * @param  none
  * @retval 0 in case of success
  */
int test_accelerometer()
{
  int ret = 0;
  int i = 0;
  int16_t axes_values[3];
  
  printf("\nAccelerometer Test Starting.\n");
  
  ret = BSP_ACCELERO_Init();
  printf("BSP_ACCELERO_Init() returns %d\n", ret);
  
  for (i = 0; i < TEST_ITERATIONS; i++)
  {
    BSP_ACCELERO_AccGetXYZ(axes_values);
    printf("%d %d %d\n", axes_values[0], axes_values[1], axes_values[2]);
    if (Button_WaitForPush(TEST_PAUSE)) break;
  }
  
  BSP_ACCELERO_DeInit();
  printf("End of Accelerometer test.\n");
  return 0;
}

/**
  * @brief  test Time Of Flight
  * @param  none
  * @retval 0 in case of success
  */
// FIXME , patch to use same I2C handler than other sensor
extern I2C_HandleTypeDef  hI2cHandler ;


int test_tof()
{
  VL53L0X_DeviceInfo_t VL53L0X_DeviceInfo;
  VL53L0X_Dev_t Dev =
  {
    .I2cHandle = &hI2cHandler,
    .I2cDevAddr = 0x52
  };
  VL53L0X_RangingMeasurementData_t RangingMeasurementData;
  int i = 0;
  
  printf("\nTime Of Flight Test Starting.\n");

  memset(&VL53L0X_DeviceInfo, 0, sizeof(VL53L0X_DeviceInfo_t));
  
  HAL_GPIO_WritePin(VL53L0X_XSHUT_GPIO_Port, VL53L0X_XSHUT_Pin, GPIO_PIN_SET);
  
  HAL_Delay(1000);
  
  if (VL53L0X_ERROR_NONE == VL53L0X_GetDeviceInfo(&Dev, &VL53L0X_DeviceInfo))
  {
    uint16_t Id = 0;
    
    if (VL53L0X_ERROR_NONE == VL53L0X_RdWord(&Dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, (uint16_t *) &Id))
    {
      if (Id == 0xEEAA)
      {
        if (VL53L0X_ERROR_NONE == VL53L0X_DataInit(&Dev))
        {
          Dev.Present = 1;
        }
      }
    }
  }
  
  if (Dev.Present)
  {
    SetupSingleShot(Dev);
    
    for (i = 0; i < TEST_ITERATIONS; i++)
    {
      VL53L0X_PerformSingleRangingMeasurement(&Dev, &RangingMeasurementData);
      printf("%d\n", RangingMeasurementData.RangeMilliMeter);
      if (Button_WaitForPush(TEST_PAUSE)) break;
    }
  }
  else
  {
    printf("VL53L0X Time of Flight Failed to Initialize!\n");
  }
  
  printf("End of Time of Flight test.\n");
  return 0;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
