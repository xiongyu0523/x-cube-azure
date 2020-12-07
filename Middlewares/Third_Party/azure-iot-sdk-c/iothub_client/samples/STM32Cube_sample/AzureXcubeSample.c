/**
  ******************************************************************************
  * @file    AzureXcubeSample.c
  * @author  MCD Application Team
  * @brief   Azure IoT Hub connection example.
  *          Basic telemetry on sensor-equipped boards.
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
#include "iot_flash_config.h"
#include "msg.h"
#include "timingSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* The Azure model declaration defines many symbols which are unused. Mask the compilation warning. */
#ifdef __ICCARM__             /* IAR */
#pragma diag_suppress=Pe177
#elif defined ( __CC_ARM )    /* Keil / armcc */
#pragma diag_suppress 177
#elif defined ( __GNUC__ )    /*GNU Compiler */
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "serializer.h"

/* Register to IoT Hub using the "iothub_client" of the SDK */
#include "iothub_client_ll.h"

#if defined(AZURE_DPS_PROV)
/* Register via IoT Hub Device Provisioning Service using the "provisioning_client" of the SDK */
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"
#endif /* AZURE_DPS_PROV */

#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"   /* For Sleep() */
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "iothubtransportmqtt.h"
#include "jsondecoder.h"
//#include "azure_c_shared_utility/macro_utils.h" /* For enum-string translation */

#if defined(AZURE_DPS_PROV) && defined(AZURE_DPS_PROOF_OF_POSS)
/* X.509 HSM function to handle the proof of possession */
#include "hsm_client_riot.h"
extern char* hsm_client_riot_create_leaf_cert(HSM_CLIENT_HANDLE handle, const char* common_name);
#endif /* AZURE_DPS_PROV && AZURE_DPS_PROOF_OF_POSS */

#if defined(CLD_OTA)
#include "rfu.h"
#endif /* CLD_OTA */

#if defined(AZURE_DPS_PROV)
#define DPS_PROCESS_VALUES \
    DPS_PROCESS_START,     \
    DPS_PROCESS_AUTH,      \
    DPS_PROCESS_ERROR,     \
    DPS_PROCESS_NOT_AUTH

