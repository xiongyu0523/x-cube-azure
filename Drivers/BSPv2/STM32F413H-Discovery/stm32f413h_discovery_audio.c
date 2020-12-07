/**
  ******************************************************************************
  * @file    stm32f413h_discovery_audio.c
  * @author  MCD Application Team
  * @brief   This file provides the Audio driver for the STM32F413H-DISCO
  *          evaluation board.
  @verbatim
  How To use this driver:
  -----------------------
   + This driver supports STM32F7xx devices on STM32F413H-DISCO (MB1274) Evaluation boards.
   + Call the function BSP_AUDIO_OUT_Init() for AUDIO OUT initialization:
        Instance : Select the output instance. Can only be 0 (I2S)
        AudioInit: Audio Out structure to select the following parameters
                   - Device: Select the output device (headphone, speaker, hdmi ..)
                   - SampleRate: Select the output sample rate (8Khz .. 96Khz)
                   - BitsPerSample: Select the output resolution (16 or 32bits per sample)
                   - ChannelsNbr: Select the output channels number(1 for mono, 2 for stereo)
                   - Volume: Select the output volume(0% .. 100%)

      This function configures all the hardware required for the audio application (codec, I2C, I2S,
      GPIOs, DMA and interrupt if needed). This function returns BSP_ERROR_NONE if configuration is OK.
      If the returned value is different from BSP_ERROR_NONE or the function is stuck then the communication with
      the codec has failed (try to un-plug the power or reset device in this case).

      User can update the I2S or the clock configurations by overriding the weak MX functions MX_I2S2_Init()
      and MX_I2S2_ClockConfig()
      User can override the default MSP configuration and register his own MSP callbacks (defined at application level)
      by calling BSP_AUDIO_OUT_RegisterMspCallbacks() function
      User can restore the default MSP configuration by calling BSP_AUDIO_OUT_RegisterDefaultMspCallbacks()
      To use these two functions, user have to enable USE_HAL_I2S_REGISTER_CALLBACKS within stm32f7xx_hal_conf.h file

   + Call the function BSP_DISCO_AUDIO_OUT_Play() to play audio stream:
        Instance : Select the output instance. Can only be 0 (I2S)
        pBuf: pointer to the audio data file address
        NbrOfBytes: Total size of the buffer to be sent in Bytes

   + Call the function BSP_AUDIO_OUT_Pause() to pause playing
   + Call the function BSP_AUDIO_OUT_Resume() to resume playing.
       Note. After calling BSP_AUDIO_OUT_Pause() function for pause, only BSP_AUDIO_OUT_Resume() should be called
          for resume (it is not allowed to call BSP_AUDIO_OUT_Play() in this case).
       Note. This function should be called only when the audio file is played or paused (not stopped).
   + Call the function BSP_AUDIO_OUT_Stop() to stop playing.
   + Call the function BSP_AUDIO_OUT_Mute() to mute the player.
   + Call the function BSP_AUDIO_OUT_UnMute() to unmute the player.
   + Call the function BSP_AUDIO_OUT_IsMute() to get the mute state(BSP_AUDIO_MUTE_ENABLED or BSP_AUDIO_MUTE_DISABLED).
   + Call the function BSP_AUDIO_OUT_SetDevice() to update the AUDIO OUT device.
   + Call the function BSP_AUDIO_OUT_GetDevice() to get the AUDIO OUT device.
   + Call the function BSP_AUDIO_OUT_SetSampleRate() to update the AUDIO OUT sample rate.
   + Call the function BSP_AUDIO_OUT_GetSampleRate() to get the AUDIO OUT sample rate.
   + Call the function BSP_AUDIO_OUT_SetBitsPerSample() to update the AUDIO OUT resolution.
   + Call the function BSP_AUDIO_OUT_GetBitPerSample() to get the AUDIO OUT resolution.
   + Call the function BSP_AUDIO_OUT_SetChannelsNbr() to update the AUDIO OUT number of channels.
   + Call the function BSP_AUDIO_OUT_GetChannelsNbr() to get the AUDIO OUT number of channels.
   + Call the function BSP_AUDIO_OUT_SetVolume() to update the AUDIO OUT volume.
   + Call the function BSP_AUDIO_OUT_GetVolume() to get the AUDIO OUT volume.
   + Call the function BSP_AUDIO_OUT_GetState() to get the AUDIO OUT state.

   + BSP_AUDIO_OUT_SetDevice(), BSP_AUDIO_OUT_SetSampleRate(), BSP_AUDIO_OUT_SetBitsPerSample() and
     BSP_AUDIO_OUT_SetChannelsNbr() cannot be called while the state is AUDIO_OUT_STATE_PLAYING.
   + For each mode, you may need to implement the relative callback functions into your code.
      The Callback functions are named AUDIO_OUT_XXX_CallBack() and only their prototypes are declared in
      the stm32f413h_discovery_audio.h file. (refer to the example for more details on the callbacks implementations)


   + Call the function BSP_AUDIO_IN_Init() for AUDIO IN initialization:
        Instance : Select the input instance. Can be 0 (I2S) or 1 (DFSDM)
        AudioInit: Audio In structure to select the following parameters
                   - Device: Select the input device (analog, digital mic1, mic2, mic1 & mic2)
                   - SampleRate: Select the input sample rate (8Khz .. 96Khz)
                   - BitsPerSample: Select the input resolution (16 or 32bits per sample)
                   - ChannelsNbr: Select the input channels number(1 for mono, 2 for stereo)
                   - Volume: Select the input volume(0% .. 100%)

      This function configures all the hardware required for the audio application (codec, I2C, I2S, DFSDM
      GPIOs, DMA and interrupt if needed). This function returns BSP_ERROR_NONE if configuration is OK.
      If the returned value is different from BSP_ERROR_NONE or the function is stuck then the communication with
      the codec or the MFX has failed (try to un-plug the power or reset device in this case).

      User can update the DFSDM/I2S or the clock configurations by overriding the weak MX functions MX_I2S2_Init(),
      MX_I2S2_ClockConfig(), MX_DFSDM1_Init() and MX_DFDSM1_ClockConfig()
      User can override the default MSP configuration and register his own MSP callbacks (defined at application level)
      by calling BSP_AUDIO_IN_RegisterMspCallbacks() function
      User can restore the default MSP configuration by calling BSP_AUDIO_IN_RegisterDefaultMspCallbacks()
      To use these two functions, user have to enable USE_HAL_I2S_REGISTER_CALLBACKS and/or USE_HAL_DFSDM_REGISTER_CALLBACKS
      within stm32f7xx_hal_conf.h file

   + Call the function BSP_DISCO_AUDIO_IN_Record() to record audio stream. The recorded data are stored to user buffer in raw
        (L, R, L, R ...)
        Instance : Select the input instance. Can be 0 (I2S) or 1 (DFSDM)
        pBuf: pointer to user buffer
        NbrOfBytes: Total size of the buffer to be sent in Bytes

   + Call the function BSP_AUDIO_IN_Pause() to pause recording
   + Call the function BSP_AUDIO_IN_Resume() to resume recording.
   + Call the function BSP_AUDIO_IN_Stop() to stop recording.
   + Call the function BSP_AUDIO_IN_SetDevice() to update the AUDIO IN device.
   + Call the function BSP_AUDIO_IN_GetDevice() to get the AUDIO IN device.
   + Call the function BSP_AUDIO_IN_SetSampleRate() to update the AUDIO IN sample rate.
   + Call the function BSP_AUDIO_IN_GetSampleRate() to get the AUDIO IN sample rate.
   + Call the function BSP_AUDIO_IN_SetBitPerSample() to update the AUDIO IN resolution.
   + Call the function BSP_AUDIO_IN_GetBitPerSample() to get the AUDIO IN resolution.
   + Call the function BSP_AUDIO_IN_SetChannelsNbr() to update the AUDIO IN number of channels.
   + Call the function BSP_AUDIO_IN_GetChannelsNbr() to get the AUDIO IN number of channels.
   + Call the function BSP_AUDIO_IN_SetVolume() to update the AUDIO IN volume.
   + Call the function BSP_AUDIO_IN_GetVolume() to get the AUDIO IN volume.
   + Call the function BSP_AUDIO_IN_GetState() to get the AUDIO IN state.

   + Call the function BSP_DISCO_AUDIO_IN_RecordChannels() to record audio stream. The recorded data are stored to user buffers separately
        (L, L, ...) (R, R ...). User has to process his data at application level.
        Instance : Select the input instance. Can be 1 (DFSDM)
        pBuf: pointer to user buffers table
        NbrOfBytes: Total size of the buffer to be sent in Bytes
   + Call the function BSP_AUDIO_IN_PauseChannels() to pause recording:
        Instance : Select the input instance. Can be 1 (DFSDM)
        Device: Select the input device (any combination of digital mic1.. mic5)
   + Call the function BSP_AUDIO_IN_ResumeChannels() to resume recording.
        Instance : Select the input instance. Can be 1 (DFSDM)
        Device: Select the input device (any combination of digital mic1.. mic5)
        Note that MIC3, MIC4 and MIC5 are synchronized with MIC2. In case this latter is initialized,
        it should be paused/resumed after resuming one of them to restart correctly DMA transfer.

   + Call the function BSP_AUDIO_IN_StopChannels() to stop recording.
        Instance : Select the input instance. Can be 1 (DFSDM)
        Device: Select the input device (any combination of digital mic1..mic5)

   + For each mode, you may need to implement the relative callback functions into your code.
      The Callback functions are named AUDIO_IN_XXX_CallBack() and only their prototypes are declared in
      the stm32f413h_discovery_audio.h file. (refer to the example for more details on the callbacks implementations)

   + The driver API and the callback functions are at the end of the stm32f413h_discovery_audio.h file.
  Known Limitations:
  ------------------
   1- If the TDM Format used to play in parallel 2 audio Stream (the first Stream is configured in codec SLOT0 and second
      Stream in SLOT1) the Pause/Resume, volume and mute feature will control the both streams.
   2- Parsing of audio file is not implemented (in order to determine audio file properties: Mono/Stereo, Data size,
      File size, Audio Frequency, Audio Data header size ...). The configuration is fixed for the given audio file.

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2018 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f413h_discovery_audio.h"
#include "stm32f413h_discovery_bus.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32F413H_DISCOVERY
  * @{
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO AUDIO
  * @brief This file includes the low layer driver for wm8994 Audio Codec
  *        available on STM32F413H-DISCO board(MB1274).
  * @{
  */
/** @defgroup STM32F413H_DISCOVERY_AUDIO_Private_Defines AUDIO Private Defines
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_Private_Macros AUDIO Private Macros
  * @{
  */
/*### RECORD ###*/
#define DFSDM_OVER_SAMPLING(__FREQUENCY__) \
        ((__FREQUENCY__) == (AUDIO_FREQUENCY_8K))  ? (256U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_11K)) ? (256U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_16K)) ? (128U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_22K)) ? (128U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_32K)) ? (64U)  \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_44K)) ? (64U)  \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_48K)) ? (32U) : (25U)

#define DFSDM_CLOCK_DIVIDER(__FREQUENCY__) \
        ((__FREQUENCY__) == (AUDIO_FREQUENCY_8K))  ? (24U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_11K)) ? (48U)  \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_16K)) ? (24U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_22K)) ? (48U)  \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_32K)) ? (24U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_44K)) ? (48U)  \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_48K)) ? (32U) : (72U)

#define DFSDM_FILTER_ORDER(__FREQUENCY__) \
        ((__FREQUENCY__) == (AUDIO_FREQUENCY_8K))  ? (DFSDM_FILTER_SINC3_ORDER) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_11K)) ? (DFSDM_FILTER_SINC3_ORDER) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_16K)) ? (DFSDM_FILTER_SINC3_ORDER) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_22K)) ? (DFSDM_FILTER_SINC3_ORDER) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_32K)) ? (DFSDM_FILTER_SINC4_ORDER) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_44K)) ? (DFSDM_FILTER_SINC4_ORDER) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_48K)) ? (DFSDM_FILTER_SINC4_ORDER) : (DFSDM_FILTER_SINC4_ORDER)

