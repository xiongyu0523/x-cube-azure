/**
  @page Microsoft Azure IoT Cloud application

  @verbatim
  ******************************************************************************
  * @file    readme.txt
  * @author  MCD Application Team
  * @brief   Description of the Azure Cloud application.
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. All rights reserved.
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  @endverbatim

@par Application Description

The AzureXcubeSample application illustrates various ways for an Azure device to interact with an
Azure IoT hub, using the Azure SDK for C low level interface and the Azure SDK for C serializer.

  Notes:
    - By contrast to the generic interface, the Azure low level interface lets the application explicitly
      schedule the network traffic.
      This allows the user to time-slice the network interface activity, and relieves from running
      a dedicated working thread for handling the network events. It can ease with a low-power SW design.
    - The serializer helps reliably building JSON strings to be sent to the IoT hub, from the device model
      attributes. It is not required - the sample applications provided by Microsoft in the Azure SDK for C
      do not use the serializer.


The connection to the hub is configured by the user at application startup thanks to the device console.
The network connections are secured by TLS.

The application supports the STM32L496G-Discovery board and connects to the Internet through the on-board or attached
network interface.


@par Hardware and Software environment

  - MCU board: 32L496G-DISCO (MB1261 rev B)

  - Quectel xG96 daughterboard connected to the MCU board IO expander: Modem package upside, SIM card
    slot downside.

    BG96 is the modem used by default. In case you want to connect a UG96 modem, you may want to
    activate the UG96 experimental support by defining the USE_MODEM_UG96 compilation switch in your
    project instead of USE_MODEM_BG96.

    See X-CUBE-CELLULAR package for further information: https://www.st.com/en/embedded-software/x-cube-cellular.html

  - (optional) Micro B USB cable, as additional power supply to the cellular daughterboard.
    This may improve the modem stability in case the computer USB port connected to the ST-Link cannot
    deliver enough power, or sometimes in limit of cellular coverage. LED4 on STM32L496 blinks when current
    limitation occurs.

  - You can use either the embedded SIM soldered on the expansion cellular board
    or insert a physical SIM card in the expansion board SIM slot.

    - To activate the embedded SIM soldered on the expansion cellular board

      * The kit comes with a 3-months free of charge subscription with 15MB of data using the embedded SIM.
      * The user must go to the stm32-c2c.com portal and enter a voucher to activate this subscription.
      * The Voucher is found by connecting the board to an hyperterminal as specified in the blister.
      * If the board needs to be reinitialized with the original firmware, this one is available
        at https://stm32-c2c.com, see "Restore factory firmware".
      * When the board has been enrolled on the https://stm32-c2c.com portal with its voucher,
        proceed with the EMnify button available on this portal.

    - To activate a cellular SIM card in SIM slot:

      In order to activate the EMnify SIM card:
      * Follow the EMnify Setup Guide available in the Knowledge Base section of https://support.emnify.com

      Note for corporate users: Your card may have already been registered by the person who received the batch.
      You can skip the account creation, and:
      * get your email address added to the list of users of the EMnify corporate account;
      * click on the activation link you will receive by email;
      * login and activate your card from its IccID;
      * create your endpoint.
      See the details in "Enabling SIMs" section and below of the EMnify Setup Guide.

      If you are not sure that the IccID printed on your card is valid, you can cross-check with the
      "SIM Id (IccID):" value displayed on the console during the application startup.

  - A Cellular access point is required.

  - Modem Firmware upgrade
    * The Firmware modem (xG96) may need to be upgraded. Most of time it is preferable to migrate to
      the latest version.
    * Check and follow the x-cube-cellular Release Note (Cellular framework),
      available at https://www.st.com/en/embedded-software/x-cube-cellular.html (SW content)
    * Modem Firmware is within the Quectel xG96 SW upgrade zip file (with all instructions inside),
      available at https://stm32-c2c.com (in precompile demos repository)

  - BG96 bands selection
    * Network selection and radio access may take time at first usage.
    * It may be needed to optimize and accelerate the searching procedure.
    * It is done by tuning the BG96 modem Firmware with bands deployed in your region, country or by operator.
    * A BG96 cellular configuration tool is available at https://stm32-c2c.com

  - Azure account
    * Using the account, create an IoT Hub.
      See https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-device-sdk-c-intro
    * Create a DPS instance if you want to use the Device Provisioning Service.
      You will have to add the "test root CA" certificate which is hard-coded in the Azure
      embedded SDK to the instance.
      See https://docs.microsoft.com/en-us/azure/iot-dps

  - A development PC for building the application, programming through ST-Link, and running the virtual console.


@par How to use it ?

For the application to work, you must follow these steps:

Azure device creation for direct IoT Hub connection
  - Log-in to Azure web portal https://portal.azure.com
  - If not already done, create an IoT Hub (in All Services / Internet of Things / IoT Hub / Add)
  - Go to the IoT Hub instance (click on its name)
  - Go to Explorers / IoT Devices. Click on "+ Add" to create a Device.
  - Enter Device ID (example: MAC address). For "Authentication type",
    select "Symmetric key". "Auto-generate key" must be checked.
  - Once the device is created in Azure, take note of the Primary Connection String in
    the device properties.
    The AzureXcubeSample application provided by this package will request the
    connection string on the console when it is launched for the first time.

Note: Alternatively to the "Symmetric key" authentication type, the sample application also supports:
  * Direct X.509 authentication to the IoT hub (self-signed or CA-signed).
    In that case, the user generates the device certificate and private key, and provisions them
    when asked by the application, after having entered the device connection string as specified
    on the console.

  * Group enrollment and indirect connection through the Azure Device Provisioning Service.
    Relying on the Hardware Security Module emulator build in the sample application, in order to
    generate X.509 device credentials at connection time.
    In that case, the user:
    - enters an application-specific "connection string" as specified on the console. It contains the
      location and identifier of the user DPS endpoint;
    - relies on the "test root CA" certificate and private key which are hard-coded in Azure SDK;
    - adds that certificate to the DPS instance on Azure portal;
    - and activates that certificate through a proof of possession procedure on the web portal.
      This can be done with the help of the sample application, which can generate a disposable
      "verification certificate".

  See the "Device authentication variants" section below for more information.


Application build and flash
  - Open and build the project with one of the supported development toolchains (see the release note
    for detailed information about the version requirements).

  - Program the STM32 board with the firmware binary: By copy (or drag and drop) of the generated ELF
    file to the USB mass storage location created when the STM32 board is connected to your PC USB.
    If the host is a Linux PC, the STM32 device can be found in the /media folder with the name
    e.g. "DIS_XXXXX". XXXXX depends on the MCU model.
    For instance, if the created mass storage location is "/media/DIS_XXXXX", then the command to
    program the board with a binary file named "my_firmware.bin" is simply: cp my_firmware.bin
    /media/DIS_XXXXX.

   Alternatively, you can program the STM32 board directly through one of the supported development
   toolchains.


Application first launch

  - Connect the board to your development PC through USB (ST-LINK USB port).
    Open the console through a serial terminal emulator (e.g. TeraTerm), select the ST-LINK COM port of your
    board and configure it with:
    - 8N1, 115200 bauds, no HW flow control;
    - set the line endings to LF or CR-LF (Transmit) and LF (receive).

  - On the console:

    - Enter your C2C network configuration (SIM operator access point code, username and password).
      Example:
        with Emnify SIM:  access point: "EM",     username: "", password: ""
        with Eseye SIM:   access point: "ESEYE1", username: "", password: ""

    - Set the device connection string (see above), without including the enclosing quotes (") or leading/trailing spaces.

    - Set the TLS root CA certificates:
      Copy-paste the contents of Middlewares\Third_Party\azure-iot-sdk-c\certs\usertrust_baltimore.pem.
      The device uses them to authenticate the remote hosts through TLS.

      Note: The AzureXcubeSample application requires that a concatenation of 2 CA certificates is provided
          1) for the HTTPS server which is used to retrieve the current time and date at boot time.
             (the "Usertrust" certificate).
          2) for the IoT hub server (the "Baltimore" certificate).
          The concatenated string must end with an empty line. This is usertrust_baltimore.pem.

      Note: The Baltimore CA authenticates the azure-devices.net domain.
            If your IoT hub belongs to a different domain, you may have to select and configure a
            different root CA certificate.


     - If your connection string follows the X.509 template, you will also be asked to enter the device
       certificate and its private key.

     - If your connection string follows the DPS template, you will be proposed to run the proof of
       possession procedure as described at https://docs.microsoft.com/en-us/azure/iot-dps/how-to-verify-certificates.
       Push the User button (blue button) on the board,
        * Use the "test root CA" certificate displayed on the console, to create a DPS certificate
          on the portal.
        * Copy/paste the "Verification Code" of the certificate from the portal to the console,
          and get in return a "Verification certificate" that you will upload to the portal to
          complete the activation.

     - After the parameters are configured, it is possible to change them by restarting the board
       and pushing the User button (blue button) just after boot.


Application runtime
  - The application makes an HTTPS request to retrieve the current time and date, and configures the RTC.

      Note: HTTPS has the advantage over NTP that the server can be authenticated by the board, preventing
            a possible man-in-the-middle attack.
            However, the first time the board is switched on (and each time it is powered down and up, if the RTC
            is not backed up), the verification of the server certificate will fail as its validity period will
            not match the RTC value.
            The following log will be printed to the console: "x509_verify_cert() returned -9984 (-0x2700)"

            If the CLOUD_TIMEDATE_TLS_VERIFICATION_IGNORE switch is defined in cloud.c (it is the case by default),
              this error is ignored and the RTC is updated from the "Date:" field of the HTTP response header.
            Else, a few more error log lines are printed, and the application retries to connect without verifying
              the server certificate.
            If/once ok, the RTC is updated.

  - If specified by the connection string entered by the used on the device console, it connects to the
    Device Provisioning Service to retrieve the hub connection information, and generate a session certificate.

  - It connects to the Azure IoT hub,
      - Gets the contents of the device twin, and displays it on the console:
        DeviceTwinCallback payload: {
            "desired": {
                "DesiredTelemetryInterval": 1,
                "version": 9
            },
            "reported": {
                "fwUpdateStatus": "Current",
                "currentFwVersion": "2.2.0",
                "TelemetryInterval": 1,
                "LedStatusOn": true,
                "version": 59
            }
        }
      - Updates its local properties (DesiredTelemetryInterval) from the device twin "desired properties";
      - Reports the "reported properties" to the device twin (TelemetryInterval and LedStatus).

    Note: From this point, the user can get the twin status updates on the web portal
    (Home > IoT Hub > [IoT Hub Name] - IoT Devices > [Device-ID] > Device twin )
    or by monitoring the device from the Azure CLI thanks to the command
    "az iot hub monitor-events -n <IoTHubName> -d <DeviceID>".

  - Stays idle, pending on local user, or hub-initiated events.

    - Possible local user actions:

      - Single push on the user button:
            Triggers a telemetry message publication to the IoT hub through a DeviceToCloud (D2C) message.
            Once the message is successfully delivered, an acknowledgement is logged to the console:
            "Confirmation received for message with result = IOTHUB_CLIENT_CONFIRMATION_OK"

      - Double push on the user button:
            Starts or stops the telemetry publication loop.
            When the loop is running, the telemetry messages are published every TelemetryInterval seconds.

      Note: The telemetry message consists of a timestamp.
      Note: Each telemetry message publication is signaled by the user LED blinking quickly for half a second.

    - Possible hub-initiated events:

      - CloudToDevice (C2D) message:
          The message is displayed on the board console.

      - C2D twin update:
          Change of the telemetry publication period (DesiredTelemetryInterval).

      - C2D method:
          Call one of the following device methods:
            * Reboot: Reboot the board.
            * Hello: Display the message passed as parameter on the board console.

      - C2D action call:
            * LedToggle: Toggle the user LED state.


    The companion document Middlewares\Third_Party\azure-iot-sdk-c\iothub_client\samples\STM32Cube_sample\ReadmeRefs.htm details:
      - The runtime state flow (Fig. 1);
      - The Azure CLI command lines for the user to trigger hub-initiated events, and monitor the device (Table 1).


@par Device authentication variants

I. X.509 certificates for direct authentication by the IoT hub

  - The IoT Hub portal allows to create a device with an "X.509 CA signed" authentication type.
  - The X.509 certificates may be created by Microsoft tools, by OpenSSL, or by any other method of your choice.
  - The test root CA certificate must then be registered in IoT Hub and activated by running the proof of possession procedure.

  More information in Microsoft documentation:
    - See https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#proof-of-possession-of-your-x509-ca-certificate
    - See https://github.com/Azure/azure-iot-sdk-c/blob/master/tools/CACertificates/CACertificateOverview.md

  If you prefer creating your certificates with OpenSSL rather than with Microsoft tools, here are some command examples.
  Please note that these commands are provided as example for test purpose and should NOT be used to create production-grade
  security artifacts.

  (1) Create the test root CA keys (ecc)
      openssl ecparam -name secp256k1 -genkey -noout -out secp256k1-key.pem

  (2) Create the test root CA associated to this private key
      openssl req -new -x509 -key secp256k1-key.pem -out iotHubRoot.cer -days 500

  (3) Create a verification key to run the proof of possession procedure
      openssl ecparam -name secp256k1 -genkey -noout -out verif-key.pem

  (4) Create a signing request to sign with the test root CA's private key
      openssl req -new -key verif-key.pem -out verification.csr
      Please make sure to indicate the verification code retrieved from the web portal as the "Common Name".

  (5) Generate the .pem file to be uploaded as an answer for the proof of possession procedure.
      openssl x509 -req -in verification.csr -CA iotHubRoot.cer -CAkey secp256k1-key.pem -CAcreateserial -out verification.pem -days 500 -sha256

  (6) Create a device key
      openssl ecparam -name secp256k1 -genkey -noout -out EccDevice.pem

  (7) Create a signing request for the device key
      openssl req -new -key EccDevice.pem -out csr_Device.csr

  (8) Sign with the your test root CA
      openssl x509 -req -days 500 -in csr_Device.csr -CA iotHubRoot.cer -CAkey secp256k1-key.pem -set_serial 01 -out deviceCert.crt
      Please make sure to indicate your device id as the "Common Name".

II. X.509 certificates for using the Device Provisioning Service

  - By contrast to the direct X.509 authentication to the hub, the DPS allows the device to generate
    its own authentication certificate and to get it signed by the "test root CA" which is embedded
    in the HSM emulator of the Azure C SDK.

  - The user must:
    1. Upload this root CA to the DPS service that the device is attached to (NB: The DPS service instance
       is identified by its ID Scope).
    -> The certificate  may be copied from the device console and uploaded as a .pem file.
    2. Activate the root CA through a proof of possession procedure.
    -> This consists in:
      * generating a verification code on the portal,
      * enter it when prompted on the device console,
      * copy the generated verification certificate from the device console, and upload it to the portal as a .pem file.

  - Once done, an enrollment group must be created to accept the devices presenting this "test root CA" in their CA chain
    at TLS connection time.

  More information in Microsoft documentation:
    - See https://docs.microsoft.com/en-us/azure/iot-dps/how-to-manage-enrollments
    - See https://docs.microsoft.com/en-us/azure/iot-dps/how-to-verify-certificates

@par Firmware update
  Notice: The firmware update system is activated on most of the board/toolchain combinations supported
  by the present package. Please see the release notes for possible restrictions.

  The bootloader allows to update the STM32 microcontroller user program to a new version,
  adding new features or correcting potential issues.

  The update process can be performed in a secure way which prevents unauthorized updates or the access to embedded
  confidential data.
   * The Secure Boot (Root of Trust services) activates STM32 security mechanisms and checks the authenticity
     and the integrity of the user application code before every execution to prevent invalid or malicious code from being run.
   * The Secure Firmware Update application is passed the encrypted firmware image, checks its authenticity,
     decrypts it, and checks the integrity of the code before installing it.

  The build scripts of the Azure application also produce an encrypted binary of the user application:
  Projects\<board>\Applications\Cloud\Azure\<toolchain>\Postbuild\<boardname>_Azure.sfb

  This file must be made available on an HTTP(S) server.
    If you use an Azure Blob storage, don't forget to make the file public: "anonymous read access for blobs only".
    If you use another HTTP over TLS server, you will probably need to add the root CA certificate
    when provisioning the board at first launch as described above.


  The sample application implements three ways to start the firmware update:

  1) Enter the URL of the file at startup when prompted by the Azure application running on STM32 device.

  2) Once the device is connected to the hub, call the FirmwareUpdate device method from Azure CLI:
     az iot hub invoke-device-method -n <hubname> -d <DeviceName> --method-name FirmwareUpdate --method-payload "{\"FwPackageUri\": \""https://blob.azure.com/storage/file.sfb\"}"

  The device then downloads the file, reboots and checks its integrity before installing and running the new user application.

  3) Change the properties of your device twin as described below:

  "properties": {
    "desired": {
      "fwVersion": "1.2.0",
      "fwPackageURI": "https://blob.azure.com/storage/file.sfb"
    }
  }

  As soon as the desired property is received by the device, if the desired fwVersion is different
  from the current running version, the sample application exits the MQTT loop and downloads
  the new user application file over HTTP(S). If successful, it reboots to let SBSFU update the firmware, and
  launch the new application.
  Once the device is connected back to Azure IoT Hub, it verifies that its version has
  changed since the update request was received, and updates the "fwUpdateStatus" device twin reported property:
  "Current" if successful, or "Error".



@par Directory contents

---> in .
Binary
  <boardname>_AZURE.sfb                Pre-built secure image of the sample application. To be used in FOTA case.
  SBSFU_<boardname>_AZURE.bin          Pre-built bootloader + sample application image.

Inc
  cmsis_os_misrac2012.h
  flash.h                            Management of the internal flash memory.
  FreeRTOSConfig.h                   FreeRTOS configuration.
  ipc_config.h                       cellular IPC configuration
  main.h                             Configuration parameters for the application and globals.
  mbedtls_config.h                   Static configuration of the mbedTLS library.
  mbedtls_entropy.h                  mbedTLS RNG abstraction interface.
  net_conf.h                         Configuration parameters for the STM32_Connect_Library.
  plf_config.h
  plf_features.h                     Cellular applications configuration (data mode, miscellaneous functionalities)
  plf_hw_config.h                    Cellular HW parameters (such as UART configuration, GPIO used, and others).
  plf_stack_size.h                   System thread stack sizes configuration (cellular and cloud thread)
                                     The stack sizes included in this file are used to calculate the FreeRTOS heap size
                                     (contained in file FreeRTOSConfig.h)
  plf_sw_config.h                    Cellular SW parameters configuration (task priorities, trace activations...)
  prj_config.h                       Cellular project file
  stm32l4xx_hal_conf.h               HAL configuration file.
  stm32l4xx_it.h                     STM32 interrupt handlers header file.
  usart.h                            Usart declaration for cellular

Src
  board_interrupts.c                 Overriding of the native weak HAL interrupt base functions, for cellular usart
  flash_l4.c                         Flash programming interface.
  main.c                             Main application file.
  mbedtls_entropy.c                  mbedTLS RNG abstraction interface.
  net_conf.c                         Bus mapping of the STM32_Connect_Library to the eS_WiFi module.
  set_credentials.c                  Interactive provisionning of the network connection settings.
  stm32l4xx_hal_msp.c                Specific initializations.
  stm32l4xx_hal_timebase_tim.c       Overriding of the native weak HAL time base functions
  stm32l4xx_it.c                     STM32 interrupt handlers.
  system_stm32l4xx.c                 System initialization.
  usart.c                            Usart definition for cellular

---> in Middlewares\Third_Party\azure-iot-sdk-c\certs:
usertrust_baltimore.pem              List of root CA certificates to be pasted on the board console at first launch.
                                     Note: Baltimore is the root CA for the azure-devices.net domain.
                                     If your IoT hub belongs to a different domain name, you may have to use a different CA.

---> in Middlewares\Third_Party\azure-iot-sdk-c\iothub_client\samples\STM32Cube_sample
azure_sha1.c                         Replacement for the Azure SDK sha1.c file (filename conflict with mbedTLS).
azure_version.c                      Replacement for the Azure SDK version.c file (filename conflict with mbedTLS).
azure_version.h                      Application version.
AzureXcubeSample.c                   Application implementation.
ReadmeRefs.htm                       Readme figures and tables.
ReadmeRefs_files\*                   Readme figures and tables.


---> in Middlewares\Third_Party\azure-iot-sdk-c\c-utility\adapters
platform_STM32Cube.c                 Adaptation of the Azure SDK platform interface.
                                       NB: platform_init() and platform_deinit() are provided by cloud.c.
socketio_mbed.c                      Implementation of the Azure SDK socket adapter interface.
                                       NB: Reuse the mbed version from the SDK, with minimal changes so that it is
                                       compatible with the mbedtls tlsio version.
tcpsocketconnection_c_STM32Cube.c    Network port for the mbed socket adapter.
threadapi_STM32Cube.c                Implementation of the Azure SDK thread adapter interface.
                                     No multithread support is currently implemented.
tickcounter_STM32Cube.c              Implementation of the Azure SDK tick counter adapter interface.
tlsio_mbedtls.c                      Implementation of the Azure SDK tlsIO adapter interface.
                                       NB: Overloads the SDK adapter file in order not to include "azure_c_shared_utility\tlsio_mbedtls.h".

---> in Middlewares\Third_Party\azure-iot-sdk-c\provisioning_client\adapters
hsm_client_riot_STM32Cube.c          Implementation of the Azure SDK HSM emulator.
hsm_client_tpm_STM32Cube.c           Stub for the Azure SDK TPM emulator.


---> in Projects\Misc_Utils
Inc
  cloud.h
  iot_flash_config.h
  msg.h                              Log interface
  rfu.h

Src
  cloud.c                            Cloud application init and deinit functions
  iot_flash_config.c                 Dialog and storage management utils for the user-configured settings.
  rfu.c                              Firmware update helpers.


---> in Utilities
CLD_utils/http_lib
  http_lib.c                         Light HTTP client
  http_lib.h

Time
  STM32CubeRTCInterface.c            Libc time porting to the RTC.
  timedate.c                         Initialization of the RTC from the network.
  timingSystem.c                     Libc time porting to the RTC.
  timedate.h
  timingSystem.h

---> in Projects\<boardname>\Applications\BootLoader_OSC

2_Images_SBSFU\*                      Bootloader binary, and security settings (in SBSFU\App\app_sfu.h)
2_Images_SECoreBin\*                  Post-build scripts for the user application.
Linker_Common\*                       Image memory mapping definitions for the user application.


@Par Target-specific notes

@par Caveats

  - The mbedTLS configuration parameter MBEDTLS_SSL_MAX_CONTENT_LEN is tailored down to 6 kbytes.
    It is sufficient for connecting to the Azure IoT cloud, and to the HTTPS server used for retrieving
    the time and date at boot time.
    But the TLS standard may require up to 16 kbytes, depending on the server configuration.
    For instance, if the server certificate is 7 kbytes large, it will not fit in the device 6 kbytes buffer,
    the TLS handshake will fail, and the TLS connection will not be possible.

  - Beware the leading and trailing characters when entering the device connection string on the console. The shared access key ends with '='.


@par Troubleshooting

  - Hardfault during the mbedTLS initialization
    * Check the contents of the root CA configured by the user.

  - mbedTLS handshake failure
    * on mbedtls_pk_sign()
      - Undetected heap overflow.


 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