MU_DEFINE_ENUM(DPS_PROCESS, DPS_PROCESS_VALUES);
MU_DEFINE_ENUM_STRINGS(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

/* custom connection string for DPS */
static char* global_prov_uri;
static char* id_scope;
static const char * dps_endpoint_prefix = "DpsEndpoint=";
static const char * id_scope_prefix = "IdScope=";

typedef struct CLIENT_SAMPLE_INFO_TAG
{
  unsigned int SleepTime;
  char* IoTHubUri;
  char* DeviceId;
  DPS_PROCESS RegistrationProcess;
} CLIENT_SAMPLE_INFO;

#endif /* AZURE_DPS_PROV */

#if defined(CLD_OTA)
#define FOTA_INSTALLATION_NOT_REQUESTED ((uint8_t)0)
#define FOTA_INSTALLATION_REQUESTED ((uint8_t)1)
#define FIRMWARE_UPDATE_STATUS_MAX_SIZE 16
/* enum values are in lower case per design */
#define FIRMWARE_UPDATE_STATUS_VALUES \
        Current, \
        Downloading, \
        Applying, \
        Error
MU_DEFINE_ENUM(FIRMWARE_UPDATE_STATUS, FIRMWARE_UPDATE_STATUS_VALUES)
MU_DEFINE_ENUM_STRINGS(FIRMWARE_UPDATE_STATUS, FIRMWARE_UPDATE_STATUS_VALUES)
#endif /* CLD_OTA */

/* Private define ------------------------------------------------------------*/
#define MESSAGE_COUNT                     5
#define DOWORK_LOOP_NUM                   3

#define MODEL_MAC_SIZE                    13
#define MODEL_DEVICEID_SIZE               13
#define MODEL_STATUS_SIZE                 32
#define MODEL_DEFAULT_TELEMETRYINTERVAL   5
#define MODEL_DEFAULT_LEDSTATUSON         true

/* Device Registration & Authentication Methods to connect to IoTHub */
#define DEVICE_AUTH_SYMKEY     (0U)  /* device authentication with a symmetric key      */
#define DEVICE_AUTH_X509       (1U)  /* device authentication with an X.509 certificate */
#define DEVICE_AUTH_DPS        (2U)  /* device registration via DPS procedure first     */
#define DEVICE_AUTH_UNKNOWN    (3U)  /* method not recognized                           */


/* Private typedef -----------------------------------------------------------*/
typedef struct {
  char mac[MODEL_MAC_SIZE];
  char deviceId[MODEL_DEVICEID_SIZE];
} model_strings_t;

typedef struct EVENT_INSTANCE_TAG
{
  IOTHUB_MESSAGE_HANDLE messageHandle;
  size_t messageTrackingId;  /* For tracking the messages within the user callback. */
} EVENT_INSTANCE;

/*
 * By default the sensors variables names are compatible with Azure IOT Central.
 * Define AZURE_USE_STM_DASHBOARD to have sensors variable names
 * for STM32 ODE Dashboard instead.
 */
#ifndef AZURE_USE_STM_DASHBOARD
/* sensors variables names for IOT Central */
#define TEMPERATURE temp
#define HUMIDITY humidity
#define PRESSURE pressure
#define ACCELEROMETERX accelerometerX
#define ACCELEROMETERY accelerometerY
#define ACCELEROMETERZ accelerometerZ
#define GYROSCOPEX gyroscopeX
#define GYROSCOPEY gyroscopeY
#define GYROSCOPEZ gyroscopeZ
#define MAGNETOMETERX magnetometerX
#define MAGNETOMETERY magnetometerY
#define MAGNETOMETERZ magnetometerZ
#else /* AZURE_USE_STM_DASHBOARD */
/* sensors variables names for ST Dashboard */
#define TEMPERATURE Temperature
#define HUMIDITY Humidity
#define PRESSURE Pressure
#define ACCELEROMETERX accX
#define ACCELEROMETERY accY
#define ACCELEROMETERZ accZ
#define GYROSCOPEX gyrX
#define GYROSCOPEY gyrY
#define GYROSCOPEZ gyrZ
#define MAGNETOMETERX magX
#define MAGNETOMETERY magY
#define MAGNETOMETERZ magZ
#endif /* AZURE_USE_STM_DASHBOARD */

BEGIN_NAMESPACE(IotThing);

DECLARE_MODEL(SerializableIotSampleDev_t,
    /* Event data: temperature, humidity... */
    WITH_DATA(ascii_char_ptr, deviceId),
    WITH_DATA(ascii_char_ptr, mac),
    WITH_DATA(float, TEMPERATURE),
    WITH_DATA(float, HUMIDITY),
    WITH_DATA(float, PRESSURE),
    WITH_DATA(int, proximity),
    WITH_DATA(float, ACCELEROMETERX),
    WITH_DATA(float, ACCELEROMETERY),
    WITH_DATA(float, ACCELEROMETERZ),
    WITH_DATA(float, GYROSCOPEX),
    WITH_DATA(float, GYROSCOPEY),
    WITH_DATA(float, GYROSCOPEZ),
    WITH_DATA(float, MAGNETOMETERX),
    WITH_DATA(float, MAGNETOMETERY),
    WITH_DATA(float, MAGNETOMETERZ),
    WITH_DATA(EDM_DATE_TIME_OFFSET, ts),
    WITH_DATA(int, devContext),
    /* Methods */
    WITH_METHOD(Reboot),
    WITH_METHOD(Hello, ascii_char_ptr, msg),
#if defined(CLD_OTA)
    WITH_METHOD(FirmwareUpdate, ascii_char_ptr, FwPackageUri),
#endif /* CLD_OTA */
    /* Commands */
    WITH_ACTION(LedToggle),
    /* Desired Properties */
    WITH_DESIRED_PROPERTY(int, DesiredTelemetryInterval),
#if defined(CLD_OTA)
    WITH_DESIRED_PROPERTY(ascii_char_ptr, fwVersion),
    WITH_DESIRED_PROPERTY(ascii_char_ptr, fwPackageURI),
#endif /* CLD_OTA */
    /* Reported Properties */
    WITH_REPORTED_PROPERTY(bool, LedStatusOn),
#if defined(CLD_OTA)
    WITH_REPORTED_PROPERTY(int, TelemetryInterval),
    WITH_REPORTED_PROPERTY(ascii_char_ptr, currentFwVersion),
    WITH_REPORTED_PROPERTY(ascii_char_ptr, fwUpdateStatus)
#else
    WITH_REPORTED_PROPERTY(int, TelemetryInterval)
#endif /* CLD_OTA */
);

END_NAMESPACE(IotThing);

typedef struct {
  IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
  SerializableIotSampleDev_t *serModel;
  model_strings_t strings;
} IotSampleDev_t;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static bool g_continueRunning;
static bool g_publishData;
static bool g_reboot;
#if defined(CLD_OTA)
static bool g_ExecuteFOTA;
#define FOTA_URI_MAX_SIZE 200
char g_firmware_update_uri[FOTA_URI_MAX_SIZE];
iot_state_t write_ota_state;
#endif /* CLD_OTA */
/*
 * Device registration and authentication method to IoT Hub
 * One of: DEVICE_AUTH_SYMKEY, DEVICE_AUTH_X509, DEVICE_AUTH_DPS
 */
uint8_t g_deviceAuthMethod;

/* Private function prototypes -----------------------------------------------*/
int device_model_create(IotSampleDev_t **pModel);
int device_model_destroy(IotSampleDev_t *model);
int32_t comp_left_ms(uint32_t init, uint32_t now, uint32_t timeout);
void deviceTwinReportedStateCallback(int status_code, void* userContextCallback);

static void setDeviceRegistrationMethod(const char * connectionString);
static void printDeviceRegistrationMethod(void);
static int directIoTHubRegistration(IotSampleDev_t * pDevice, const char * pConnectionString, const char * pCaCert, const char *pClientCert, const char *pClientPrivateKey);
static int setAllCallbacks(IotSampleDev_t * pDevice);

/* Exported functions --------------------------------------------------------*/
int cloud_device_enter_credentials(void)
{
  iot_config_t iot_config;
  int ret = 0;

  memset(&iot_config, 0, sizeof(iot_config_t));

  /* Prompt asking for the connection string : the connection string determines the requested registration method */
  printf("\nAzure IoT Hub connection string templates: \n");
  printf("\tSymmetric Key template: HostName=xxx;DeviceId=xxx;SharedAccessKey=xxxx= \n");
  printf("\tX.509 template: HostName=xxx;DeviceId=xxx;x509=true \n");
  printf("Or custom DPS string of your device: \n");
  printf("\tDPS template: DpsEndpoint=xxx;IdScope=xxx; \n");
  printf("Enter the Azure connection string of your device: \n");

  getInputString(iot_config.device_name, USER_CONF_DEVICE_NAME_LENGTH);
  msg_info("read: --->\n%s\n<---\n", iot_config.device_name);

  if (setIoTDeviceConfig(&iot_config) != 0)
  {
    ret = -1;
    msg_error("Failed programming the IoT device configuration to Flash.\n");
  }
  else
  {
    /*
     * Setting the device registration and authentication method to IoT Hub
     * We need to do this right now, before 'app_needs_device_keypair' is called,
     * to define if we need to ask for a device certificate and device key (X.509 authentication case)
     */
    setDeviceRegistrationMethod(iot_config.device_name);
  }

  return ret;
}


bool app_needs_root_ca(void)
{
  return true;
}


bool app_needs_device_keypair(void)
{
  if (DEVICE_AUTH_X509 == g_deviceAuthMethod)
  {
    /* For an X.509 authentication to IoT Hub we need a device certificate and a device key */
    return true;
  }
  else
  {
    return false;
  }
}

bool app_needs_iot_config(void)
{
  return true;
}

EXECUTE_COMMAND_RESULT LedToggle(SerializableIotSampleDev_t *dev)
{
  IotSampleDev_t * device = (IotSampleDev_t *) dev->devContext;
  unsigned char *buffer;
  size_t bufferSize;

  msg_info("Received LedToggle command.\n");

  dev->LedStatusOn = !dev->LedStatusOn;
  Led_SetState(dev->LedStatusOn);

  /* Report the changed state */
  if (SERIALIZE_REPORTED_PROPERTIES(&buffer, &bufferSize, *dev) != CODEFIRST_OK)
  {
    msg_error("Failed serializing Reported State.\n");
  }
  else
  {
    if (IoTHubClient_LL_SendReportedState(device->iotHubClientHandle, buffer, bufferSize, deviceTwinReportedStateCallback, NULL) != IOTHUB_CLIENT_OK)
    {
      msg_error("Failed sending Reported State.\n");
    }
    free(buffer);
  }

  return EXECUTE_COMMAND_SUCCESS;
}


METHODRETURN_HANDLE Reboot(SerializableIotSampleDev_t *dev)
{
  (void)(dev);
  METHODRETURN_HANDLE result = MethodReturn_Create(1, "{\"Message\":\"Rebooting with Method\"}");
  msg_info("Received Reboot request.\n");
  g_reboot = true;
  return result;
}


METHODRETURN_HANDLE Hello(SerializableIotSampleDev_t *dev, ascii_char_ptr msg)
{
  (void)(dev);
  METHODRETURN_HANDLE result = MethodReturn_Create(1, "{\"Message\":\"Hello with Method\"}");
  msg_info("Received Hello %s.\n", msg);
  return result;
}




static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
  int result;
  char* payloadZeroTerminated = (char*)malloc(size + 1);
  if (payloadZeroTerminated == 0)
  {
    msg_error("Allocation failed.\n");
    *resp_size = 0;
    *response = NULL;
    result = -1;
  }
  else
  {
    (void)memcpy(payloadZeroTerminated, payload, size);
    payloadZeroTerminated[size] = '\0';

    /*execute method - userContextCallback is of type deviceModel*/
    METHODRETURN_HANDLE methodResult = EXECUTE_METHOD(userContextCallback, method_name, payloadZeroTerminated);
    free(payloadZeroTerminated);

    if (methodResult == NULL)
    {
      printf("failed to EXECUTE_METHOD\r\n");
      const char* resp = "{ }";
      *resp_size = sizeof(resp)-1;
      *response = (unsigned char*)malloc(*resp_size);
      (void)memcpy(*response, resp, *resp_size);
      result = -1;
    }
    else
    {
      /* get the serializer answer and push it in the networking stack*/
      const METHODRETURN_DATA* data = MethodReturn_GetReturn(methodResult);
      if (data == NULL)
      {
        printf("failed to MethodReturn_GetReturn\r\n");
        *resp_size = 0;
        *response = NULL;
        result = -1;
      }
      else
      {
        result = data->statusCode;
        if (data->jsonValue == NULL)
        {
          char* resp = "{}";
          *resp_size = strlen(resp);
          *response = (unsigned char*)malloc(*resp_size);
          (void)memcpy(*response, resp, *resp_size);
        }
        else
        {
          *resp_size = strlen(data->jsonValue);
          *response = (unsigned char*)malloc(*resp_size);
          (void)memcpy(*response, data->jsonValue, *resp_size);
        }
      }
      MethodReturn_Destroy(methodResult);
    }
  }
  return result;
}