#define DFSDM_MIC_BIT_SHIFT(__FREQUENCY__) \
        ((__FREQUENCY__) == (AUDIO_FREQUENCY_8K))  ? (5U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_11K)) ? (4U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_16K)) ? (2U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_22K)) ? (2U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_32K)) ? (5U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_44K)) ? (6U) \
      : ((__FREQUENCY__) == (AUDIO_FREQUENCY_48K)) ? (2U) : (0U)

/* Saturate the record PCM sample */
#define SaturaLH(N, L, H) (((N)<(L))?(L):(((N)>(H))?(H):(N)))

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_Exported_Variables AUDIO Exported Variables
  * @{
  */
/* Play */
void                            *Audio_CompObj;
AUDIO_OUT_Ctx_t                 AudioOut_Ctx[AUDIO_OUT_INSTANCES_NBR];
I2S_HandleTypeDef               haudio_out_i2s;

/* Record */
AUDIO_IN_Ctx_t                  AudioIn_Ctx[AUDIO_IN_INSTANCES_NBR];
DFSDM_Channel_HandleTypeDef     haudio_in_dfsdm_channel[DFSDM_MIC_NUMBER];
DFSDM_Filter_HandleTypeDef      haudio_in_dfsdm_filter[DFSDM_MIC_NUMBER];
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_Private_Variables AUDIO Private Variables
  * @{
  */
static AUDIO_Drv_t              *AudioDrv = {NULL};
static DMA_HandleTypeDef        hDmaDfsdm[DFSDM_MIC_NUMBER];
static __IO uint32_t            RecBuffTrigger          = 0;
static __IO uint32_t            RecBuffHalf             = 0;
static int32_t                  MicRecBuff[2][DEFAULT_AUDIO_IN_BUFFER_SIZE];
static __IO uint32_t            MicBuffIndex[DFSDM_MIC_NUMBER];
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_Private_Function_Prototypes AUDIO Private Function Prototypes
  * @{
  */

/* I2S Msp config */
static void  I2S_MspInit(I2S_HandleTypeDef *hi2s);
static void  I2S_MspDeInit(I2S_HandleTypeDef *hi2s);

/* I2S callbacks */
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
static void I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
static void I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
static void I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
static void I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
static void I2S_ErrorCallback(I2S_HandleTypeDef *hi2s);
#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 1) */

/* DFSDM Channel Msp config */
static void DFSDM_ChannelMspInit(DFSDM_Channel_HandleTypeDef *hDfsdmChannel);
static void DFSDM_ChannelMspDeInit(DFSDM_Channel_HandleTypeDef *hDfsdmChannel);

/* DFSDM Filter Msp config */
static void DFSDM_FilterMspInit(DFSDM_Filter_HandleTypeDef *hDfsdmFilter);
static void DFSDM_FilterMspDeInit(DFSDM_Filter_HandleTypeDef *hDfsdmFilter);

/* DFSDM Filter conversion callbacks */
#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
static void DFSDM_FilterRegConvHalfCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter);
static void DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter);
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1) */
#if (USE_AUDIO_CODEC_WM8994 == 1)
static int32_t WM8994_Probe(void);
#endif

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_OUT_Exported_Functions AUDIO_OUT Exported Functions
  * @{
  */
/**
  * @brief  Configures the audio peripherals.
  * @param  Instance   AUDIO OUT Instance. It can only be 0
  * @param  AudioInit  AUDIO OUT init Structure
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_Init(uint32_t Instance, BSP_AUDIO_Init_t* AudioInit)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (AudioInit->BitsPerSample != AUDIO_RESOLUTION_16B)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (AudioOut_Ctx[Instance].State != AUDIO_OUT_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    /* Fill AudioOut_Ctx structure */
    AudioOut_Ctx[Instance].Device         = AudioInit->Device;
    AudioOut_Ctx[Instance].Instance       = Instance;
    AudioOut_Ctx[Instance].SampleRate     = AudioInit->SampleRate;
    AudioOut_Ctx[Instance].BitsPerSample  = AudioInit->BitsPerSample;
    /* This feature is not supported by I2S IP: only steareo mode supported */
    AudioOut_Ctx[Instance].ChannelsNbr    = 2U;
    AudioOut_Ctx[Instance].Volume         = AudioInit->Volume;
    AudioOut_Ctx[Instance].State          = AUDIO_OUT_STATE_RESET;

#if (USE_AUDIO_CODEC_WM8994 == 1)
    if(WM8994_Probe() != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
#endif
    if(ret == BSP_ERROR_NONE)
    {
      /* PLL clock is set depending by the AudioFreq (44.1khz vs 48khz groups) */
      if(MX_I2S2_ClockConfig(&haudio_out_i2s, AudioInit->SampleRate) != HAL_OK)
      {
        ret = BSP_ERROR_CLOCK_FAILURE;
      }
      else
      {
        /* I2S data transfer preparation:
        Prepare the Media to be used for the audio transfer from memory to I2S peripheral */
        haudio_out_i2s.Instance = AUDIO_OUT_I2Sx;

#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
        /* Register the I2S MSP Callbacks */
        if(AudioOut_Ctx[Instance].IsMspCallbacksValid == 0U)
        {
          if(BSP_AUDIO_OUT_RegisterDefaultMspCallbacks(0) != BSP_ERROR_NONE)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }
        }
#else
        I2S_MspInit(&haudio_out_i2s);
#endif /* #if (USE_HAL_I2S_REGISTER_CALLBACKS == 1) */

        if(ret == BSP_ERROR_NONE)
        {
          /* Disable I2S peripheral to allow access to I2S internal registers */
          __HAL_I2S_DISABLE(&haudio_out_i2s);

          /* I2S peripheral initialization: this __weak function can be redefined by the application  */
          if(MX_I2S2_Init(&haudio_out_i2s, AudioInit->SampleRate) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
          /* Register I2S TC, HT and Error callbacks */
          else if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_TX_COMPLETE_CB_ID, I2S_TxCpltCallback) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }
          else if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_TX_HALFCOMPLETE_CB_ID, I2S_TxHalfCpltCallback) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }
          else if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_ERROR_CB_ID, I2S_ErrorCallback) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }

#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 1) */
          else
          {
            /* Enable I2S peripheral to generate MCLK */
            __HAL_I2S_ENABLE(&haudio_out_i2s);

#if (USE_AUDIO_CODEC_WM8994 == 1)

            WM8994_Init_t codec_init;
            uint16_t buffer_fake[16] = {0x00};

            /* Fill codec_init structure */
            codec_init.Resolution   = 0;
            codec_init.Frequency    = AudioInit->SampleRate;
            codec_init.InputDevice  = WM8994_IN_NONE;
            codec_init.OutputDevice = AudioInit->Device;

            /* Convert volume before sending to the codec */
            codec_init.Volume       = VOLUME_OUT_CONVERT(AudioInit->Volume);

            /* Receive fake I2S data in order to generate MCLK needed by WM8994 to set its registers */
            if(HAL_I2S_Transmit_DMA(&haudio_out_i2s, buffer_fake, 16) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }/* Initialize the codec internal registers */
            else if(AudioDrv->Init(Audio_CompObj, &codec_init) < 0)
            {
              ret = BSP_ERROR_COMPONENT_FAILURE;
            }
            else
            {
              /* Stop receiving fake I2S data */
              if(HAL_I2S_DMAStop(&haudio_out_i2s) != HAL_OK)
              {
                ret = BSP_ERROR_PERIPH_FAILURE;
              }
            }
#endif

          }
        }
      }
    }

    if(ret == BSP_ERROR_NONE)
    {
      /* Update BSP AUDIO OUT state */
      AudioOut_Ctx[Instance].State = AUDIO_OUT_STATE_STOP;

    }
  }

  return ret;
}

/**
  * @brief  De-initializes the audio out peripheral.
  * @param  Instance AUDIO OUT Instance. It can only be 0
  * @retval None
  */
int32_t BSP_AUDIO_OUT_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 0)
    I2S_MspDeInit(&haudio_out_i2s);
#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 0) */

    /* Initialize the haudio_out_i2s Instance parameter */
    haudio_out_i2s.Instance = AUDIO_OUT_I2Sx;
    /* Call the Media layer stop function */
    if(AudioDrv->DeInit(Audio_CompObj) < 0)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(HAL_I2S_DeInit(&haudio_out_i2s) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      /* Update BSP AUDIO OUT state */
      AudioOut_Ctx[Instance].State = AUDIO_OUT_STATE_RESET;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  I2S clock Config.
  * @param  hi2s I2S handle
  * @param  SampleRate  Audio frequency used to play the audio stream.
  * @note   This API is called by BSP_AUDIO_OUT_Init() and BSP_AUDIO_OUT_SetFrequency()
  *         Being __weak it can be overwritten by the application
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_I2S2_ClockConfig(I2S_HandleTypeDef *hi2s, uint32_t SampleRate)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);

  RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;
  HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

  /* Set the PLL configuration according to the audio frequency */
  if((SampleRate == AUDIO_FREQUENCY_11K) || (SampleRate == AUDIO_FREQUENCY_22K) || (SampleRate == AUDIO_FREQUENCY_44K))
  {
    /* Configure PLLI2S prescalers */
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 271;
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = 2;
  }
  else /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, AUDIO_FREQUENCY_48K, AUDIO_FREQUENCY_96K */
  {
    /* I2S clock config
    PLLI2S_VCO: VCO_344M
    I2S_CLK(first level) = PLLI2S_VCO/PLLI2SR = 344/7 = 49.142 Mhz
    I2S_CLK_x = I2S_CLK(first level)/PLLI2SDIVR = 49.142/1 = 49.142 Mhz */
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 344;
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = (SampleRate == AUDIO_FREQUENCY_96K) ? 2U : 7U;
  }
  rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_I2S_APB1 | RCC_PERIPHCLK_PLLI2S;
  rcc_ex_clk_init_struct.I2sApb1ClockSelection = RCC_I2SAPB1CLKSOURCE_PLLI2S;
  rcc_ex_clk_init_struct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
  rcc_ex_clk_init_struct.PLLI2S.PLLI2SM = 8;

  return HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
}

/**
  * @brief  Initializes the Audio audio out peripheral (I2S).
  * @param  hi2s I2S handle
  * @param  SampleRate Audio OUT frequency
  * @note   Being __weak it can be overwritten by the application
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_I2S2_Init(I2S_HandleTypeDef* hi2s, uint32_t SampleRate)
{
  /* I2S peripheral configuration */
  hi2s->Init.AudioFreq      = SampleRate;
  hi2s->Init.ClockSource    = I2S_CLOCK_PLL;
  hi2s->Init.CPOL           = I2S_CPOL_LOW;
  hi2s->Init.DataFormat     = I2S_DATAFORMAT_16B;
  hi2s->Init.MCLKOutput     = I2S_MCLKOUTPUT_ENABLE;
  hi2s->Init.Mode           = I2S_MODE_MASTER_TX;
  hi2s->Init.Standard       = I2S_STANDARD_PHILIPS;
  hi2s->Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

  /* Init the I2S */
  return HAL_I2S_Init(hi2s);
}

#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP AUDIO OUT Msp Callbacks
  * @param Instance AUDIO OUT Instance. It can only be 0
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    __HAL_I2S_RESET_HANDLE_STATE(&haudio_out_i2s);

    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_MSPINIT_CB_ID, I2S_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_MSPDEINIT_CB_ID, I2S_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      AudioOut_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP AUDIO OUT Msp Callback registering
  * @param Instance     AUDIO OUT Instance. It can only be 0
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_RegisterMspCallbacks (uint32_t Instance, BSP_AUDIO_OUT_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    __HAL_I2S_RESET_HANDLE_STATE(&haudio_out_i2s);

    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      AudioOut_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Starts playing audio stream from a data buffer for a determined size.
  * @param  Instance      AUDIO OUT Instance. It can only be 0.
  * @param  pData         pointer on data address
  * @param  NbrOfBytes   Size of total samples in bytes
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_Play(uint32_t Instance, uint8_t* pData, uint32_t NbrOfBytes)
{
  int32_t ret = BSP_ERROR_NONE;

  if((Instance >= AUDIO_OUT_INSTANCES_NBR) || (NbrOfBytes > 0xFFFFU))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (AudioOut_Ctx[Instance].State != AUDIO_OUT_STATE_STOP)
  {
    ret = BSP_ERROR_BUSY;
  }
  else if(HAL_I2S_Transmit_DMA(&haudio_out_i2s, (uint16_t*)pData, (uint16_t)(NbrOfBytes /(AudioOut_Ctx[Instance].BitsPerSample/8U))) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(AudioDrv->Play(Audio_CompObj) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO OUT state */
    AudioOut_Ctx[Instance].State = AUDIO_OUT_STATE_PLAYING;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  This function Pauses the audio file stream. In case
  *         of using DMA, the DMA Pause feature is used.
  * @param  Instance: AUDIO OUT Instance. It can only be 0.
  * @note   When calling BSP_AUDIO_OUT_Pause() function for pause, only
  *          BSP_AUDIO_OUT_Resume() function should be called for resume (use of BSP_AUDIO_OUT_Play()
  *          function for resume could lead to unexpected behaviour).
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_Pause(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio out state */
  else if(AudioOut_Ctx[Instance].State != AUDIO_OUT_STATE_PLAYING)
  {
    ret = BSP_ERROR_BUSY;
  }/* Call the Audio Codec Pause/Resume function */
  else if(AudioDrv->Pause(Audio_CompObj) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }/* Call the Media layer pause function */
  else if(HAL_I2S_DMAPause(&haudio_out_i2s) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO OUT state */
    AudioOut_Ctx[Instance].State = AUDIO_OUT_STATE_PAUSE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief   Resumes the audio file stream.
  * @param   Instance  AUDIO OUT Instance. It can only be 0.
  * @note    When calling BSP_AUDIO_OUT_Pause() function for pause, only
  *          BSP_AUDIO_OUT_Resume() function should be called for resume (use of BSP_AUDIO_OUT_Play()
  *          function for resume could lead to unexpected behavior).
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_Resume(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  } /* Check audio out state */
  else if(AudioOut_Ctx[Instance].State != AUDIO_OUT_STATE_PAUSE)
  {
    ret = BSP_ERROR_BUSY;
  } /* Call the Media layer pause/resume function */
  else if(HAL_I2S_DMAResume(&haudio_out_i2s) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(AudioDrv->Resume(Audio_CompObj) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO OUT state */
    AudioOut_Ctx[Instance].State = AUDIO_OUT_STATE_PLAYING;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Stops audio playing and Power down the Audio Codec.
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_Stop(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio out state */
  else if (AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_STOP)
  {
    /* Nothing to do */
  }
  else if ((AudioOut_Ctx[Instance].State != AUDIO_OUT_STATE_PLAYING) &&
           (AudioOut_Ctx[Instance].State != AUDIO_OUT_STATE_PAUSE))
  {
    ret = BSP_ERROR_BUSY;
  }/* Call the Media layer stop function */
  else if(AudioDrv->Stop(Audio_CompObj, CODEC_PDWN_SW) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if(HAL_I2S_DMAStop(&haudio_out_i2s) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO OUT state */
    AudioOut_Ctx[Instance].State = AUDIO_OUT_STATE_STOP;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Controls the current audio volume level.
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @param  Volume    Volume level to be set in percentage from 0% to 100% (0 for
  *         Mute and 100 for Max volume level).
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_SetVolume(uint32_t Instance, uint32_t Volume)
{
  int32_t ret = BSP_ERROR_NONE;

  if ((Instance >= AUDIO_OUT_INSTANCES_NBR) || (Volume > 100U))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Call the codec volume control function with converted volume value */
  else if(AudioDrv->SetVolume(Audio_CompObj, AUDIO_VOLUME_OUTPUT, VOLUME_OUT_CONVERT(Volume)) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Update audio out context */
    if(Volume == 0U)
    {
      /* Update Mute State */
      AudioOut_Ctx[Instance].IsMute = BSP_AUDIO_MUTE_ENABLED;
    }
    else
    {
      /* Update Mute State */
      AudioOut_Ctx[Instance].IsMute = BSP_AUDIO_MUTE_DISABLED;
    }

    AudioOut_Ctx[Instance].Volume = Volume;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the current audio volume level.
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @param  Volume    pointer to volume to be returned
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_GetVolume(uint32_t Instance, uint32_t *Volume)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    *Volume = AudioOut_Ctx[Instance].Volume;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Enables the MUTE
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_Mute(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio out state */
  else if (AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }/* Check audio out mute status */
  else if (AudioOut_Ctx[Instance].IsMute == 1U)
  {
    /* Nothing to do */
  }/* Call the Codec Mute function */
  else if(AudioDrv->SetMute(Audio_CompObj, CODEC_MUTE_ON) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Update Mute State */
    AudioOut_Ctx[Instance].IsMute = BSP_AUDIO_MUTE_ENABLED;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Disables the MUTE mode
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_UnMute(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  /* Check audio out state */
  else if (AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  /* Check audio out mute status */
  else if (AudioOut_Ctx[Instance].IsMute == 0U)
  {
    /* Nothing to do */
  }
  /* Call the audio codec mute function */
  else if (AudioDrv->SetMute(Audio_CompObj, CODEC_MUTE_OFF) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Update audio out mute status */
    AudioOut_Ctx[Instance].IsMute = 0U;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Check whether the MUTE mode is enabled or not
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @param  IsMute    pointer to mute state
  * @retval Mute status
  */
int32_t BSP_AUDIO_OUT_IsMute(uint32_t Instance, uint32_t *IsMute)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    *IsMute = AudioOut_Ctx[Instance].IsMute;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Switch dynamically (while audio file is played) the output target
  *         (speaker or headphone).
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @param  Device    The audio output device
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_SetDevice(uint32_t Instance, uint32_t Device)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Call the Codec output device function */
  else if(AudioDrv->SetOutputMode(Audio_CompObj, Device) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Update AudioOut_Ctx structure */
    AudioOut_Ctx[Instance].Device = Device;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the Output Device
  * @param  Instance  AUDIO OUT Instance. It can only be 0.
  * @param  Device    The audio output device
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_GetDevice(uint32_t Instance, uint32_t *Device)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Get AudioOut_Ctx Device */
    *Device = AudioOut_Ctx[Instance].Device;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Updates the audio frequency.
  * @param  Instance   AUDIO OUT Instance. It can only be 0.
  * @param  SampleRate Audio frequency used to play the audio stream.
  * @note   This API should be called after the BSP_AUDIO_OUT_Init() to adjust the
  *         audio frequency.
  * @note   This API can be only called if the AUDIO OUT state is RESET or
  *         STOP to guarantee a correct I2S behavior
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_SetSampleRate(uint32_t Instance, uint32_t SampleRate)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if((AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_RESET) || (AudioOut_Ctx[Instance].State == AUDIO_OUT_STATE_STOP))
  {
    /* PLL clock is set depending by the AudioFreq (44.1khz vs 48khz groups) */
    if(MX_I2S2_ClockConfig(&haudio_out_i2s, SampleRate) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      if(MX_I2S2_Init(&haudio_out_i2s, SampleRate) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
        /* Enable I2S peripheral to generate MCLK */
        __HAL_I2S_ENABLE(&haudio_out_i2s);

        /* Call the Codec output device function */
        if(AudioDrv->SetFrequency(Audio_CompObj, SampleRate) < 0)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }

#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
        /* Register I2S TC, HT and Error callbacks */
        if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_TX_COMPLETE_CB_ID, I2S_TxCpltCallback) != HAL_OK)
        {
          return BSP_ERROR_PERIPH_FAILURE;
        }
        if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_TX_HALFCOMPLETE_CB_ID, I2S_TxHalfCpltCallback) != HAL_OK)
        {
          return BSP_ERROR_PERIPH_FAILURE;
        }
        if(HAL_I2S_RegisterCallback(&haudio_out_i2s, HAL_I2S_ERROR_CB_ID, I2S_ErrorCallback) != HAL_OK)
        {
          return BSP_ERROR_PERIPH_FAILURE;
        }
#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 1) */

        /* Store new sample rate */
        AudioOut_Ctx[Instance].SampleRate = SampleRate;
      }
    }
  }
  else
  {
    ret = BSP_ERROR_BUSY;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the audio frequency.
  * @param  Instance    AUDIO OUT Instance. It can only be 0.
  * @param  SampleRate  Audio frequency used to play the audio stream.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_GetSampleRate(uint32_t Instance, uint32_t *SampleRate)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *SampleRate = AudioOut_Ctx[Instance].SampleRate;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the audio Resolution.
  * @param  Instance       AUDIO OUT Instance. It can only be 0.
  * @param  BitsPerSample  Audio Resolution used to play the audio stream.
  * @note   This feature is not supported by I2S IP.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_SetBitsPerSample(uint32_t Instance, uint32_t BitsPerSample)
{
  /* Return BSP status */
  return BSP_ERROR_FEATURE_NOT_SUPPORTED;
}

/**
  * @brief  Get the audio Resolution.
  * @param  Instance       AUDIO OUT Instance. It can only be 0.
  * @param  BitsPerSample  Audio Resolution used to play the audio stream.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_GetBitsPerSample(uint32_t Instance, uint32_t *BitsPerSample)
{
  /* Return BSP status */
  return BSP_ERROR_FEATURE_NOT_SUPPORTED;
}

/**
  * @brief  Set the audio Channels number.
  * @param  Instance       AUDIO OUT Instance. It can only be 0.
  * @param  ChannelNbr     Audio Channels number used to play the audio stream.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_SetChannelsNbr(uint32_t Instance, uint32_t ChannelNbr)
{
  /* In I2S, only stereo mode is supported:
     A full frame has to be considered as a Left channel data transmission followed by a Right
     channel data transmission. It is not possible to have a partial frame where only the left
     channel is sent.
 */
  AudioOut_Ctx[Instance].ChannelsNbr = 2;

  /* Return BSP status */
  return BSP_ERROR_FEATURE_NOT_SUPPORTED;
}

/**
  * @brief  Get the audio Channels number.
  * @param  Instance       AUDIO OUT Instance. It can only be 0.
  * @param  ChannelNbr     Audio Channels number used to play the audio stream.
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_GetChannelsNbr(uint32_t Instance, uint32_t *ChannelNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Get the audio Channels number */
    *ChannelNbr = AudioOut_Ctx[Instance].ChannelsNbr;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get Audio Out state
  * @param  Instance  Audio Out instance
  * @param  State     Audio Out state
  * @retval BSP status
  */
int32_t BSP_AUDIO_OUT_GetState(uint32_t Instance, uint32_t *State)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_OUT_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Return audio Output State */
    *State = AudioOut_Ctx[Instance].State;
  }

  return ret;
}

/**
  * @brief  This function handles Audio Out DMA interrupt requests.
  * @param  Instance  Audio Out instance
  * @retval None
  */
void BSP_AUDIO_OUT_DMA_IRQHandler(uint32_t Instance)
{
  HAL_DMA_IRQHandler(haudio_out_i2s.hdmatx);
}

#if (USE_HAL_I2S_REGISTER_CALLBACKS == 0) || !defined(USE_HAL_I2S_REGISTER_CALLBACKS)
/**
  * @brief  Tx Transfer completed callbacks.
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);

  /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32f413h_discovery_audio.h) */
  BSP_AUDIO_OUT_TransferComplete_CallBack(0);
}

/**
  * @brief  Tx Half Transfer completed callbacks.
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);

  /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32f413h_discovery_audio.h) */
  BSP_AUDIO_OUT_HalfTransfer_CallBack(0);
}

/**
  * @brief  I2S error callbacks.
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  if(hi2s->Instance == AUDIO_OUT_I2Sx)
  {
    BSP_AUDIO_OUT_Error_CallBack(0);
  }
  else
  {
    BSP_AUDIO_IN_Error_CallBack(0);
  }
}
#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 0) || !defined(USE_HAL_I2S_REGISTER_CALLBACKS) */

/**
  * @brief  Manages the DMA full Transfer complete event.
  * @param  Instance AUDIO OUT Instance. It can only be 0.
  * @retval None
  */
__weak void BSP_AUDIO_OUT_TransferComplete_CallBack(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
}

/**
  * @brief  Manages the DMA Half Transfer complete event.
  * @param  Instance AUDIO OUT Instance. It can only be 0.
  * @retval None
  */
__weak void BSP_AUDIO_OUT_HalfTransfer_CallBack(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
}

/**
  * @brief  Manages the DMA FIFO error event.
  * @param  Instance AUDIO OUT Instance. It can only be 0.
  * @retval None
  */
__weak void BSP_AUDIO_OUT_Error_CallBack(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);
}
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_OUT_Private_Functions AUDIO_OUT Private Functions
  * @{
  */
#if (USE_HAL_I2S_REGISTER_CALLBACKS == 1)
/**
  * @brief  Tx Transfer completed callbacks.
  * @param  hi2s I2S handle
  * @retval None
  */
static void I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);

  /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32f413h_discovery_audio.h) */
  BSP_AUDIO_OUT_TransferComplete_CallBack(0);
}

/**
  * @brief  Tx Half Transfer completed callbacks.
  * @param  hi2s I2S handle
  * @retval None
  */
static void I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hi2s);

  /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32f413h_discovery_audio.h) */
  BSP_AUDIO_OUT_HalfTransfer_CallBack(0);
}

/**
  * @brief  I2S error callbacks.
  * @param  hi2s I2S handle
  * @retval None
  */
static void I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  if(hi2s->Instance == AUDIO_OUT_I2Sx)
  {
  BSP_AUDIO_OUT_Error_CallBack(0);
  }
  else
  {
  BSP_AUDIO_IN_Error_CallBack(0);
  }
}
#endif /* (USE_HAL_I2S_REGISTER_CALLBACKS == 1) */

#if (USE_AUDIO_CODEC_WM8994 == 1)
/**
  * @brief  Register Bus IOs if component ID is OK
  * @retval error status
  */
static int32_t WM8994_Probe(void)
{
  int32_t ret = BSP_ERROR_NONE;
  WM8994_IO_t              IOCtx;
  static WM8994_Object_t   WM8994Obj;
  uint32_t id;

  /* Configure the audio driver */
  IOCtx.Address     = AUDIO_I2C_ADDRESS;
  IOCtx.Init        = BSP_FMPI2C1_Init;
  IOCtx.DeInit      = BSP_FMPI2C1_DeInit;
  IOCtx.ReadReg     = BSP_FMPI2C1_ReadReg16;
  IOCtx.WriteReg    = BSP_FMPI2C1_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if(WM8994_RegisterBusIO (&WM8994Obj, &IOCtx) != WM8994_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }/* Reset the codec */
  else if(WM8994_Reset(&WM8994Obj) != WM8994_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if(WM8994_ReadID(&WM8994Obj, &id) != WM8994_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if(id != WM8994_ID)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    AudioDrv = (AUDIO_Drv_t *) &WM8994_Driver;
    Audio_CompObj = &WM8994Obj;
  }

  return ret;
}
#endif

/**
  * @brief  Initialize BSP_AUDIO_OUT MSP.
  * @param  hi2s  I2S handle
  * @retval None
  */
static void I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
  static DMA_HandleTypeDef hdma_i2s_tx;
  GPIO_InitTypeDef  gpio_init_structure;

  /* Enable I2S clock */
  AUDIO_OUT_I2Sx_CLK_ENABLE();

  /* Enable MCK, SCK, WS, SD and CODEC_INT GPIO clock */
  AUDIO_OUT_I2Sx_MCK_GPIO_CLK_ENABLE();
  AUDIO_OUT_I2Sx_SCK_GPIO_CLK_ENABLE();
  AUDIO_OUT_I2Sx_SD_GPIO_CLK_ENABLE();
  AUDIO_OUT_I2Sx_WS_GPIO_CLK_ENABLE();

  /* CODEC_I2S pins configuration: MCK, SCK, WS and SD pins */
  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_MCK_PIN;
  gpio_init_structure.Mode = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull = GPIO_PULLDOWN;
  gpio_init_structure.Speed = GPIO_SPEED_FAST;
  gpio_init_structure.Alternate = AUDIO_OUT_I2Sx_MCK_AF;
  HAL_GPIO_Init(AUDIO_OUT_I2Sx_MCK_GPIO_PORT, &gpio_init_structure);

  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_SCK_PIN;
  gpio_init_structure.Alternate = AUDIO_OUT_I2Sx_SCK_AF;
  HAL_GPIO_Init(AUDIO_OUT_I2Sx_SCK_GPIO_PORT, &gpio_init_structure);

  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_WS_PIN;
  gpio_init_structure.Pull = GPIO_PULLUP;
  gpio_init_structure.Alternate = AUDIO_OUT_I2Sx_WS_AF;
  HAL_GPIO_Init(AUDIO_OUT_I2Sx_WS_GPIO_PORT, &gpio_init_structure);

  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_SD_PIN;
  gpio_init_structure.Alternate = AUDIO_OUT_I2Sx_SD_AF;
  HAL_GPIO_Init(AUDIO_OUT_I2Sx_SD_GPIO_PORT, &gpio_init_structure);

  /* Enable the DMA clock */
  AUDIO_OUT_I2Sx_DMAx_CLK_ENABLE();

  /* Configure the hdma_i2s_tx handle parameters */
  hdma_i2s_tx.Init.Channel             = AUDIO_OUT_I2Sx_DMAx_CHANNEL;
  hdma_i2s_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_i2s_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_i2s_tx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_i2s_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_i2s_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  hdma_i2s_tx.Init.Mode                = DMA_CIRCULAR;
  hdma_i2s_tx.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_i2s_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  hdma_i2s_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  hdma_i2s_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
  hdma_i2s_tx.Init.PeriphBurst         = DMA_MBURST_SINGLE;

  hdma_i2s_tx.Instance = AUDIO_OUT_I2Sx_DMAx_STREAM;

  /* Associate the DMA handle */
  __HAL_LINKDMA(hi2s, hdmatx, hdma_i2s_tx);

  /* Deinitialize the Stream for new transfer */
  (void)HAL_DMA_DeInit(&hdma_i2s_tx);

  /* Configure the DMA Stream */
  (void)HAL_DMA_Init(&hdma_i2s_tx);

  /* I2S DMA IRQ Channel configuration */
  HAL_NVIC_SetPriority(AUDIO_OUT_I2Sx_DMAx_IRQ, BSP_AUDIO_OUT_IT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(AUDIO_OUT_I2Sx_DMAx_IRQ);

  /* Enable and set I2Sx Interrupt to a lower priority */
  HAL_NVIC_SetPriority(SPI2_IRQn, BSP_AUDIO_OUT_IT_PRIORITY, 0x00);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

/**
  * @brief  Deinitializes I2S MSP.
  * @param  hi2s  I2S handle
  * @retval HAL status
  */
static void I2S_MspDeInit(I2S_HandleTypeDef *hi2s)
{
  GPIO_InitTypeDef  gpio_init_structure;

  /* I2S DMA IRQ Channel deactivation */
  HAL_NVIC_DisableIRQ(AUDIO_OUT_I2Sx_DMAx_IRQ);

  /* Deinitialize the DMA stream */
  (void)HAL_DMA_DeInit(hi2s->hdmatx);

  /* Deactivates CODEC_I2S pins WS, SCK, MCK and SD by putting them in input mode */
  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_MCK_PIN;
  HAL_GPIO_DeInit(AUDIO_OUT_I2Sx_MCK_GPIO_PORT, gpio_init_structure.Pin);

  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_SCK_PIN;
  HAL_GPIO_DeInit(AUDIO_OUT_I2Sx_SCK_GPIO_PORT, gpio_init_structure.Pin);

  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_WS_PIN;
  HAL_GPIO_DeInit(AUDIO_OUT_I2Sx_WS_GPIO_PORT, gpio_init_structure.Pin);

  gpio_init_structure.Pin = AUDIO_OUT_I2Sx_SD_PIN;
  HAL_GPIO_DeInit(AUDIO_OUT_I2Sx_SD_GPIO_PORT, gpio_init_structure.Pin);

  /* Disable I2S clock */
  AUDIO_OUT_I2Sx_CLK_DISABLE();
}

/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_IN_Exported_Functions AUDIO_IN Exported Functions
  * @{
  */

/**
  * @brief  Initialize wave recording.
  * @param  Instance  AUDIO IN Instance. It can be only 0
  * @param  AudioInit Init structure
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_Init(uint32_t Instance, BSP_AUDIO_Init_t* AudioInit)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t i;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Store the audio record context */
    AudioIn_Ctx[Instance].Device          = AudioInit->Device;
    AudioIn_Ctx[Instance].ChannelsNbr     = AudioInit->ChannelsNbr;
    AudioIn_Ctx[Instance].SampleRate      = AudioInit->SampleRate;
    AudioIn_Ctx[Instance].BitsPerSample   = AudioInit->BitsPerSample;
    AudioIn_Ctx[Instance].Volume          = AudioInit->Volume;
    AudioIn_Ctx[Instance].State           = AUDIO_IN_STATE_RESET;

    DFSDM_Filter_TypeDef* FilterInstnace[DFSDM_MIC_NUMBER] = {AUDIO_DFSDMx_MIC1_FILTER, AUDIO_DFSDMx_MIC2_FILTER, AUDIO_DFSDMx_MIC3_FILTER, AUDIO_DFSDMx_MIC4_FILTER, AUDIO_DFSDMx_MIC5_FILTER};
    DFSDM_Channel_TypeDef* ChannelInstance[DFSDM_MIC_NUMBER] = {AUDIO_DFSDMx_MIC1_CHANNEL, AUDIO_DFSDMx_MIC2_CHANNEL, AUDIO_DFSDMx_MIC3_CHANNEL, AUDIO_DFSDMx_MIC4_CHANNEL, AUDIO_DFSDMx_MIC5_CHANNEL};
    uint32_t DigitalMicPins[DFSDM_MIC_NUMBER] = {DFSDM_CHANNEL_SAME_CHANNEL_PINS, DFSDM_CHANNEL_SAME_CHANNEL_PINS, DFSDM_CHANNEL_FOLLOWING_CHANNEL_PINS, DFSDM_CHANNEL_SAME_CHANNEL_PINS, DFSDM_CHANNEL_FOLLOWING_CHANNEL_PINS};
    uint32_t DigitalMicType[DFSDM_MIC_NUMBER] = {DFSDM_CHANNEL_SPI_RISING, DFSDM_CHANNEL_SPI_RISING, DFSDM_CHANNEL_SPI_FALLING, DFSDM_CHANNEL_SPI_RISING, DFSDM_CHANNEL_SPI_FALLING};
    uint32_t Channel4Filter[DFSDM_MIC_NUMBER] = {AUDIO_DFSDMx_MIC1_CHANNEL_FOR_FILTER, AUDIO_DFSDMx_MIC2_CHANNEL_FOR_FILTER, AUDIO_DFSDMx_MIC3_CHANNEL_FOR_FILTER, AUDIO_DFSDMx_MIC4_CHANNEL_FOR_FILTER, AUDIO_DFSDMx_MIC5_CHANNEL_FOR_FILTER};
    MX_DFSDM_Config dfsdm_config;

    /* PLL clock is set depending on the AudioFreq (44.1khz vs 48khz groups) */
    if(MX_DFDSM1_ClockConfig(&haudio_in_dfsdm_channel[0], AudioInit->SampleRate) != HAL_OK)
    {
      ret = BSP_ERROR_CLOCK_FAILURE;
    }
    else
    {
#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
      /* Register the default DFSDM MSP callbacks */
      if(AudioIn_Ctx[0].IsMspCallbacksValid == 0U)
      {
        if(BSP_AUDIO_IN_RegisterDefaultMspCallbacks(1) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
      }
#else
      DFSDM_FilterMspInit(&haudio_in_dfsdm_filter[1]);
      DFSDM_ChannelMspInit(&haudio_in_dfsdm_channel[1]);
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1) */
      if(ret == BSP_ERROR_NONE)
      {
        for(i = 0; i < DFSDM_MIC_NUMBER; i ++)
        {
          dfsdm_config.FilterInstance  = FilterInstnace[i];
          dfsdm_config.ChannelInstance = ChannelInstance[i];
          dfsdm_config.DigitalMicPins  = DigitalMicPins[i];
          dfsdm_config.DigitalMicType  = DigitalMicType[i];
          dfsdm_config.Channel4Filter  = Channel4Filter[i];
          dfsdm_config.RegularTrigger  = DFSDM_FILTER_SW_TRIGGER;
          if((i >= 2U) && ((AudioIn_Ctx[Instance].Device & AUDIO_IN_DEVICE_DIGITAL_MIC2) == AUDIO_IN_DEVICE_DIGITAL_MIC2))
          {
            dfsdm_config.RegularTrigger = DFSDM_FILTER_SYNC_TRIGGER;
          }
          dfsdm_config.SincOrder       = DFSDM_FILTER_ORDER(AudioIn_Ctx[Instance].SampleRate);
          dfsdm_config.Oversampling    = DFSDM_OVER_SAMPLING(AudioIn_Ctx[Instance].SampleRate);
          dfsdm_config.ClockDivider    = DFSDM_CLOCK_DIVIDER(AudioIn_Ctx[Instance].SampleRate);
          dfsdm_config.RightBitShift   = DFSDM_MIC_BIT_SHIFT(AudioIn_Ctx[Instance].SampleRate);

          if(((AudioInit->Device >> i) & AUDIO_IN_DEVICE_DIGITAL_MIC1) == AUDIO_IN_DEVICE_DIGITAL_MIC1)
          {
            /* Default configuration of DFSDM filters and channels */
            if(MX_DFSDM1_Init(&haudio_in_dfsdm_filter[i], &haudio_in_dfsdm_channel[i], &dfsdm_config) != HAL_OK)
            {
              /* Return BSP_ERROR_PERIPH_FAILURE when operations are not correctly done */
              ret = BSP_ERROR_PERIPH_FAILURE;
            }

#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
            /* Register filter regular conversion callbacks */
            else if(HAL_DFSDM_FILTER_RegisterCallback(&haudio_in_dfsdm_filter[i], HAL_DFSDM_FILTER_REG_COMPLETE_CB_ID, DFSDM_FilterRegConvCpltCallback) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }
            else
            {
              if(HAL_DFSDM_FILTER_RegisterCallback(&haudio_in_dfsdm_filter[i], HAL_DFSDM_FILTER_REG_HALFCOMPLETE_CB_ID, DFSDM_FilterRegConvHalfCpltCallback) != HAL_OK)
              {
                ret = BSP_ERROR_PERIPH_FAILURE;
              }
            }
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1) */
          }
        }
      }
    }
  }
  if(ret == BSP_ERROR_NONE)
  {
    /* Update BSP AUDIO IN state */
    AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_STOP;
  }

  return ret;
}

/**
  * @brief  Deinit the audio IN peripherals.
  * @param  Instance AUDIO IN Instance. It can be only 0
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t i;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    for(i = 0U; i < DFSDM_MIC_NUMBER; i ++)
    {
      if(HAL_DFSDM_FilterDeInit(&haudio_in_dfsdm_filter[i]) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
        if(HAL_DFSDM_ChannelDeInit(&haudio_in_dfsdm_channel[i]) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
      }
#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 0)
      DFSDM_FilterMspDeInit(&haudio_in_dfsdm_filter[i]);
      DFSDM_ChannelMspDeInit(&haudio_in_dfsdm_channel[i]);
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 0) */
    }

    /* Reset AudioIn_Ctx[0].IsMultiBuff if any */
    AudioIn_Ctx[Instance].IsMultiBuff = 0;

    /* Update BSP AUDIO IN state */
    AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_RESET;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Clock Config.
  * @param  hDfsdmChannel  DFSDM Channel Handle
  * @param  SampleRate     Audio frequency to be configured for the DFSDM Channel.
  * @note   This API is called by BSP_AUDIO_IN_Init()
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_DFDSM1_ClockConfig(DFSDM_Channel_HandleTypeDef *hDfsdmChannel, uint32_t SampleRate)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hDfsdmChannel);

  RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;
  HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

  /* Set the PLL configuration according to the audio frequency */
  rcc_ex_clk_init_struct.PeriphClockSelection = (RCC_PERIPHCLK_I2S_APB1 | RCC_PERIPHCLK_DFSDM | RCC_PERIPHCLK_DFSDM2);
  rcc_ex_clk_init_struct.I2sApb1ClockSelection = RCC_I2SAPB1CLKSOURCE_PLLI2S;
  rcc_ex_clk_init_struct.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_APB2;
  rcc_ex_clk_init_struct.Dfsdm2ClockSelection = RCC_DFSDM2CLKSOURCE_APB2;
  rcc_ex_clk_init_struct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
  rcc_ex_clk_init_struct.PLLI2S.PLLI2SM = 8;

  if((SampleRate == AUDIO_FREQUENCY_11K) || (SampleRate == AUDIO_FREQUENCY_22K) || (SampleRate == AUDIO_FREQUENCY_44K))
  {
    /* Configure PLLI2S prescalers */
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 271;
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = 2;
  }
  else /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, AUDIO_FREQUENCY_32K, AUDIO_FREQUENCY_48K, AUDIO_FREQUENCY_96K */
  {
    /* I2S clock config
    PLLI2S_VCO: VCO_344M
    I2S_CLK(first level) = PLLI2S_VCO/PLLI2SR = 344/7 = 49.142 Mhz
    I2S_CLK_x = I2S_CLK(first level)/PLLI2SDIVR = 49.142/1 = 49.142 Mhz */
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 344;
    rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = (SampleRate == AUDIO_FREQUENCY_96K) ? 2U : 7U;
  }

  /* I2S_APB1 selected as DFSDM audio clock source */
  __HAL_RCC_DFSDM1AUDIO_CONFIG(RCC_DFSDM1AUDIOCLKSOURCE_I2SAPB1);
  /* I2S_APB1 selected as DFSDM audio clock source */
  __HAL_RCC_DFSDM2AUDIO_CONFIG(RCC_DFSDM2AUDIOCLKSOURCE_I2SAPB1);

  return HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
}