static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE status_code,  const unsigned char* payload, size_t size, __attribute__((unused)) void* userContextCallback)
{
  IotSampleDev_t * device = userContextCallback;

  msg_info("DeviceTwinCallback payload: %.*s\nStatus_code = %d\n", size, (const char*) payload, status_code);

  JSON_DECODER_RESULT JSONDec;
  MULTITREE_HANDLE multiTreeHandle;
  char* temp = malloc(size + 1);
  if (temp == NULL)
  {
    msg_error("Alloc failed.\n");
    return;
  }

  /* We need to add the missing termination char */
  (void)memcpy(temp, payload, size);
  temp[size] = '\0';

  if ((JSONDec = JSONDecoder_JSON_To_MultiTree(temp,&multiTreeHandle)) != JSON_DECODER_OK)
  {
    msg_error("Decoding JSON Code=%d\r\n",JSONDec);
    free(temp);
    return;
  }

  MULTITREE_HANDLE childHandle;
  if (status_code == DEVICE_TWIN_UPDATE_COMPLETE)
  {
    MultiTree_GetChildByName(multiTreeHandle, "desired", &childHandle);
  }
  else
  {
    /* The payload has no "desired" object. Looking up directly at the property name. */
    childHandle = multiTreeHandle;
  }

  size_t childCount;

  if (MultiTree_GetChildCount(childHandle, &childCount) == MULTITREE_OK)
  {
#if defined(CLD_OTA)
    char const *fwVersion = NULL;
    char const *fwPackageURI = NULL;
#endif /* CLD_OTA */

    /* Loop on all "desired" properties. */
    for (int i = 0; i < childCount; i++)
    {
      MULTITREE_HANDLE childHandle2;
      STRING_HANDLE childName = STRING_new();
      if (childName == NULL)
      {
        msg_error("Allocation failed.\n");
        break;
      }
      else
      {
        if (MultiTree_GetChild(childHandle, i, &childHandle2) == MULTITREE_OK)
        {
          if (MultiTree_GetName(childHandle2, childName) != MULTITREE_OK)
          {
              break;
          }
          else
          {
              const char *childName_str = STRING_c_str(childName);
              /* Match the supported desired properties. */
              if (strcmp("DesiredTelemetryInterval", childName_str) == 0)
              {
                void const *value = NULL;
                if (MultiTree_GetValue(childHandle2, &value) != MULTITREE_OK)
                {
                  msg_error("Failed parsing the desired TelemetryInterval attribute.\n");
                }
                else
                {
                  unsigned char *buffer;
                  size_t bufferSize;

                  device->serModel->TelemetryInterval = atoi(value);
                  msg_info("Setting telemetry interval to %d.\n", device->serModel->TelemetryInterval);

                  if (SERIALIZE_REPORTED_PROPERTIES(&buffer, &bufferSize, *(device->serModel)) != CODEFIRST_OK)
                  {
                    msg_error("Failed serializing Reported State.\n");
                  }
                  else
                  {
                    if (IoTHubClient_LL_SendReportedState(device->iotHubClientHandle, buffer, bufferSize, deviceTwinReportedStateCallback, NULL) != IOTHUB_CLIENT_OK)
                    {
                      msg_error("Failed sending Reported State.\n");
                    }
                    free(buffer);
                  }
                }
              }
#if defined(CLD_OTA)
              if (strcmp("fwVersion", childName_str) == 0)
              {
                if (MultiTree_GetValue(childHandle2, (void const **) &fwVersion) != MULTITREE_OK)
                {
                  msg_error("Failed parsing the desired fwVersion attribute.\n");
                }
              }

              if (strcmp("fwPackageURI", childName_str) == 0)
              {
                if (MultiTree_GetValue(childHandle2, (void const **) &fwPackageURI) != MULTITREE_OK)
                {
                  msg_error("Failed parsing the desired fwPackageURI attribute.\n");
                }
              }
#endif /* CLD_OTA */
          }
        }
      }
    }

#if defined(CLD_OTA)
    if ( (fwVersion != NULL) && (fwPackageURI != NULL) )
    {
      const char https_uri_prefix[] = "\"https://";
      const char http_uri_prefix[]  = "\"http://";
      g_ExecuteFOTA = true;

      if ((strncmp(fwPackageURI, https_uri_prefix, sizeof(https_uri_prefix) - 1) != 0)
          && (strncmp(fwPackageURI, http_uri_prefix, sizeof(http_uri_prefix) - 1) != 0))
      {
        msg_error("Incorrect fwPackageURI (no https:// or http:// prefix) : %s\n", fwPackageURI);
        g_ExecuteFOTA = false;
      }

      /* In the Serializer module, the string values begin and end with " character */
      /* Don't copy the first and last '"' characters */
      if ((strlen(fwVersion) - 2) >= IOT_STATE_FW_VERSION_MAX_SIZE)
      {
         msg_error("fwVersion property is too long.\n");
         g_ExecuteFOTA = false;
      }

      if (g_ExecuteFOTA == false)
      {
        sprintf(device->serModel->fwUpdateStatus, MU_ENUM_TO_STRING(FIRMWARE_UPDATE_STATUS, Error));
      }
      else
      {
        /* Enable the Firmware Update */
        /* Remove the first and last '"' characters */
        strncpy(device->serModel->fwVersion, fwVersion + 1, IOT_STATE_FW_VERSION_MAX_SIZE);
        device->serModel->fwVersion[strlen(device->serModel->fwVersion) - 1] = '\0';
        msg_info("Setting desired fwVersion to %s.\n", device->serModel->fwVersion);

        if (strcmp(device->serModel->fwVersion, device->serModel->currentFwVersion) == 0)
        {
          msg_info("Desired FW version and current FW version are the same. No Firmware Update.\n");
          g_ExecuteFOTA = false;
        }
        else
        {
          strncpy(device->serModel->fwPackageURI, fwPackageURI + 1, FOTA_URI_MAX_SIZE);
          device->serModel->fwPackageURI[strlen(device->serModel->fwPackageURI) - 1] = '\0';
          strncpy(g_firmware_update_uri, device->serModel->fwPackageURI, FOTA_URI_MAX_SIZE);
          msg_info("Setting desired fwPackageURI to %s.\n", g_firmware_update_uri);
        }
      }
    }
    else if ((fwVersion == NULL) ^ (fwPackageURI == NULL))
    {
      msg_error("Cannot find one of the desired properties: fwVersion=%s or fwPackageURI=%s\n", fwVersion, fwPackageURI);
      sprintf(device->serModel->fwUpdateStatus, MU_ENUM_TO_STRING(FIRMWARE_UPDATE_STATUS, Error));
    }
#endif /* CLD_OTA */

  }

  MultiTree_Destroy(multiTreeHandle);
  free(temp);
}