/**
  * @brief  Initializes the Audio instance (DFSDM).
  * @param  hDfsdmFilter  DFSDM Filter Handle
  * @param  hDfsdmChannel DFSDM Channel Handle
  * @param  MXConfig      DFSDM configuration structure.
  * @note   Being __weak it can be overwritten by the application
  * @note   Channel output Clock Divider and Filter Oversampling are calculated as follow:
  *         - Clock_Divider = CLK(input DFSDM)/CLK(micro) with
  *           1MHZ < CLK(micro) < 3.2MHZ (TYP 2.4MHZ for MP34DT01TR)
  *         - Oversampling = CLK(input DFSDM)/(Clock_Divider * AudioFreq)
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_DFSDM1_Init(DFSDM_Filter_HandleTypeDef *hDfsdmFilter, DFSDM_Channel_HandleTypeDef *hDfsdmChannel, MX_DFSDM_Config *MXConfig)
{
  HAL_StatusTypeDef ret = HAL_OK;

  /* MIC channels initialization */
  hDfsdmChannel->Instance                      = MXConfig->ChannelInstance;
  hDfsdmChannel->Init.OutputClock.Activation   = ENABLE;
  hDfsdmChannel->Init.OutputClock.Selection    = DFSDM_CHANNEL_OUTPUT_CLOCK_AUDIO;
  hDfsdmChannel->Init.OutputClock.Divider      = MXConfig->ClockDivider;
  hDfsdmChannel->Init.Input.Multiplexer        = DFSDM_CHANNEL_EXTERNAL_INPUTS;
  hDfsdmChannel->Init.Input.DataPacking        = DFSDM_CHANNEL_STANDARD_MODE;
  hDfsdmChannel->Init.SerialInterface.SpiClock = DFSDM_CHANNEL_SPI_CLOCK_INTERNAL;
  hDfsdmChannel->Init.Awd.FilterOrder          = DFSDM_CHANNEL_SINC1_ORDER;
  hDfsdmChannel->Init.Awd.Oversampling         = 10;
  hDfsdmChannel->Init.Offset                   = 0;
  hDfsdmChannel->Init.RightBitShift            = MXConfig->RightBitShift;
  hDfsdmChannel->Init.Input.Pins               = MXConfig->DigitalMicPins;
  hDfsdmChannel->Init.SerialInterface.Type     = MXConfig->DigitalMicType;

  if(HAL_OK != HAL_DFSDM_ChannelInit(hDfsdmChannel))
  {
    ret = HAL_ERROR;
  }
  else
  {
    /* MIC filters  initialization */
    hDfsdmFilter->Instance                          = MXConfig->FilterInstance;
    hDfsdmFilter->Init.RegularParam.Trigger         = MXConfig->RegularTrigger;
    hDfsdmFilter->Init.RegularParam.FastMode        = ENABLE;
    hDfsdmFilter->Init.RegularParam.DmaMode         = ENABLE;
    hDfsdmFilter->Init.InjectedParam.Trigger        = DFSDM_FILTER_SW_TRIGGER;
    hDfsdmFilter->Init.InjectedParam.ScanMode       = DISABLE;
    hDfsdmFilter->Init.InjectedParam.DmaMode        = DISABLE;
    hDfsdmFilter->Init.InjectedParam.ExtTrigger     = DFSDM_FILTER_EXT_TRIG_TIM8_TRGO;
    hDfsdmFilter->Init.InjectedParam.ExtTriggerEdge = DFSDM_FILTER_EXT_TRIG_BOTH_EDGES;
    hDfsdmFilter->Init.FilterParam.SincOrder        = MXConfig->SincOrder;
    hDfsdmFilter->Init.FilterParam.Oversampling     = MXConfig->Oversampling;
    hDfsdmFilter->Init.FilterParam.IntOversampling  = 1;

    if(HAL_DFSDM_FilterInit(hDfsdmFilter) != HAL_OK)
    {
      ret = HAL_ERROR;
    }
    else
    {
      /* Configure injected channel */
      if(HAL_DFSDM_FilterConfigRegChannel(hDfsdmFilter, MXConfig->Channel4Filter, DFSDM_CONTINUOUS_CONV_ON) != HAL_OK)
      {
        ret = HAL_ERROR;
      }
    }
  }

  return ret;
}

#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP AUDIO IN Msp Callbacks
  * @param Instance BSP AUDIO IN Instance
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t i;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    for(i = 0; i < DFSDM_MIC_NUMBER; i ++)
    {
      if(((AudioIn_Ctx[Instance].Device >> i) & AUDIO_IN_DEVICE_DIGITAL_MIC1) == AUDIO_IN_DEVICE_DIGITAL_MIC1)
      {
        __HAL_DFSDM_CHANNEL_RESET_HANDLE_STATE(&haudio_in_dfsdm_channel[i]);
        __HAL_DFSDM_FILTER_RESET_HANDLE_STATE(&haudio_in_dfsdm_filter[i]);

#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
        /* Register MspInit/MspDeInit Callbacks */
        if(HAL_DFSDM_CHANNEL_RegisterCallback(&haudio_in_dfsdm_channel[i], HAL_DFSDM_CHANNEL_MSPINIT_CB_ID, DFSDM_ChannelMspInit) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else if(HAL_DFSDM_FILTER_RegisterCallback(&haudio_in_dfsdm_filter[i], HAL_DFSDM_FILTER_MSPINIT_CB_ID, DFSDM_FilterMspInit) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else if(HAL_DFSDM_CHANNEL_RegisterCallback(&haudio_in_dfsdm_channel[i], HAL_DFSDM_CHANNEL_MSPDEINIT_CB_ID, DFSDM_ChannelMspDeInit) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else
        {
          if(HAL_DFSDM_FILTER_RegisterCallback(&haudio_in_dfsdm_filter[i], HAL_DFSDM_FILTER_MSPDEINIT_CB_ID, DFSDM_FilterMspDeInit) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
          }
        }
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)  */
      }
    }
    if(ret == BSP_ERROR_NONE)
    {
      AudioIn_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }


  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP AUDIO In Filter Msp Callback registering
  * @param Instance    AUDIO IN Instance
  * @param CallBacks   pointer to filter MspInit/MspDeInit functions
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_RegisterMspCallbacks (uint32_t Instance, BSP_AUDIO_IN_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
    uint32_t i;
    for(i = 0U; i < DFSDM_MIC_NUMBER; i ++)
    {
      __HAL_DFSDM_FILTER_RESET_HANDLE_STATE(&haudio_in_dfsdm_filter[i]);

      /* Register MspInit/MspDeInit Callback */
      if(HAL_DFSDM_FILTER_RegisterCallback(&haudio_in_dfsdm_filter[i], HAL_DFSDM_FILTER_MSPINIT_CB_ID, CallBacks->pMspFltrInitCb) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(HAL_DFSDM_FILTER_RegisterCallback(&haudio_in_dfsdm_filter[i], HAL_DFSDM_FILTER_MSPDEINIT_CB_ID, CallBacks->pMspFltrDeInitCb) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if(HAL_DFSDM_CHANNEL_RegisterCallback(&haudio_in_dfsdm_channel[i], HAL_DFSDM_CHANNEL_MSPINIT_CB_ID, CallBacks->pMspChInitCb) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
        if(HAL_DFSDM_CHANNEL_RegisterCallback(&haudio_in_dfsdm_channel[i], HAL_DFSDM_CHANNEL_MSPDEINIT_CB_ID, CallBacks->pMspChDeInitCb) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
      }
    }
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1) */

    if(ret == BSP_ERROR_NONE)
    {
      AudioIn_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }

  /* Return BSP status */
  return ret;
}

#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Start audio recording.
  * @param  Instance AUDIO IN Instance. It can be only 0
  * @param  pBuf     Main buffer pointer for the recorded data storing
  * @param  NbrOfBytes Size of the record buffer
  * @note   The NbrOfBytes should be a multiple of twice DEFAULT_AUDIO_IN_BUFFER_SIZE
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_Record(uint32_t Instance, uint8_t* pBuf, uint32_t NbrOfBytes)
{
  int32_t ret = BSP_ERROR_NONE;

  if ((Instance >= AUDIO_IN_INSTANCES_NBR) || (NbrOfBytes > 0xFFFFU))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    AudioIn_Ctx[Instance].pBuff = (uint16_t*)pBuf;
    AudioIn_Ctx[Instance].Size  = NbrOfBytes;
    /* Reset Buffer Trigger */
    RecBuffTrigger = 0;
    RecBuffHalf = 0;

    if(NbrOfBytes % (2*DEFAULT_AUDIO_IN_BUFFER_SIZE) != 0)
    {
      ret = BSP_ERROR_WRONG_PARAM;
    }/* Call the Media layer start function for MIC2 channel */
    else if(HAL_DFSDM_FilterRegularStart_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)],\
      (int32_t*)MicRecBuff[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)], DEFAULT_AUDIO_IN_BUFFER_SIZE) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_DFSDM_FilterRegularStart_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)],\
      (int32_t*)MicRecBuff[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)], DEFAULT_AUDIO_IN_BUFFER_SIZE) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      /* Update BSP AUDIO IN state */
      AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_RECORDING;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Stop audio recording.
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_Stop(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State == AUDIO_IN_STATE_STOP)
  {
    /* Nothing to do */
  }
  else if ((AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_RECORDING) &&
           (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_PAUSE))
  {
    ret = BSP_ERROR_BUSY;
  }/* Call the Media layer stop function */
  else if(HAL_DFSDM_FilterRegularStop_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)]) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(HAL_DFSDM_FilterRegularStop_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)]) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO IN state */
    AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_STOP;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Pause the audio file stream.
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_Pause(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_RECORDING)
  {
    ret = BSP_ERROR_BUSY;
  }/* Call the Media layer stop function */
  else if(HAL_DFSDM_FilterRegularStop_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)]) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(HAL_DFSDM_FilterRegularStop_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)]) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO IN state */
    AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_PAUSE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Resume the audio file stream.
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_Resume(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_PAUSE)
  {
    ret = BSP_ERROR_BUSY;
  }/* Call the Media layer start function for MIC2/MIC1 channel */
  else if(HAL_DFSDM_FilterRegularStart_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)],\
    (int32_t*)MicRecBuff[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)], DEFAULT_AUDIO_IN_BUFFER_SIZE) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if(HAL_DFSDM_FilterRegularStart_DMA(&haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)],\
    (int32_t*)MicRecBuff[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)], DEFAULT_AUDIO_IN_BUFFER_SIZE) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* Update BSP AUDIO IN state */
    AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_RECORDING;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Starts audio recording.
  * @param  Instance   AUDIO IN Instance. It can be 1 (DFSDM used)
  * @param  pBuf       Main buffer pointer for the recorded data storing
  * @param  NbrOfBytes Size of the recorded buffer
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_RecordChannels(uint32_t Instance, uint8_t **pBuf, uint32_t NbrOfBytes)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t mic_init[DFSDM_MIC_NUMBER] = {0};
  uint32_t audio_in_digital_mic = AUDIO_IN_DEVICE_DIGITAL_MIC1, pbuf_index = 0;
  uint32_t enabled_mic = 0;
  uint16_t i;

  if((Instance >= AUDIO_IN_INSTANCES_NBR) || (NbrOfBytes > 0xFFFFU))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Get the number of activated microphones */
    for(i = 0U; i < DFSDM_MIC_NUMBER; i++)
    {
      if((AudioIn_Ctx[Instance].Device & audio_in_digital_mic) == audio_in_digital_mic)
      {
        enabled_mic++;
      }
      audio_in_digital_mic = audio_in_digital_mic << 1;
    }

    AudioIn_Ctx[Instance].pMultiBuff = pBuf;
    AudioIn_Ctx[Instance].Size  = NbrOfBytes;
    AudioIn_Ctx[Instance].IsMultiBuff = 1U;

    audio_in_digital_mic = AUDIO_IN_DEVICE_DIGITAL_MIC_LAST;
    for(i = 0U; i < DFSDM_MIC_NUMBER; i++)
    {
      if(((AudioIn_Ctx[Instance].Device & audio_in_digital_mic) == audio_in_digital_mic) && (mic_init[POS_VAL(audio_in_digital_mic)] != 1U))
      {
        /* Call the Media layer start function for MICx channel */
        if(HAL_DFSDM_FilterRegularStart_DMA(&haudio_in_dfsdm_filter[POS_VAL(audio_in_digital_mic)], (int32_t*)pBuf[enabled_mic - 1U - pbuf_index], NbrOfBytes) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else
        {
          MicBuffIndex[POS_VAL(audio_in_digital_mic)] = enabled_mic - 1U - pbuf_index;
          mic_init[POS_VAL(audio_in_digital_mic)] = 1U;
          pbuf_index++;
        }
      }
      audio_in_digital_mic = audio_in_digital_mic >> 1;
    }

    if(ret == BSP_ERROR_NONE)
    {
      /* Update BSP AUDIO IN state */
      AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_RECORDING;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Stop audio recording.
  * @param  Instance  AUDIO IN Instance. It can be 1 (DFSDM used)
  * @param  Device    Digital input device to be stopped
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_StopChannels(uint32_t Instance, uint32_t Device)
{
  int32_t ret = BSP_ERROR_NONE;

  if((Instance >= AUDIO_IN_INSTANCES_NBR)  || ((Device < AUDIO_IN_DEVICE_DIGITAL_MIC1) && (Device > AUDIO_IN_DEVICE_DIGITAL_MIC_LAST)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Stop selected devices */
    if(BSP_AUDIO_IN_PauseChannels(Instance, Device) == BSP_ERROR_NONE)
    {
      /* Update BSP AUDIO IN state */
      AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_STOP;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Pause the audio file stream.
  * @param  Instance  AUDIO IN Instance. It can be 1 (DFSDM is used)
  * @param  Device    Digital mic to be paused
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_PauseChannels(uint32_t Instance, uint32_t Device)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t audio_in_digital_mic = AUDIO_IN_DEVICE_DIGITAL_MIC1;
  uint32_t i;

  if((Instance >= AUDIO_IN_INSTANCES_NBR)  || ((Device < AUDIO_IN_DEVICE_DIGITAL_MIC1) && (Device > AUDIO_IN_DEVICE_DIGITAL_MIC_LAST)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_RECORDING)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    for(i = 0; i < DFSDM_MIC_NUMBER; i++)
    {
      if((Device & audio_in_digital_mic) == audio_in_digital_mic)
      {
        /* Call the Media layer stop function */
        if(HAL_DFSDM_FilterRegularStop_DMA(&haudio_in_dfsdm_filter[POS_VAL(audio_in_digital_mic)]) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
      }
      audio_in_digital_mic = audio_in_digital_mic << 1;
    }
    if(ret == BSP_ERROR_NONE)
    {
      /* Update BSP AUDIO IN state */
      AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_PAUSE;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Resume the audio file stream.
  * @param  Instance  AUDIO IN Instance. It can be 1(DFSDM used)
  * @param  Device    Digital mic to be resumed
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_ResumeChannels(uint32_t Instance, uint32_t Device)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t audio_in_digital_mic = AUDIO_IN_DEVICE_DIGITAL_MIC_LAST;
  uint32_t i, mic_index;

  if((Instance >= AUDIO_IN_INSTANCES_NBR)  || ((Device < AUDIO_IN_DEVICE_DIGITAL_MIC1) && (Device > AUDIO_IN_DEVICE_DIGITAL_MIC_LAST)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_PAUSE)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    for(i = 0; i < DFSDM_MIC_NUMBER; i++)
    {
      if((Device & audio_in_digital_mic) == audio_in_digital_mic)
      {
        mic_index = MicBuffIndex[POS_VAL(audio_in_digital_mic)];

        /* Start selected device channel */
        if(HAL_DFSDM_FilterRegularStart_DMA(&haudio_in_dfsdm_filter[POS_VAL(audio_in_digital_mic)],\
          (int32_t*)AudioIn_Ctx[Instance].pMultiBuff[mic_index], AudioIn_Ctx[Instance].Size) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
      }
      audio_in_digital_mic = audio_in_digital_mic >> 1;
    }

    if(ret == BSP_ERROR_NONE)
    {
      /* Update BSP AUDIO IN state */
      AudioIn_Ctx[Instance].State = AUDIO_IN_STATE_RECORDING;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set Audio In device
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @param  Device    The audio input device to be used
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_SetDevice(uint32_t Instance, uint32_t Device)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t i;
  BSP_AUDIO_Init_t audio_init;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if(AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_STOP)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    for(i = 0; i < DFSDM_MIC_NUMBER; i ++)
    {
      if(((AudioIn_Ctx[Instance].Device >> i) & AUDIO_IN_DEVICE_DIGITAL_MIC1) == AUDIO_IN_DEVICE_DIGITAL_MIC1)
      {
        if(HAL_DFSDM_FilterDeInit(&haudio_in_dfsdm_filter[i]) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
          break;
        }
        else
        {
          if(HAL_DFSDM_ChannelDeInit(&haudio_in_dfsdm_channel[i]) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
            break;
          }
        }
      }
    }

    audio_init.Device        = Device;
    audio_init.ChannelsNbr   = AudioIn_Ctx[Instance].ChannelsNbr;
    audio_init.SampleRate    = AudioIn_Ctx[Instance].SampleRate;
    audio_init.BitsPerSample = AudioIn_Ctx[Instance].BitsPerSample;
    audio_init.Volume        = AudioIn_Ctx[Instance].Volume;

    if(BSP_AUDIO_IN_Init(Instance, &audio_init) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_NO_INIT;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get Audio In device
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @param  Device    The audio input device used
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_GetDevice(uint32_t Instance, uint32_t *Device)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State == AUDIO_IN_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    /* Return audio Input Device */
    *Device = AudioIn_Ctx[Instance].Device;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set Audio In frequency
  * @param  Instance     Audio IN instance
  * @param  SampleRate  Input frequency to be set
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_SetSampleRate(uint32_t Instance, uint32_t  SampleRate)
{
  int32_t ret = BSP_ERROR_NONE;
  BSP_AUDIO_Init_t audio_init;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_STOP)
  {
    ret = BSP_ERROR_BUSY;
  }/* Check if sample rate is modified */
  else if (AudioIn_Ctx[Instance].SampleRate == SampleRate)
  {
    /* Nothing to do */
  }
  else
  {
    uint32_t i;
    for(i = 0; i < DFSDM_MIC_NUMBER; i ++)
    {
      if(((AudioIn_Ctx[Instance].Device >> i) & AUDIO_IN_DEVICE_DIGITAL_MIC1) == AUDIO_IN_DEVICE_DIGITAL_MIC1)
      {
        if(HAL_DFSDM_ChannelDeInit(&haudio_in_dfsdm_channel[i]) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
          break;
        }
        else
        {
          if(HAL_DFSDM_FilterDeInit(&haudio_in_dfsdm_filter[i]) != HAL_OK)
          {
            ret = BSP_ERROR_PERIPH_FAILURE;
            break;
          }
        }
      }
    }

    audio_init.Device        = AudioIn_Ctx[Instance].Device;
    audio_init.ChannelsNbr   = AudioIn_Ctx[Instance].ChannelsNbr;
    audio_init.SampleRate    = SampleRate;
    audio_init.BitsPerSample = AudioIn_Ctx[Instance].BitsPerSample;
    audio_init.Volume        = AudioIn_Ctx[Instance].Volume;
    if(BSP_AUDIO_IN_Init(Instance, &audio_init) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_NO_INIT;
    }
  }

  /* Return BSP status */
  return ret;
}

/**
* @brief  Get Audio In frequency
* @param  Instance    AUDIO IN Instance. It can be only 0
* @param  SampleRate  Audio Input frequency to be returned
* @retval BSP status
*/
int32_t BSP_AUDIO_IN_GetSampleRate(uint32_t Instance, uint32_t *SampleRate)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State == AUDIO_IN_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    /* Return audio in frequency */
    *SampleRate = AudioIn_Ctx[Instance].SampleRate;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set Audio In Resolution
  * @param  Instance       AUDIO IN Instance. It can be only 0
  * @param  BitsPerSample  Input resolution to be set
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_SetBitsPerSample(uint32_t Instance, uint32_t BitsPerSample)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (BitsPerSample != AUDIO_RESOLUTION_16B)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  /* Check audio in state */
  else if (AudioIn_Ctx[Instance].State != AUDIO_IN_STATE_STOP)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    /* Nothing to do because there is only one bits per sample supported (AUDIO_RESOLUTION_16B) */
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get Audio In Resolution
  * @param  Instance       AUDIO IN Instance. It can be only 0
  * @param  BitsPerSample  Input resolution to be returned
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_GetBitsPerSample(uint32_t Instance, uint32_t *BitsPerSample)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }/* Check audio in state */
  else if (AudioIn_Ctx[Instance].State == AUDIO_IN_STATE_RESET)
  {
    ret = BSP_ERROR_BUSY;
  }
  else
  {
    /* Return audio in resolution */
    *BitsPerSample = AudioIn_Ctx[Instance].BitsPerSample;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set Audio In Channel number
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @param  ChannelNbr  Channel number to be used
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_SetChannelsNbr(uint32_t Instance, uint32_t ChannelNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if((Instance >= AUDIO_IN_INSTANCES_NBR) || (ChannelNbr > 2U))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Update AudioIn Context */
    AudioIn_Ctx[Instance].ChannelsNbr = ChannelNbr;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get Audio In Channel number
  * @param  Instance    AUDIO IN Instance. It can be only 0
  * @param  ChannelNbr  Channel number to be used
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_GetChannelsNbr(uint32_t Instance, uint32_t *ChannelNbr)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Channel number to be returned */
    *ChannelNbr = AudioIn_Ctx[Instance].ChannelsNbr;
  }
  return ret;
}

/**
  * @brief  Set the current audio in volume level.
  * @param  Instance  AUDIO IN Instance. It can only be 0
  * @param  Volume    Volume level to be returnd
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_SetVolume(uint32_t Instance, uint32_t Volume)
{
  /* Return BSP status */
  return BSP_ERROR_FEATURE_NOT_SUPPORTED;
}

/**
  * @brief  Get the current audio in volume level.
  * @param  Instance  AUDIO IN Instance. It can only be 0
  * @param  Volume    Volume level to be returnd
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_GetVolume(uint32_t Instance, uint32_t *Volume)
{
  /* Return BSP status */
  return BSP_ERROR_FEATURE_NOT_SUPPORTED;
}

/**
  * @brief  Get Audio In device
  * @param  Instance    AUDIO IN Instance. It can be 0 when I2S is used
  *                     or 1 if DFSDM is used
  * @param  State     Audio Out state
  * @retval BSP status
  */
int32_t BSP_AUDIO_IN_GetState(uint32_t Instance, uint32_t *State)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= AUDIO_IN_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Input State to be returned */
    *State = AudioIn_Ctx[Instance].State;
  }
  return ret;
}

/**
  * @brief  This function handles Audio In DMA interrupt requests.
  * @param  Instance Audio IN instance: 0 for DFSDM
  * @param  InputDevice Can be:
  *         - AUDIO_IN_DEVICE_DIGITAL_MIC
  *         - AUDIO_IN_DEVICE_DIGITAL_MICx (1<= x <= 5)
  * @retval None
  */
void BSP_AUDIO_IN_DMA_IRQHandler(uint32_t Instance, uint32_t InputDevice)
{
  if(Instance == 0U)
  {
    HAL_DMA_IRQHandler(haudio_in_dfsdm_filter[POS_VAL(InputDevice)].hdmaReg);
  }
}

#if !defined (USE_HAL_DFSDM_REGISTER_CALLBACKS) || (USE_HAL_DFSDM_REGISTER_CALLBACKS == 0)
/**
  * @brief  Regular conversion complete callback.
  * @note   In interrupt mode, user has to read conversion value in this function
            using HAL_DFSDM_FilterGetRegularValue.
  * @param  hdfsdm_filter   DFSDM filter handle.
  * @retval None
  */
void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  uint32_t index;
  static uint32_t DmaRecBuffCplt[DFSDM_MIC_NUMBER]  = {0};
  int32_t  tmp;

  if(AudioIn_Ctx[0].IsMultiBuff == 1U)
  {
    /* Call the record update function to get the second half */
    BSP_AUDIO_IN_TransferComplete_CallBack(0);
  }
  else
  {
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)])
    {
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 1;
    }
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)])
    {
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 1;
    }

    if((DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] == 1U) && (DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] == 1U))
    {
      for(index = (DEFAULT_AUDIO_IN_BUFFER_SIZE/2U) ; index < DEFAULT_AUDIO_IN_BUFFER_SIZE; index++)
      {
        if(AudioIn_Ctx[0].ChannelsNbr == 2U)
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]     = (uint16_t)(tmp);
          tmp = MicRecBuff[1][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        else
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]      = (uint16_t)(tmp);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        RecBuffTrigger +=2U;
      }
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 0;
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 0;
    }

    /* Call Half Transfer Complete callback */
    if(RecBuffTrigger == (AudioIn_Ctx[0].Size/2U))
    {
      if(RecBuffHalf == 0U)
      {
        RecBuffHalf = 1;
        BSP_AUDIO_IN_HalfTransfer_CallBack(0);
      }
    }
    /* Call Transfer Complete callback */
    if(RecBuffTrigger == AudioIn_Ctx[0].Size)
    {
      /* Reset Application Buffer Trigger */
      RecBuffTrigger = 0;
      RecBuffHalf = 0;
      /* Call the record update function to get the next buffer to fill and its size (size is ignored) */
      BSP_AUDIO_IN_TransferComplete_CallBack(0);
    }
  }
}