static IOTHUBMESSAGE_DISPOSITION_RESULT MessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
  IOTHUBMESSAGE_DISPOSITION_RESULT result;
  const unsigned char* buffer;
  size_t size;

  if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
  {
    msg_error("unable to IoTHubMessage_GetByteArray\n");
    result = IOTHUBMESSAGE_ABANDONED;
  }
  else
  {
    /*buffer is not zero terminated*/
    char* temp = malloc(size + 1);
    if (temp == NULL)
    {
      msg_error("failed to malloc\n");
      result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
      (void)memcpy(temp, buffer, size);
      temp[size] = '\0';

      msg_info("Received message: %s\n", temp);

      EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
      result =
        (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
        (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
        IOTHUBMESSAGE_REJECTED;
      free(temp);
    }
  }
  return result;
}


void deviceTwinReportedStateCallback(int status_code, void* userContextCallback)
{
  (void)(userContextCallback);
  (void)(status_code);
  msg_info("IoTHub: reported properties delivered.\n");
}


static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
  IOTHUB_MESSAGE_HANDLE msgHnd = userContextCallback;
  msg_info("Confirmation received for message with result = %s\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
  IoTHubMessage_Destroy(msgHnd);
}


/**
 * @brief Set the device registration method
 * @param char * connectionString  connection string to be parsed
 * @retval None
 * @note This function updates the global variable 'g_deviceAuthMethod'
 */
void setDeviceRegistrationMethod(const char * pConnectionString)
{
  if (NULL != strstr(pConnectionString,";x509=true"))
  {
    g_deviceAuthMethod = DEVICE_AUTH_X509;
  }
  else if (NULL != strstr(pConnectionString,";SharedAccessKey="))
  {
    g_deviceAuthMethod = DEVICE_AUTH_SYMKEY;
  }
#if defined(AZURE_DPS_PROV)
  else if (NULL != strstr(pConnectionString, dps_endpoint_prefix))
  {
    g_deviceAuthMethod = DEVICE_AUTH_DPS;
  }
#endif /* AZURE_DPS_PROV */
  else
  {
    g_deviceAuthMethod = DEVICE_AUTH_UNKNOWN;
  }
}

/**
 * @brief prints the device registration method in the console
  * @retval None
 * @note This function uses the global variable 'g_deviceAuthMethod'
 */

void printDeviceRegistrationMethod(void)
{
  if (DEVICE_AUTH_SYMKEY == g_deviceAuthMethod)
  {
    msg_info("Symmetric Key device authentication method selected.\n");
  }
  else if (DEVICE_AUTH_X509 == g_deviceAuthMethod)
  {
    msg_info("X.509 device authentication method selected.\n");
  }
#if defined(AZURE_DPS_PROV)
  else if (DEVICE_AUTH_DPS == g_deviceAuthMethod)
  {
    msg_info("DPS registration procedure selected.\n");
  }
#endif /* AZURE_DPS_PROV */
  else
  {
    msg_error("Unknown device registration and authentication method.\n");
    msg_info("Usage:\n");
    msg_info("\tConnection string ending with \";x509=true\" for X.509 authentication.\n");
    msg_info("\tConnection string ending with \";SharedAccessKey=...\" for symmetric key.\n");
#if defined(AZURE_DPS_PROV)
    msg_info("\tConnection string is \"DpsEndpoint=xxx;IdScope=xxx;\" for DPS registration.\n");
#endif /* AZURE_DPS_PROV */
  }
}

#if defined(AZURE_DPS_PROV)
/* ------------- */
/* DPS Functions */
/* ------------- */

/**
  * @brief  Parse the custom DPS string to extract the DPS endpoint information.
  * @param  In:  const char * custom_dps_string  custom DPS string
  * @param  Out: char ** uri                 Global device endpoint
  * @param  Out: char ** id_scope            ID Scope
  * @retval 0 in case of success and -1 otherwise
  *
  */
static int setDPSconnectionInfo(const char * custom_dps_string, char ** uri, char ** id_scope)
{
  /*
   * ST proprietary DPS string template:
   *      DpsEndpoint=xxx;IdScope=xxx;
   *
   * Assumption: The input string has a valid format.
   *             This function is not robust against incorrect inputs.
   */
  int ret = -1;
  char * prefix = NULL;
  char * postfix = NULL;
  char * value = NULL;
  uint32_t size = 0;

  /* Get the Global device endpoint first */
  prefix = strstr(custom_dps_string, dps_endpoint_prefix);
  value = prefix + strlen(dps_endpoint_prefix);  /* beginning of uri */
  postfix = strstr(value,";");                    /* ; after uri      */
  size = postfix - value;
  *uri = calloc(size + 1, 1);
  if (*uri != NULL)
  {
    memcpy(*uri, value, size);
    (*uri)[size] = '\0';
    ret = 0;
  }
  else
  {
    msg_error("malloc failure for uri.\n");
    ret = -1;
  }

  if (ret == 0)
  {
    /* Get the ID scope */
    prefix = strstr(postfix, id_scope_prefix);
    value = prefix + strlen(id_scope_prefix);
    postfix = strstr(value, ";");
    if (postfix == NULL)
    {
      size = strlen(value);
    }
    else
    {
      size = postfix - value;
    }
    *id_scope = calloc(size + 1, 1);
    if (*id_scope != NULL)
    {
      memcpy(*id_scope, value, size);
      (*id_scope)[size] = '\0';
      ret = 0;
    }
    else
    {
      msg_error("malloc failure for id_scope.\n");
      ret = -1;
    }
  } /* else return */

  return(ret);
}

static void DPSRegistrationStatusCallBack(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
  if (user_context == NULL)
  {
    msg_info("user_context is NULL.\n");
  }
  else
  {
    msg_info("Provisioning Status: %s.\n", MU_ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
  }
}

static void IOTHubDPSRegisterDeviceCallBack(PROV_DEVICE_RESULT register_result, const char* IoTHubUri, const char* DeviceId, void* user_context)
{
  if (user_context == NULL)
  {
    msg_info("user_context is NULL.\n");
  }
  else
  {
    CLIENT_SAMPLE_INFO* user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
    if (register_result == PROV_DEVICE_RESULT_OK)
    {
      msg_info("Registration Information received from service: %s!\n", IoTHubUri);
      msg_info("\tIoTHubUri=%s\n",IoTHubUri);
      msg_info("\tDeviceId=%s\n",DeviceId);
      mallocAndStrcpy_s(&user_ctx->IoTHubUri, IoTHubUri);
      mallocAndStrcpy_s(&user_ctx->DeviceId, DeviceId);
      user_ctx->RegistrationProcess = DPS_PROCESS_AUTH;
    }
    else
    {
      msg_error("DPS Device Registration failure.\n");
      user_ctx->RegistrationProcess = DPS_PROCESS_NOT_AUTH;
    }
  }
}

/**
  * @brief  Connect to IoT Hub using the information retrieved during the DPS procedure
  * @param  IotSampleDev_t * pDevice                  Device structure with the IoT Hub client handle
  * @param  const char * pCaCcert                     Root CA certs
  * @param  const char * puserCtx                     Connection Information obtained from the DPS server
  * @param  IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol Transport Protocol
  * @retval 0 in case of success and different from 0 otherwise
  */
static int registerIoTHubFromDPS(IotSampleDev_t * pDevice, const char * pCaCert, CLIENT_SAMPLE_INFO * pUserCtx, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
  int ret = 0;

  if ((pDevice->iotHubClientHandle = IoTHubClient_LL_CreateFromDeviceAuth(pUserCtx->IoTHubUri, pUserCtx->DeviceId, protocol)) == NULL)
  {
    msg_error("Err: iotHubClientHandle is NULL!\r\n");
    ret = -1;
  }
  else
  {
    msg_info("iotHubClientHandle Created\r\n");
    bool traceOn = false;
    ret = (IoTHubClient_LL_SetOption(pDevice->iotHubClientHandle, "logtrace", &traceOn) != IOTHUB_CLIENT_OK);
    if (ret != 0)
    {
      msg_error("failed setting the option: \"logtrace\".\n");
    }

    ret = (IoTHubClient_LL_SetOption(pDevice->iotHubClientHandle, "TrustedCerts", pCaCert) != IOTHUB_CLIENT_OK);
    if (ret != 0)
    {
      msg_error("failed setting the option: \"TrustedCerts\".\n");
    }
  }

  return(ret);
}

#endif /* AZURE_DPS_PROV */

#if defined(CLD_OTA)
/**
 * @brief Implementation of FirmwareUpdate Direct Method
 * @param SerializableIotSampleDev_t *dev Pointer to the Model instance
 * @param ascii_char_ptr *FwPackageUri OTA URL
 * @retval METHODRETURN_HANDLE Direct Method Result
 */
METHODRETURN_HANDLE FirmwareUpdate(SerializableIotSampleDev_t *dev, ascii_char_ptr FwPackageUri)
{
  (void)(dev);

  METHODRETURN_HANDLE result = MethodReturn_Create(1, "\"Initiating Firmware Update\"");
  msg_info("Received firmware update request. Use package at: [%s]\n", FwPackageUri);

  /* Memorize the URI : the memory allocated for FwPackageUri will be released so we need to copy it */
  if (strlen(FwPackageUri) >= FOTA_URI_MAX_SIZE)
  {
    msg_error("Fw Package URI is too long.\n");
  }
  else
  {
    strncpy(g_firmware_update_uri, FwPackageUri, FOTA_URI_MAX_SIZE);
  }

  /* Make the FOTA */
  g_ExecuteFOTA = true;

  return result;
}

/**
 * @brief Function for reporting the Model Status (dedicated to Firmware Update status)
 * @param FIRMWARE_UPDATE_STATUS status that we want send (Running/Downloading...)
 * @retval None
 */
void ReportState(IotSampleDev_t * device, FIRMWARE_UPDATE_STATUS status)
{
  unsigned char *buffer;
  size_t         bufferSize;

  SerializableIotSampleDev_t * model = device->serModel;

  /* Fill the report State */
  snprintf(model->fwUpdateStatus, FIRMWARE_UPDATE_STATUS_MAX_SIZE, "%s", MU_ENUM_TO_STRING(FIRMWARE_UPDATE_STATUS, status));

  msg_info("Reporting status %s\n", model->fwUpdateStatus);

  /*serialize the model using SERIALIZE_REPORTED_PROPERTIES */
  if (SERIALIZE_REPORTED_PROPERTIES(&buffer, &bufferSize, *model) != CODEFIRST_OK)
  {
    msg_error("Err: Serializing Reported State\r\n");
  }
  else
  {
    /* send the data */
    if (IoTHubClient_LL_SendReportedState(device->iotHubClientHandle, buffer, bufferSize, deviceTwinReportedStateCallback, NULL) != IOTHUB_CLIENT_OK)
    {
      msg_error("Err: Failure Sending Reported State\r\n");
    }
    else
    {
      msg_info("Ok reported State: %s\r\n", model->fwUpdateStatus);
    }
    free(buffer);
  }
}

/**
  * @brief  Function for waiting that the IoT has received all the messages/status sent
  * @param  IotSampleDev_t * device Pointer to the IoT Device
  * @retval None
  */
void WaitAllTheMessages(IotSampleDev_t * device)
{
  /* TODO: is this good enough or shall we do something similar to the FP ? */
  for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
  {
    IoTHubClient_LL_DoWork(device->iotHubClientHandle);
    HAL_Delay(10);
  }
}


/**
  * @brief  Function to close the IoT Hub connection
  * @param  IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle handle to the IoT Hub client
  * @retval None
  */
static void CloseIoTHubConnection(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
  msg_info("===== IoT Hub Disconnect =====\n");
  IoTHubClient_LL_Destroy(iotHubClientHandle);
}

/**
  * @brief  Function to open the IoT Hub connection
  * @param  CLIENT_SAMPLE_INFO * user_ctx Info collected during the DPS registration procedure
  * @param  IotSampleDev_t * device       Device structure with the IoT Hub client handle
  * @param  const char * pConnectionString connection string
  * @param  const char * pCaCert          Root CA certs
  * @param  const char * pClientCert       Device certificate
  * @param  const char * pClientPrivateKey Device private key
  * @retval None
  */
#if defined(AZURE_DPS_PROV)
static void ReopenIoTHubConnection(CLIENT_SAMPLE_INFO * user_ctx, IotSampleDev_t * device, const char * pConnectionString, const char * pCaCert, const char *pClientCert, const char *pClientPrivateKey)
#else
static void ReopenIoTHubConnection(IotSampleDev_t * device, const char * pConnectionString, const char * pCaCert, const char *pClientCert, const char *pClientPrivateKey)
#endif /* AZURE_DPS_PROV */
{
  int ret = 0;

  msg_info("===== IoT Hub re-connection (%d)=====\n", g_deviceAuthMethod);

#if defined(AZURE_DPS_PROV)
  if (DEVICE_AUTH_DPS == g_deviceAuthMethod)
  {
    IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport = MQTT_Protocol;
    if (user_ctx->RegistrationProcess != DPS_PROCESS_AUTH)
    {
      msg_error("Provisioning registration failed!\r\n");
    }
    else
    {
      /* IoT Hub registration */
      ret = registerIoTHubFromDPS(device, pCaCert, user_ctx, iothub_transport);

      if (ret != 0)
      {
        msg_error("IoT Hub reconnection failure.\n");
      }
      else
      {
        msg_info("IoT Hub reconnection success.\n");

        /* Set all the callbacks required to operate with IoT Hub */
        ret = setAllCallbacks(device);

        if (ret != 0)
        {
          msg_error("Failed setting all required callbacks.\n");
        }
        else
        {
          msg_info("Callbacks registered successfully.\n");
        }
      }
    }
  }
  else
  {
#endif /* AZURE_DPS_PROV */
    /*
     * Standard IoT Hub connection.
     * No further check of 'g_deviceAuthMethod' because the FOTA command would not have been received for an incorrect value.
     */
    ret = directIoTHubRegistration(device, pConnectionString, pCaCert, pClientCert, pClientPrivateKey);

    if (ret != 0)
    {
      msg_error("Failed to register to IoT Hub.\n");
    }
    else
    {
      ret = setAllCallbacks(device);

      if (ret != 0)
      {
        msg_error("Failed setting all required callbacks.\n");
      }
      else
      {
        msg_info("Callbacks registered successfully.\n");
      }
    }
#if defined(AZURE_DPS_PROV)
  }
#endif /* AZURE_DPS_PROV */
}

#endif /* CLD_OTA */


/**
  * @brief  Connect to IoT Hub using the connection string
  * @param  IotSampleDev_t * pDevice       Device structure with the IoT Hub client handle
  * @param  const char * pConnectionString connection string
  * @param  const char * pCaCcert          Root CA certs
  * @param  const char * pClientCert       Device certificate
  * @param  const char * pClientPrivateKey Device private key
  * @retval 0 in case of success and different from 0 otherwise
  */
int directIoTHubRegistration(IotSampleDev_t * pDevice, const char * pConnectionString, const char * pCaCert, const char *pClientCert, const char *pClientPrivateKey)
{
  int ret = 0;

  pDevice->iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(pConnectionString, MQTT_Protocol);

  if (pDevice->iotHubClientHandle == NULL)
  {
    ret = -1;
    msg_error("Failed creating the iotHub client.\n");
  }
  else
  {
    bool traceOn = false;
    ret = (IoTHubClient_LL_SetOption(pDevice->iotHubClientHandle, "logtrace", &traceOn) != IOTHUB_CLIENT_OK);

    if (ret != 0)
    {
      msg_error("failed setting the option: \"logtrace\".\n");
    }
    else
    {
      ret = (IoTHubClient_LL_SetOption(pDevice->iotHubClientHandle, "TrustedCerts", pCaCert) != IOTHUB_CLIENT_OK);
    }

    if (ret != 0)
    {
      msg_error("Failed setting the option: \"TrustedCerts\"\n");
    }
    else
    {
      if (DEVICE_AUTH_X509 == g_deviceAuthMethod)
      {
        ret = (IoTHubClient_LL_SetOption(pDevice->iotHubClientHandle, "x509certificate", pClientCert) != IOTHUB_CLIENT_OK);

        if (ret != 0)
        {
          msg_error("Failed setting the option: \"x509certificate\"\n");
        }
        else
        {
          ret = (IoTHubClient_LL_SetOption(pDevice->iotHubClientHandle, "x509privatekey", pClientPrivateKey) != IOTHUB_CLIENT_OK);
        }

        if (ret != 0)
        {
          msg_error("Failed setting the option: \"x509privatekey\"\n");
        }
      } /* else: no need to set the device certificate and key */
    }
  }

  return(ret);
}


/**
  * @brief  Set all the callbacks required to communicate with IoT Hub
  * @param  IotSampleDev_t * pDevice       Device structure with the IoT Hub client handle
  * @retval 0 in case of success and different from 0 otherwise
  */
int setAllCallbacks(IotSampleDev_t * pDevice)
{
  int ret = (IoTHubClient_LL_SetDeviceMethodCallback(pDevice->iotHubClientHandle, DeviceMethodCallback, pDevice->serModel) != IOTHUB_CLIENT_OK);

  if (ret != 0)
  {
    msg_error("Failed registering the device method callback.\n");
  }
  else
  {
    ret = (IoTHubClient_LL_SetMessageCallback(pDevice->iotHubClientHandle, MessageCallback, pDevice->serModel) != IOTHUB_CLIENT_OK);
  }

  if (ret != 0)
  {
    msg_error("Failed registering the message callback.\n");
  }
  else
  {
    ret = (IoTHubClient_LL_SetDeviceTwinCallback(pDevice->iotHubClientHandle, DeviceTwinCallback, pDevice) != IOTHUB_CLIENT_OK);

    if (ret != 0)
    {
      msg_error("Failed registering the twin callback.\n");
    }
  }

  return(ret);
}


/** Main loop */
void cloud_run(void const *arg)
{
  int ret = 0;
  IotSampleDev_t * device = NULL;
  g_continueRunning = true;
  g_publishData = false;
  g_reboot = false;
  g_deviceAuthMethod = DEVICE_AUTH_UNKNOWN;
#if defined(CLD_OTA)
  g_ExecuteFOTA = false;
  const iot_state_t * read_ota_state = NULL;
#endif /* CLD_OTA */
  const char * connectionString = NULL;
  const char * ca_cert = NULL;
  /*
   * IoT Leaf Device: device certificate (signed by the CA uploaded in Azure portal which has been verified with the proof of possession)
   */
  const char *pClientCert = NULL;
  /*
   * IoT Leaf Device: private key
   */
  const char *pClientPrivateKey = NULL;
#if defined(AZURE_DPS_PROV)
  SECURE_DEVICE_TYPE hsm_type = SECURE_DEVICE_TYPE_X509;
  CLIENT_SAMPLE_INFO user_ctx;

  user_ctx.IoTHubUri = NULL;
  user_ctx.DeviceId = NULL;
#endif /* AZURE_DPS_PROV */

  /* Retrieve the connection string and set the connection method */
  ret = (getIoTDeviceConfig(&connectionString) != 0);

  if (ret != 0)
  {
    msg_error("Cannot retrieve the connection string from the user configuration storage.\n");
  }
  else
  {
    if (DEVICE_AUTH_UNKNOWN == g_deviceAuthMethod)
    {
      setDeviceRegistrationMethod(connectionString);
    }
    /*
     * else: method already set when executing 'cloud_device_enter_credentials',
     * no need to parse the connection string again
     * In all cases we print the selected method in the console
     */
    printDeviceRegistrationMethod();

    /* Get the appropriate certificates */
    if (DEVICE_AUTH_X509 == g_deviceAuthMethod)
    {
      /* Root CA + device certificate and key */
      ret = getTLSKeys(&ca_cert, &pClientCert, &pClientPrivateKey);
    }
    else
    {
      /* Root CA only */
      ret = getTLSKeys(&ca_cert, NULL, NULL);
    }

    if (ret != 0)
    {
      msg_error("Cannot retrieve the authentication items (certificates and/or key) from the user configuration storage.\n");
    }

    ret = (SERIALIZER_REGISTER_NAMESPACE(IotThing) == NULL);

    if (ret != 0)
    {
      msg_error("SERIALIZER_REGISTER_NAMESPACE failed.\n");
    }
    else
    {
      ret = device_model_create(&device);
    }
  }

#if defined(AZURE_DPS_PROV)
  if (DEVICE_AUTH_DPS == g_deviceAuthMethod)
  {
    msg_info("====== DPS procedure initiation ======\n");
    /* ------------------------- */
    /* DPS registration          */
    /* ------------------------- */
    if (ret != 0)
    {
      msg_error("Failed to create the device model.\n");
    }
    else
    {
      /* Initialize the HSM : this module will provide the device alias certificate and the alias key */
      ret = prov_dev_security_init(hsm_type);

      if (ret != 0)
      {
        msg_error("Failed to initialize the Provisioning Secure Module\n");
      }
      else
      {
        IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport = MQTT_Protocol;
        PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION g_prov_transport = Prov_Device_MQTT_Protocol;
        PROV_DEVICE_LL_HANDLE handle;

        user_ctx.RegistrationProcess = DPS_PROCESS_START;
        user_ctx.SleepTime = 10;

        msg_info("Provisioning API Version: %s\n", Prov_Device_LL_GetVersionString());

#if defined(AZURE_DPS_PROOF_OF_POSS)
        /* User interaction to handle the proof of possession */
        /* -------------------------------------------------- */
        printf("Push the User button (Blue) within the next 5 seconds if you want to generate "
               "the DPS proof of possession response.\n\n");

        if (Button_WaitForPush(5000) != BP_NOT_PUSHED)
        {
          char verification_code[128];
          char * response_certificate = NULL;
          /*
           * Generate a Verification Certificate for the Root CA (or intermediate CA) used for the enrollment group.
           * This proves your ownership of the certificate used for group enrollment so this proves that
           * you are authorized to handle the group and generate leaf certificates (alias certificates).
           *
           * Activation steps:
           *  1. Upload the X.509 HSM Root CA in the DPS portal (this CA is in the certificates chain of the device, see hsm_client_riot_STM32Cube.c);
           *  2. Get the verification code from the DPS portal;
           *  3. Run this procedure on STM32 side to get the proof of possession response (that is the "verification certificate" .pem);
           *  4. Run the verification in the DPS portal.
           */

          /* X.509 HSM handle */
          handle = hsm_client_riot_create();

          if (NULL == handle)
          {
            msg_error("hsm_client_riot_create failure.\n");
          }
          else
          {
            /* Enter Verification Code obtained from DPS portal */
            msg_info("Proof Of Possession: enter the verification code.\n");
            getInputString(verification_code, 128);
            msg_info("Verification code: %s.\n", verification_code);

            /* Get Response Certificate */
            response_certificate = hsm_client_riot_create_leaf_cert(handle, verification_code);

            if (NULL == response_certificate )
            {
              msg_error("Response generation failed.\n");
            }
            else
            {
              msg_info("Response:\r\n%s\r\n\r\n",response_certificate);
              free(response_certificate);
            }

            /* Destroy the handle */
            hsm_client_riot_destroy(handle);
          }
        }
        /* else skip the proof of possession */
#endif /* AZURE_DPS_PROOF_OF_POSS */

        /* Retrieve the DPS global device endpoint and ID Scope from the ST proprietary DPS string */
        ret = setDPSconnectionInfo(connectionString, &global_prov_uri, &id_scope);

        if (ret != 0)
        {
          msg_error("setDPSconnectionInfo failure.\n");
        }
        else
        {
          msg_info("DPS uri: %s\n", global_prov_uri);
          msg_info("DPS id_scope: %s\n", id_scope);
        }

        if ((handle = Prov_Device_LL_Create(global_prov_uri, id_scope, g_prov_transport)) == NULL)
        {
          msg_error("failed calling Prov_Device_LL_Create.\n");
        }
        else
        {
          /* DPS registration can start */
#ifdef ENABLE_IOT_DEBUG
          bool traceOn = true;
#else
          bool traceOn = false;
#endif /* ENABLE_IOT_DEBUG */
          Prov_Device_LL_SetOption(handle, "logtrace", &traceOn);
          if (Prov_Device_LL_SetOption(handle, "TrustedCerts", ca_cert) == PROV_DEVICE_RESULT_OK)
          {
            msg_info("Done Prov_Device_LL_SetOption TrustedCerts.\n");
          }
          else
          {
            msg_error("Err for Prov_Device_LL_SetOption \"TrustedCerts\".\n");
          }

          if (Prov_Device_LL_Register_Device(handle, IOTHubDPSRegisterDeviceCallBack, &user_ctx, DPSRegistrationStatusCallBack, &user_ctx) != PROV_DEVICE_RESULT_OK)
          {
            msg_error("failed calling Prov_Device_LL_Register_Device\r\n");
          }
          else
          {
            do
            {
              Prov_Device_LL_DoWork(handle);
              HAL_Delay(user_ctx.SleepTime);
            } while (user_ctx.RegistrationProcess == DPS_PROCESS_START);
          }

          msg_info("DPS call to Prov_Device_LL_Destroy\r\n");
          Prov_Device_LL_Destroy(handle);
        }

        if (user_ctx.RegistrationProcess != DPS_PROCESS_AUTH)
        {
          msg_error("Provisioning registration failed!\r\n");
          ret = -1;
        }
        else
        {
          msg_info("===== DPS part completed successfully: IoT Hub connection =====\n");

          ret = registerIoTHubFromDPS(device, ca_cert, &user_ctx, iothub_transport);

          if (ret != 0)
          {
            msg_error("Failed to connect to IoTHub with DPS info.\n");
          }
        }
      }
    }
  } /* if DEVICE_AUTH_DPS == g_deviceAuthMethod */
  else
  {
#endif /* AZURE_DPS_PROV */

    /* --------------------------- */
    /* IoT Hub direct registration */
    /* --------------------------- */
    if (ret != 0)
    {
      msg_error("Failed to create the device model.\n");
    }
    else
    {

      ret = directIoTHubRegistration(device, connectionString, ca_cert, pClientCert, pClientPrivateKey);

      if (ret != 0)
      {
        msg_error("Failed to register to IoT Hub.\n");
      }
    }

#if defined(AZURE_DPS_PROV)
  } /* end of the else to handle the case with DPS enabled (compiler switch) but not requested */
#endif /* AZURE_DPS_PROV */

  ret = setAllCallbacks(device);

  if (ret != 0)
  {
    msg_error("Failed setting all required callbacks.\n");
  }
  else
  {
    msg_info("Callbacks registered successfully.\n");

    /* Report the initial state. */
    unsigned char *buffer;
    size_t bufferSize;
    macaddr_t mac = { 0 };
    if (net_if_get_mac_address(NULL, &mac) == NET_OK)
    {
      snprintf(device->serModel->mac, MODEL_MAC_SIZE, "%02X%02X%02X%02X%02X%02X", mac.mac[0], mac.mac[1], mac.mac[2], mac.mac[3], mac.mac[4], mac.mac[5]);
    }
    else
    {
      msg_warning("Could not retrieve the MAC address to set the device ID.\n");
      snprintf(device->serModel->mac, MODEL_MAC_SIZE, "UnknownMAC");
    }
#if defined(AZURE_DPS_PROV)
    if ((user_ctx.DeviceId != NULL) && (user_ctx.DeviceId[0] != '\0'))
    {
      strncpy(device->serModel->deviceId, user_ctx.DeviceId, MODEL_DEVICEID_SIZE);
    }
    else
    {
      strncpy(device->serModel->deviceId, device->serModel->mac, MODEL_DEVICEID_SIZE);
    }
#endif /* AZURE_DPS_PROV */

    device->serModel->TelemetryInterval = MODEL_DEFAULT_TELEMETRYINTERVAL;
    device->serModel->LedStatusOn = MODEL_DEFAULT_LEDSTATUSON;
    Led_SetState(device->serModel->LedStatusOn);

#if defined(CLD_OTA)
    /* Device's Current Firmware Version */
    snprintf(device->serModel->currentFwVersion, IOT_STATE_FW_VERSION_MAX_SIZE, "%d.%d.%d", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);

    /* Device's update Status */
    snprintf(device->serModel->fwUpdateStatus, FIRMWARE_UPDATE_STATUS_MAX_SIZE, "%s", MU_ENUM_TO_STRING(FIRMWARE_UPDATE_STATUS, Current));

    /* Retrieve ota state if any */
    ret = getIoTState(&read_ota_state);

    if (ret != 0)
    {
      msg_info("getIoTState: read error or no info.\n");
      read_ota_state = NULL;
    }
    else
    {
      /* Check if a FOTA installation procedure was requested when rebooting */
      if (FOTA_INSTALLATION_REQUESTED == read_ota_state->fota_state)
      {
        /* Version which processed the FOTA request */
        msg_info("FOTA update requested by version: %s\n", read_ota_state->prev_fw_version);
        msg_info("Current version: %s\n", device->serModel->currentFwVersion);

        if (0 == strncmp(read_ota_state->prev_fw_version, device->serModel->currentFwVersion, IOT_STATE_FW_VERSION_MAX_SIZE))
        {
          /*
           * The Firmware Version stored in FLASH (version before new firmware installation) is the same as the current one.
           * This means that the Firmware Installation Procedure failed and we are still running the previous Firmware.
           * (The firmware version stored in FLASH is the version of the Firmware which downloaded the new firmware image).
           *
           * Warning: this logic does not work if you use a FOTA procedure to re-apply the very same Firmware version.
           *          The assumption is that the Firmware Versions are monotonic.
           */

          /* Device's update Status */
          snprintf(device->serModel->fwUpdateStatus, FIRMWARE_UPDATE_STATUS_MAX_SIZE, "%s", MU_ENUM_TO_STRING(FIRMWARE_UPDATE_STATUS, Error));
          msg_info("FOTA update error.\n");
        }
        else
        {
          /*
           * The active Firmware has a different version from the Firmware that processed the FOTA request.
           * This means the installation procedure succeeded.
           */
          msg_info("FOTA update successful.\n");
        }

        /* Reset the Flash information until next FOTA request */
        memset(&write_ota_state, 0x00, sizeof(iot_state_t));

        write_ota_state.fota_state = FOTA_INSTALLATION_NOT_REQUESTED;
        /* No need to update write_ota_state.prev_fw_version: already filled with 0s */

        ret = setIoTState(&write_ota_state);

        if (0 != ret)
        {
          msg_error("setIoTDeviceConfig(FOTA_INSTALLATION_NOT_REQUESTED) failed.\n");
        } /* else nothing to do */
      }
    } /* else: nothing to do, report the status as 'Current' */
#endif /* CLD_OTA */

    if (SERIALIZE_REPORTED_PROPERTIES(&buffer, &bufferSize, *device->serModel) != CODEFIRST_OK)
    {
      msg_error("Serializing Reported State.\n");
    }
    else
    {
      if (IoTHubClient_LL_SendReportedState(device->iotHubClientHandle, buffer, bufferSize, deviceTwinReportedStateCallback, NULL)!= IOTHUB_CLIENT_OK)
      {
        msg_error("Failure Sending Reported State.\n");
      }
      free(buffer);
    }

    for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
    {
      IoTHubClient_LL_DoWork(device->iotHubClientHandle);
    }

    /* Loop sending telemetry data. */
    uint32_t last_telemetry_time_ms = HAL_GetTick();
    do
    {
      uint8_t command = Button_WaitForMultiPush(500);
      bool b_sample_data = (command == BP_SINGLE_PUSH); /* If short button push, publish once. */
      if (command == BP_MULTIPLE_PUSH)                  /* If long button push, toggle the telemetry publication. */
      {
        g_publishData = !g_publishData;
        msg_info("%s the sensor values publication loop.\n", (g_publishData == true) ? "Enter" : "Exit");
      }

      int32_t left_ms = comp_left_ms(last_telemetry_time_ms, HAL_GetTick(), device->serModel->TelemetryInterval * 1000);

      if ( ((g_publishData == true) && (left_ms <= 0))
          || (b_sample_data == true) )
      {
        SerializableIotSampleDev_t * mdl = device->serModel;
        unsigned char* destination;
        size_t destinationSize;

        last_telemetry_time_ms = HAL_GetTick();

        /* Read the Data from the sensors */
        time_t time = TimingSystemGetSystemTime();
        memset(&mdl->ts.dateTime, 0, sizeof(EDM_DATE_TIME_OFFSET));
        mdl->ts.dateTime = *(gmtime(&time));
        {
#ifdef SENSOR
#define INSTANCE_TEMPERATURE_HUMIDITY 0
#define INSTANCE_TEMPERATURE_PRESSURE 1
#define INSTANCE_GYROSCOPE_ACCELEROMETER 0
#define INSTANCE_MAGNETOMETER 1
          BSP_MOTION_SENSOR_Axes_t acc_value;
          BSP_MOTION_SENSOR_Axes_t gyr_value;
          BSP_MOTION_SENSOR_Axes_t mag_value;
          BSP_ENV_SENSOR_GetValue(INSTANCE_TEMPERATURE_HUMIDITY, ENV_TEMPERATURE, &mdl->TEMPERATURE);
          BSP_ENV_SENSOR_GetValue(INSTANCE_TEMPERATURE_HUMIDITY, ENV_HUMIDITY, &mdl->HUMIDITY);
          BSP_ENV_SENSOR_GetValue(INSTANCE_TEMPERATURE_PRESSURE, ENV_PRESSURE, &mdl->PRESSURE);
          mdl->proximity = VL53L0X_PROXIMITY_GetDistance();
          BSP_MOTION_SENSOR_GetAxes(INSTANCE_GYROSCOPE_ACCELEROMETER, MOTION_ACCELERO, &acc_value);
          mdl->ACCELEROMETERX = acc_value.x;
          mdl->ACCELEROMETERY = acc_value.y;
          mdl->ACCELEROMETERZ = acc_value.z;
          BSP_MOTION_SENSOR_GetAxes(INSTANCE_GYROSCOPE_ACCELEROMETER, MOTION_GYRO, &gyr_value);
          mdl->GYROSCOPEX = gyr_value.x;
          mdl->GYROSCOPEY = gyr_value.y;
          mdl->GYROSCOPEZ = gyr_value.z;
          BSP_MOTION_SENSOR_GetAxes(INSTANCE_MAGNETOMETER, MOTION_MAGNETO, &mag_value);
          mdl->MAGNETOMETERX = mag_value.x;
          mdl->MAGNETOMETERY = mag_value.y;
          mdl->MAGNETOMETERZ = mag_value.z;

          /* Serialize the device data. */
          if (SERIALIZE(&destination, &destinationSize,
                mdl->mac,
#if defined(AZURE_DPS_PROV)
                mdl->deviceId,
#endif /* AZURE_DPS_PROV */
                mdl->TEMPERATURE, mdl->HUMIDITY, mdl->PRESSURE, mdl->proximity,
                mdl->ACCELEROMETERX , mdl->ACCELEROMETERY, mdl->ACCELEROMETERZ,
                mdl->GYROSCOPEX , mdl->GYROSCOPEY, mdl->GYROSCOPEZ,
                mdl->MAGNETOMETERX , mdl->MAGNETOMETERY, mdl->MAGNETOMETERZ,
                mdl->ts) != CODEFIRST_OK)
#else /* SENSOR */
          /* Serialize the device data. */
          if (SERIALIZE(&destination, &destinationSize,
                mdl->mac,
#if defined(AZURE_DPS_PROV)
                mdl->deviceId,
#endif /* AZURE_DPS_PROV */
                mdl->ts) != CODEFIRST_OK)
#endif /* SENSOR */
          {
            msg_error("Failed to serialize.\n");
          }
          else
          {
            IOTHUB_MESSAGE_HANDLE msgHnd = NULL;
            /* Create the message. */
            /* Note: The message is destroyed by the confirmation callback. */
            if ((msgHnd = IoTHubMessage_CreateFromByteArray(destination, destinationSize)) == NULL)
            {
              msg_error("Failed allocating an iotHubMessage.\n");
            }
            else
            {
              /* Send the message. */
              if (IoTHubClient_LL_SendEventAsync(device->iotHubClientHandle, msgHnd, SendConfirmationCallback, msgHnd) != IOTHUB_CLIENT_OK)
              {
                msg_error("IoTHubClient_LL_SendEventAsync failed.\n");
              }

            }
            free(destination);
          }
        }

        /* Visual notification of the telemetry publication: LED blink. */
        Led_Blink(80, 40, 5);
        /* Restore the LED state */
        Led_SetState(device->serModel->LedStatusOn);
      }

      for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
      {
        IoTHubClient_LL_DoWork(device->iotHubClientHandle);
      }

#if defined(CLD_OTA)
      /* Execute the FOTA */
      if (true == g_ExecuteFOTA)
      {
        g_ExecuteFOTA = false;

        /* Download the new firmware image */
        ReportState(device, Downloading);
        WaitAllTheMessages(device);

        /* Close the IoTHub connection before next TLS connection to blob storage service */
        CloseIoTHubConnection(device->iotHubClientHandle);
        ret = rfu_update(g_firmware_update_uri, ca_cert);

        /* Re-open IoTHub connection  */
#if defined(AZURE_DPS_PROV)
        ReopenIoTHubConnection(&user_ctx, device, connectionString, ca_cert, pClientCert, pClientPrivateKey);
#else
        ReopenIoTHubConnection(device, connectionString, ca_cert, pClientCert, pClientPrivateKey);
#endif /* AZURE_DPS_PROV */

        if (RFU_OK == ret)
        {
          /* The download succeeded: the new firmware image is ready to be installed */
          msg_info("The Board will restart in for Applying the OTA\r\n");
          ReportState(device, Applying);
          WaitAllTheMessages(device);
          g_reboot = true;

          /*
           * Memorize that the bootloader will run the Firmware Installation procedure.
           * This will be useful at next boot to determine the proper status ('Error' or 'Current') to be reported.
           */
          memset(&write_ota_state, 0x00, sizeof(iot_state_t));

          write_ota_state.fota_state = FOTA_INSTALLATION_REQUESTED;
          /*
           * Store the current firmware version in FLASH.
           * This is used at next boot to determine if the installation procedure succeeded or not.
           * If the running firmware has the same version this means the installation procedure failed.
           * If the running firmware has a different version then it means the installation procedure succeeded.
           */
          strncpy(write_ota_state.prev_fw_version, device->serModel->currentFwVersion, IOT_STATE_FW_VERSION_MAX_SIZE);

          ret = setIoTState(&write_ota_state);

          if (0 != ret)
          {
            msg_error("setIoTDeviceConfig(FOTA_INSTALLATION_REQUESTED) failed.\n");
          }
          else
          {
            msg_info("%s memorized as previous FW version.\n", write_ota_state.prev_fw_version);
          }
        }
        else
        {
          /* Download failure */
          msg_error("rfu_update failure: %d.\n", ret);

          /* Something Wrong */
          ReportState(device, Error);
          WaitAllTheMessages(device);
        }
      }
#endif /* CLD_OTA */

      ThreadAPI_Sleep(500);
    } while (g_continueRunning && !g_reboot);

    msg_info("cloud_run / iothub_client_XCube_sample_run exited, call DoWork %d more time to complete final sending...\n", DOWORK_LOOP_NUM);
    for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
    {
      IoTHubClient_LL_DoWork(device->iotHubClientHandle);
    }
  }

  if (device->iotHubClientHandle != NULL)
  {
    IoTHubClient_LL_Destroy(device->iotHubClientHandle);
    device->iotHubClientHandle = NULL;
  }

  ret = device_model_destroy(device);
  if (ret != 0) msg_error("Failed destroying the device model.\n");

  serializer_deinit();

#if defined(AZURE_DPS_PROV)
  /* Clean the user_ctx */
  free(user_ctx.IoTHubUri);
  free(user_ctx.DeviceId);

  /* Clean the DPS endpoint info */
  free(global_prov_uri);
  free(id_scope);
#endif /* AZURE_DPS_PROV */

  if (g_reboot == true)
  {
    msg_info("Calling HAL_NVIC_SystemReset()\n");
    HAL_NVIC_SystemReset();
  }

}


int device_model_create(IotSampleDev_t ** pDev)
{
  int ret = -1;
  IotSampleDev_t *dev = malloc(sizeof(IotSampleDev_t));
  if (dev == NULL)
  {
    msg_error("Model alloc failed.\n");
  }
  else
  {
    memset(dev, 0, sizeof(IotSampleDev_t));
    dev->serModel = CREATE_MODEL_INSTANCE(IotThing, SerializableIotSampleDev_t);
    if (dev->serModel == NULL)
    {
      msg_error("Ser model alloc failed.\n");
      free(pDev);
    }
    else
    {
      /* Keep a reference to the upper device structure.
       * It is used in the device methods or actions implementation.
       */
      dev->serModel->devContext = (int32_t) dev;

      /* Register the buffers for all the string model attributes */
      dev->serModel->mac = dev->strings.mac;
      dev->serModel->deviceId = dev->strings.deviceId;

      ret = 0;

#if defined(CLD_OTA)
      /* Allocate Space for storing desired Firmware Version */
      dev->serModel->fwVersion = (char *)calloc(IOT_STATE_FW_VERSION_MAX_SIZE, 1);
      if (dev->serModel->fwVersion == NULL)
      {
        msg_error("Err Allocating memory for desired firmware version.\n");
        ret = -1;
      }

      /* Allocate Space for storing desired Firmware location */
      dev->serModel->fwPackageURI = (char *)calloc(FOTA_URI_MAX_SIZE, 1);
      if (dev->serModel->fwPackageURI == NULL)
      {
        msg_error("Err Allocating memory for desired firmware URI.\n");
        ret = -1;
      }

      /* Allocate Space for storing current Firmware Version */
      dev->serModel->currentFwVersion = (char *)calloc(IOT_STATE_FW_VERSION_MAX_SIZE, 1);
      if (dev->serModel->currentFwVersion == NULL)
      {
        msg_error("Err Allocating memory for current firmware version.\n");
        ret = -1;
      }

      /* Allocate Space for storing Device Status */
      dev->serModel->fwUpdateStatus = (char *)calloc(FIRMWARE_UPDATE_STATUS_MAX_SIZE, 1);
      if (dev->serModel->fwUpdateStatus == NULL)
      {
        msg_error("Err Allocating memory for device status.\n");
        ret = -1;
      }

      if (ret == -1)
      {
        free(dev->serModel->fwPackageURI);
        free(dev->serModel->fwVersion);
        free(dev->serModel->currentFwVersion);
        free(dev->serModel->fwUpdateStatus);
      }
#endif /* CLD_OTA */

      *pDev = dev;
    }
  }

  return ret;
}

int device_model_destroy(IotSampleDev_t *dev)
{
#if defined(CLD_OTA)
  free(dev->serModel->fwVersion);
  free(dev->serModel->fwUpdateStatus);
  free(dev->serModel->currentFwVersion);
  free(dev->serModel->fwPackageURI);
#endif /* CLD_OTA */
  free(dev->serModel);
  free(dev);
  return 0;
}

/**
 * @brief   Return the integer difference between 'init + timeout' and 'now'.
 *          The implementation is robust to uint32_t overflows.
 * @param   In:   init      Reference index.
 * @param   In:   now       Current index.
 * @param   In:   timeout   Target index.
 * @retval  Number of units from now to target.
 */
int32_t comp_left_ms(uint32_t init, uint32_t now, uint32_t timeout)
{
  uint32_t elapsed = 0;

  if (now < init)
  { /* Timer wrap-around detected */
    /* printf("Timer: wrap-around detected from %d to %d\n", init, now); */
    elapsed = UINT32_MAX - init + now;
  }
  else
  {
    elapsed = now - init;
  }

  return timeout - elapsed;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