/**
  * @brief  Half regular conversion complete callback.
  * @param  hdfsdm_filter   DFSDM filter handle.
  * @retval None
  */
void HAL_DFSDM_FilterRegConvHalfCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  uint32_t index;
  static uint32_t DmaRecHalfBuffCplt[DFSDM_MIC_NUMBER]  = {0};
  int32_t  tmp;

  if(AudioIn_Ctx[0].IsMultiBuff == 1U)
  {
    /* Call the record update function to get the first half */
    BSP_AUDIO_IN_HalfTransfer_CallBack(0);
  }
  else
  {
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)])
    {
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 1;
    }
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)])
    {
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 1;
    }

    if((DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] == 1U) && (DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] == 1U))
    {
      for(index = 0 ; index < (DEFAULT_AUDIO_IN_BUFFER_SIZE/2U); index++)
      {
        if(AudioIn_Ctx[0].ChannelsNbr == 2U)
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]     = (uint16_t)(tmp);
          tmp = MicRecBuff[1][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        else
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]      = (uint16_t)(tmp);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        RecBuffTrigger +=2U;
      }
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 0;
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 0;
    }

    /* Call Half Transfer Complete callback */
    if(RecBuffTrigger == (AudioIn_Ctx[0].Size/2U))
    {
      if(RecBuffHalf == 0U)
      {
        RecBuffHalf = 1;
        BSP_AUDIO_IN_HalfTransfer_CallBack(0);
      }
    }
    /* Call Transfer Complete callback */
    if(RecBuffTrigger == AudioIn_Ctx[0].Size)
    {
      /* Reset Application Buffer Trigger */
      RecBuffTrigger = 0;
      RecBuffHalf = 0;
      /* Call the record update function to get the next buffer to fill and its size (size is ignored) */
      BSP_AUDIO_IN_TransferComplete_CallBack(0);
    }
  }
}
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 0) */

/**
  * @brief  User callback when record buffer is filled.
  * @retval None
  */
__weak void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* This function should be implemented by the user application.
     It is called into this driver when the current buffer is filled
     to prepare the next buffer pointer and its size. */
}

/**
  * @brief  Manages the DMA Half Transfer complete event.
  * @retval None
  */
__weak void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* This function should be implemented by the user application.
     It is called into this driver when the current buffer is filled
     to prepare the next buffer pointer and its size. */
}

/**
  * @brief  Audio IN Error callback function.
  * @retval None
  */
__weak void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* This function is called when an Interrupt due to transfer error on or peripheral
     error occurs. */
}
/**
  * @}
  */

/** @defgroup STM32F413H_DISCOVERY_AUDIO_IN_Private_Functions AUDIO_IN Private Functions
  * @{
  */
#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
/**
  * @brief  Regular conversion complete callback.
  * @note   In interrupt mode, user has to read conversion value in this function
            using HAL_DFSDM_FilterGetRegularValue.
  * @param  hdfsdm_filter   DFSDM filter handle.
  * @retval None
  */
static void DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  uint32_t index;
  static uint32_t DmaRecBuffCplt[DFSDM_MIC_NUMBER]  = {0};
  int32_t  tmp;

  if(AudioIn_Ctx[0].IsMultiBuff == 1U)
  {
    /* Call the record update function to get the second half */
    BSP_AUDIO_IN_TransferComplete_CallBack(0);
  }
  else
  {
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)])
    {
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 1;
    }
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)])
    {
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 1;
    }

    if((DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] == 1U) && (DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] == 1U))
    {
      for(index = (DEFAULT_AUDIO_IN_BUFFER_SIZE/2U) ; index < DEFAULT_AUDIO_IN_BUFFER_SIZE; index++)
      {
        if(AudioIn_Ctx[0].ChannelsNbr == 2U)
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]     = (uint16_t)(tmp);
          tmp = MicRecBuff[1][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        else
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]      = (uint16_t)(tmp);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        RecBuffTrigger +=2U;
      }
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 0;
      DmaRecBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 0;
    }

    /* Call Half Transfer Complete callback */
    if(RecBuffTrigger == (AudioIn_Ctx[0].Size/2U))
    {
      if(RecBuffHalf == 0U)
      {
        RecBuffHalf = 1;
        BSP_AUDIO_IN_HalfTransfer_CallBack(0);
      }
    }
    /* Call Transfer Complete callback */
    if(RecBuffTrigger == AudioIn_Ctx[0].Size)
    {
      /* Reset Application Buffer Trigger */
      RecBuffTrigger = 0;
      RecBuffHalf = 0;
      /* Call the record update function to get the next buffer to fill and its size (size is ignored) */
      BSP_AUDIO_IN_TransferComplete_CallBack(0);
    }
  }
}

/**
  * @brief  Half regular conversion complete callback.
  * @param  hdfsdm_filter   DFSDM filter handle.
  * @retval None
  */
static void DFSDM_FilterRegConvHalfCpltCallback(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  uint32_t index;
  static uint32_t DmaRecHalfBuffCplt[DFSDM_MIC_NUMBER]  = {0};
  int32_t  tmp;

  if(AudioIn_Ctx[0].IsMultiBuff == 1U)
  {
    /* Call the record update function to get the first half */
    BSP_AUDIO_IN_HalfTransfer_CallBack(0);
  }
  else
  {
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)])
    {
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 1;
    }
    if(hdfsdm_filter == &haudio_in_dfsdm_filter[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)])
    {
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 1;
    }

    if((DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] == 1U) && (DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] == 1U))
    {
      for(index = 0 ; index < (DEFAULT_AUDIO_IN_BUFFER_SIZE/2U); index++)
      {
        if(AudioIn_Ctx[0].ChannelsNbr == 2U)
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]     = (uint16_t)(tmp);
          tmp = MicRecBuff[1][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        else
        {
          tmp = MicRecBuff[0][index] / 256;
          tmp = SaturaLH(tmp, -32768, 32767);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger]      = (uint16_t)(tmp);
          AudioIn_Ctx[0].pBuff[RecBuffTrigger + 1U] = (uint16_t)(tmp);
        }
        RecBuffTrigger +=2U;
      }
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] = 0;
      DmaRecHalfBuffCplt[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] = 0;
    }

    /* Call Half Transfer Complete callback */
    if(RecBuffTrigger == (AudioIn_Ctx[0].Size/2U))
    {
      if(RecBuffHalf == 0U)
      {
        RecBuffHalf = 1;
        BSP_AUDIO_IN_HalfTransfer_CallBack(0);
      }
    }
    /* Call Transfer Complete callback */
    if(RecBuffTrigger == AudioIn_Ctx[0].Size)
    {
      /* Reset Application Buffer Trigger */
      RecBuffTrigger = 0;
      RecBuffHalf = 0;
      /* Call the record update function to get the next buffer to fill and its size (size is ignored) */
      BSP_AUDIO_IN_TransferComplete_CallBack(0);
    }
  }
}
#endif /* (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Initialize the DFSDM channel MSP.
  * @param  hDfsdmChannel DFSDM Channel handle
  * @retval None
  */
static void DFSDM_ChannelMspInit(DFSDM_Channel_HandleTypeDef *hDfsdmChannel)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hDfsdmChannel);

  /* Enable DFSDM clock */
  AUDIO_DFSDM1_CLK_ENABLE();
  AUDIO_DFSDM2_CLK_ENABLE();

  /* Enable GPIO clock */
  AUDIO_DFSDMx_CKOUT_MIC1_GPIO_CLK_ENABLE();
  AUDIO_DFSDMx_CKOUT_GPIO_CLK_ENABLE();
  AUDIO_DFSDMx_DATIN_MIC1_GPIO_CLK_ENABLE();
  AUDIO_DFSDMx_DATIN_MIC2_GPIO_CLK_ENABLE();
  AUDIO_DFSDMx_DATIN_MIC3_GPIO_CLK_ENABLE();
  AUDIO_DFSDMx_DATIN_MIC4_GPIO_CLK_ENABLE();
  AUDIO_DFSDMx_DATIN_MIC5_GPIO_CLK_ENABLE();

  /* DFSDM pins configuration: DFSDM_CKOUT, DMIC_DATIN pins ------------------*/
  GPIO_InitStruct.Pin       = AUDIO_DFSDMx_CKOUT_MIC1_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_CKOUT_MIC1_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_CKOUT_MIC1_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC1_PIN;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_DATIN_MIC1_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_DATIN_MIC1_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin       = AUDIO_DFSDMx_CKOUT_PIN;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_CKOUT_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_CKOUT_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC2_PIN;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_DATIN_MIC2_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_DATIN_MIC2_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC3_PIN;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_DATIN_MIC3_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_DATIN_MIC3_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC3_PIN;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_DATIN_MIC4_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_DATIN_MIC4_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC5_PIN;
  GPIO_InitStruct.Alternate = AUDIO_DFSDMx_DATIN_MIC5_AF;
  HAL_GPIO_Init(AUDIO_DFSDMx_DATIN_MIC5_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  DeInitialize the DFSDM channel MSP.
  * @param  hDfsdmChannel DFSDM Channel handle
  * @retval None
  */
static void DFSDM_ChannelMspDeInit(DFSDM_Channel_HandleTypeDef *hDfsdmChannel)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hDfsdmChannel);

  /* DFSDM pins configuration: DFSDM_CKOUT, DMIC_DATIN pins ------------------*/
  GPIO_InitStruct.Pin = AUDIO_DFSDMx_CKOUT_MIC1_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_CKOUT_MIC1_GPIO_PORT, GPIO_InitStruct.Pin);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_CKOUT_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_CKOUT_GPIO_PORT, GPIO_InitStruct.Pin);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC1_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_DATIN_MIC1_GPIO_PORT, GPIO_InitStruct.Pin);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC2_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_DATIN_MIC2_GPIO_PORT, GPIO_InitStruct.Pin);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC3_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_DATIN_MIC3_GPIO_PORT, GPIO_InitStruct.Pin);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC4_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_DATIN_MIC4_GPIO_PORT, GPIO_InitStruct.Pin);

  GPIO_InitStruct.Pin = AUDIO_DFSDMx_DATIN_MIC5_PIN;
  HAL_GPIO_DeInit(AUDIO_DFSDMx_DATIN_MIC5_GPIO_PORT, GPIO_InitStruct.Pin);
}

/**
  * @brief  Initialize the DFSDM filter MSP.
  * @param  hDfsdmFilter DFSDM Filter handle
  * @retval None
  */
static void DFSDM_FilterMspInit(DFSDM_Filter_HandleTypeDef *hDfsdmFilter)
{
  uint32_t i, mic_num = 0, mic_init[DFSDM_MIC_NUMBER] = {0};
  IRQn_Type AUDIO_DFSDM_DMAx_MIC_IRQHandler[DFSDM_MIC_NUMBER] = {AUDIO_DFSDMx_DMAx_MIC1_IRQ, AUDIO_DFSDMx_DMAx_MIC2_IRQ, AUDIO_DFSDMx_DMAx_MIC3_IRQ, AUDIO_DFSDMx_DMAx_MIC4_IRQ, AUDIO_DFSDMx_DMAx_MIC5_IRQ};
  DMA_Stream_TypeDef* AUDIO_DFSDMx_DMAx_MIC_STREAM[DFSDM_MIC_NUMBER] = {AUDIO_DFSDMx_DMAx_MIC1_STREAM, AUDIO_DFSDMx_DMAx_MIC2_STREAM, AUDIO_DFSDMx_DMAx_MIC3_STREAM, AUDIO_DFSDMx_DMAx_MIC4_STREAM, AUDIO_DFSDMx_DMAx_MIC5_STREAM};
  uint32_t AUDIO_DFSDMx_DMAx_MIC_CHANNEL[DFSDM_MIC_NUMBER] = {AUDIO_DFSDMx_DMAx_MIC1_CHANNEL, AUDIO_DFSDMx_DMAx_MIC2_CHANNEL, AUDIO_DFSDMx_DMAx_MIC3_CHANNEL, AUDIO_DFSDMx_DMAx_MIC4_CHANNEL, AUDIO_DFSDMx_DMAx_MIC5_CHANNEL};

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hDfsdmFilter);

  /* Enable DFSDM clock */
  AUDIO_DFSDM1_CLK_ENABLE();
  AUDIO_DFSDM2_CLK_ENABLE();

  /* Enable the DMA clock */
  AUDIO_DFSDMx_DMAx_CLK_ENABLE();

  for(i = 0; i < DFSDM_MIC_NUMBER; i++)
  {
    if(((AudioIn_Ctx[0].Device & AUDIO_IN_DEVICE_DIGITAL_MIC1) == AUDIO_IN_DEVICE_DIGITAL_MIC1) && (mic_init[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1)] != 1U))
    {
      mic_num = POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC1);
      mic_init[mic_num] = 1;
    }
    else if(((AudioIn_Ctx[0].Device & AUDIO_IN_DEVICE_DIGITAL_MIC2) == AUDIO_IN_DEVICE_DIGITAL_MIC2) && (mic_init[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2)] != 1U))
    {
      mic_num = POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC2);
      mic_init[mic_num] = 1;
    }
    else if(((AudioIn_Ctx[0].Device & AUDIO_IN_DEVICE_DIGITAL_MIC3) == AUDIO_IN_DEVICE_DIGITAL_MIC3) && (mic_init[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC3)] != 1U))
    {
      mic_num = POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC3);
      mic_init[mic_num] = 1;
    }
    else if(((AudioIn_Ctx[0].Device & AUDIO_IN_DEVICE_DIGITAL_MIC4) == AUDIO_IN_DEVICE_DIGITAL_MIC4) && (mic_init[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC4)] != 1U))
    {
      mic_num = POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC4);
      mic_init[mic_num] = 1;
    }
    else
    {
      if(((AudioIn_Ctx[0].Device & AUDIO_IN_DEVICE_DIGITAL_MIC5) == AUDIO_IN_DEVICE_DIGITAL_MIC5) && (mic_init[POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC5)] != 1U))
      {
        mic_num = POS_VAL(AUDIO_IN_DEVICE_DIGITAL_MIC5);
        mic_init[mic_num] = 1;
      }
    }

    /* Configure the hDmaDfsdm[i] handle parameters */
    hDmaDfsdm[mic_num].Init.Channel             = AUDIO_DFSDMx_DMAx_MIC_CHANNEL[mic_num];
    hDmaDfsdm[mic_num].Instance                 = AUDIO_DFSDMx_DMAx_MIC_STREAM[mic_num];
    hDmaDfsdm[mic_num].Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hDmaDfsdm[mic_num].Init.PeriphInc           = DMA_PINC_DISABLE;
    hDmaDfsdm[mic_num].Init.MemInc              = DMA_MINC_ENABLE;
    hDmaDfsdm[mic_num].Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hDmaDfsdm[mic_num].Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hDmaDfsdm[mic_num].Init.Mode                = DMA_CIRCULAR;
    hDmaDfsdm[mic_num].Init.Priority            = DMA_PRIORITY_HIGH;
    hDmaDfsdm[mic_num].Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hDmaDfsdm[mic_num].Init.MemBurst            = DMA_MBURST_SINGLE;
    hDmaDfsdm[mic_num].Init.PeriphBurst         = DMA_PBURST_SINGLE;
    hDmaDfsdm[mic_num].State                    = HAL_DMA_STATE_RESET;

    /* Associate the DMA handle */
    __HAL_LINKDMA(&haudio_in_dfsdm_filter[mic_num], hdmaReg, hDmaDfsdm[mic_num]);

    /* Reset DMA handle state */
    __HAL_DMA_RESET_HANDLE_STATE(&hDmaDfsdm[mic_num]);

    /* Configure the DMA Channel */
    (void)HAL_DMA_Init(&hDmaDfsdm[mic_num]);

    /* DMA IRQ Channel configuration */
    HAL_NVIC_SetPriority(AUDIO_DFSDM_DMAx_MIC_IRQHandler[mic_num], BSP_AUDIO_IN_IT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(AUDIO_DFSDM_DMAx_MIC_IRQHandler[mic_num]);
  }
}

/**
  * @brief  DeInitialize the DFSDM filter MSP.
  * @param  hDfsdmFilter DFSDM Filter handle
  * @retval None
  */
static void DFSDM_FilterMspDeInit(DFSDM_Filter_HandleTypeDef *hDfsdmFilter)
{
  uint32_t i;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hDfsdmFilter);

  /* Configure the DMA Channel */
  for(i = 0; i < DFSDM_MIC_NUMBER; i++)
  {
    if(hDmaDfsdm[i].Instance != NULL)
    {
      (void)HAL_DMA_DeInit(&hDmaDfsdm[i]);
    }
  }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
