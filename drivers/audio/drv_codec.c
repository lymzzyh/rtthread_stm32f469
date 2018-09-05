/**
  ******************************************************************************
  * @file    stm32469i_discovery_audio.c
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    27-January-2017
  * @brief   This file provides the Audio driver for the STM32469I-Discovery board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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

/*==============================================================================
                                 User NOTES

How To use this driver:
-----------------------
   + This driver supports STM32F4xx devices on STM32469I-Discovery (MB1189) Discovery boards.
   + Call the function BSP_AUDIO_OUT_Init(
                                    OutputDevice: physical output mode (OUTPUT_DEVICE_HEADPHONE1,
                                                  OUTPUT_DEVICE_HEADPHONE2 or OUTPUT_DEVICE_BOTH)
                                    Volume      : Initial volume to be set (0 is min (mute), 100 is max (100%)
                                    AudioFreq   : Audio frequency in Hz (8000, 16000, 22500, 32000...)
                                                  this parameter is relative to the audio file/stream type.
                                   )
      This function configures all the hardware required for the audio application (codec, I2C, SAI,
      GPIOs, DMA and interrupt if needed). This function returns AUDIO_OK if configuration is OK.
      If the returned value is different from AUDIO_OK or the function is stuck then the communication with
      the codec or the MFX has failed (try to un-plug the power or reset device in this case).
      - OUTPUT_DEVICE_HEADPHONE1 : only headphones 1 will be set as output for the audio stream.
      - OUTPUT_DEVICE_HEADPHONE2 : only headphones 2 will be set as output for the audio stream.
      - OUTPUT_DEVICE_BOTH       : both Headphones are used as outputs for the audio stream
                                 at the same time.
      Note. On STM32469I-Discovery SAI_DMA is configured in CIRCULAR mode. Due to this the application
        does NOT need to call BSP_AUDIO_OUT_ChangeBuffer() to assure straming.
   + Call the function BSP_Discovery_AUDIO_OUT_Play(
                                  pBuffer: pointer to the audio data file address
                                  Size   : size of the buffer to be sent in Bytes
                                 )
      to start playing (for the first time) from the audio file/stream.
   + Call the function BSP_AUDIO_OUT_Pause() to pause playing
   + Call the function BSP_AUDIO_OUT_Resume() to resume playing.
       Note. After calling BSP_AUDIO_OUT_Pause() function for pause, only BSP_AUDIO_OUT_Resume() should be called
          for resume (it is not allowed to call BSP_AUDIO_OUT_Play() in this case).
       Note. This function should be called only when the audio file is played or paused (not stopped).
   + For each mode, you may need to implement the relative callback functions into your code.
      The Callback functions are named AUDIO_OUT_XXX_CallBack() and only their prototypes are declared in
      the stm32469i_discovery_audio.h file. (refer to the example for more details on the callbacks implementations)
   + To Stop playing, to modify the volume level, the frequency, the audio frame slot,
      the device output mode the mute or the stop, use the functions: BSP_AUDIO_OUT_SetVolume(),
      AUDIO_OUT_SetFrequency(), BSP_AUDIO_OUT_SetAudioFrameSlot(), BSP_AUDIO_OUT_SetOutputMode(),
      BSP_AUDIO_OUT_SetMute() and BSP_AUDIO_OUT_Stop().
   + The driver API and the callback functions are at the end of the stm32469i_discovery_audio.h file.

Driver architecture:
--------------------
   + This driver provide the High Audio Layer: consists of the function API exported in the stm32469i_discovery_audio.h file
     (BSP_AUDIO_OUT_Init(), BSP_AUDIO_OUT_Play() ...)
   + This driver provide also the Media Access Layer (MAL): which consists of functions allowing to access the media containing/
     providing the audio file/stream. These functions are also included as local functions into
     the stm32469i_discovery_audio_codec.c file (I2Sx_Init(), I2Sx_DeInit(), SAIx_Init() and SAIx_DeInit())

Known Limitations:
------------------
   1- If the TDM Format used to paly in parallel 2 audio Stream (the first Stream is configured in codec SLOT0 and second
      Stream in SLOT1) the Pause/Resume, volume and mute feature will control the both streams.
   2- Parsing of audio file is not implemented (in order to determine audio file properties: Mono/Stereo, Data size,
      File size, Audio Frequency, Audio Data header size ...). The configuration is fixed for the given audio file.
   3- Supports only Stereo audio streaming.
   4- Supports only 16-bits audio data size.
==============================================================================*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "stm32f4xx.h"
#include "drv_codec.h"

#define OUTPUT_DEVICE_HEADPHONE1 OUTPUT_DEVICE_HEADPHONE
#define OUTPUT_DEVICE_HEADPHONE2 OUTPUT_DEVICE_SPEAKER

/* SCK(kHz) = SAI_CK_x/(SAIClockDivider*2*256) */
#define SAIClockDivider(__FREQUENCY__)                                                                                                                                                                                                                     \
    (__FREQUENCY__ == AUDIO_FREQUENCY_8K) ? 12                                                                                                                                                                                                             \
                                          : (__FREQUENCY__ == AUDIO_FREQUENCY_11K) ? 2                                                                                                                                                                     \
                                                                                   : (__FREQUENCY__ == AUDIO_FREQUENCY_16K) ? 6                                                                                                                            \
                                                                                                                            : (__FREQUENCY__ == AUDIO_FREQUENCY_22K) ? 1                                                                                   \
                                                                                                                                                                     : (__FREQUENCY__ == AUDIO_FREQUENCY_32K) ? 3                                          \
                                                                                                                                                                                                              : (__FREQUENCY__ == AUDIO_FREQUENCY_44K) ? 0 \
                                                                                                                                                                                                                                                       : (__FREQUENCY__ == AUDIO_FREQUENCY_48K) ? 2 : 1

AUDIO_DrvTypeDef *audio_drv;
SAI_HandleTypeDef haudio_out_sai;

static uint8_t SAIx_Init(uint32_t AudioFreq);
static void SAIx_DeInit(void);

void BSP_AUDIO_OUT_ChangeAudioConfig(uint32_t AudioOutOption);

uint8_t BSP_AUDIO_OUT_Init(uint16_t OutputDevice,
                           uint8_t Volume,
                           uint32_t AudioFreq)
{
    uint8_t ret = AUDIO_OK;

    SAIx_DeInit();

    /* PLL clock is set depending by the AudioFreq (44.1khz vs 48khz groups) */
    BSP_AUDIO_OUT_ClockConfig(&haudio_out_sai, AudioFreq, NULL);

    /* SAI data transfer preparation:
  Prepare the Media to be used for the audio transfer from memory to SAI peripheral */
    haudio_out_sai.Instance = AUDIO_SAIx;
    if (HAL_SAI_GetState(&haudio_out_sai) == HAL_SAI_STATE_RESET)
    {
        /* Init the SAI MSP: this __weak function can be redefined by the application*/
        BSP_AUDIO_OUT_MspInit(&haudio_out_sai, NULL);
    }

    if (SAIx_Init(AudioFreq) != AUDIO_OK)
    {
        ret = AUDIO_ERROR;
    }

    if (ret == AUDIO_OK)
    {
        /* Retieve audio codec identifier */
        if (cs43l22_drv.ReadID(AUDIO_I2C_ADDRESS) == CS43L22_ID)
        {
            /* Initialize the audio driver structure */
            audio_drv = &cs43l22_drv;
        }
        else
        {
            ret = AUDIO_ERROR;
        }
    }

    if (ret == AUDIO_OK)
    {
        /* Initialize the audio codec internal registers */
        if (audio_drv->Init(AUDIO_I2C_ADDRESS,
                            OutputDevice,
                            Volume,
                            AudioFreq) != AUDIO_OK)
        {
            ret = AUDIO_ERROR;
        }
    }

    return ret;
}

uint8_t BSP_AUDIO_OUT_Play(uint16_t *pBuffer, uint32_t Size)
{
    uint8_t ret = AUDIO_OK;

    /* Call the audio Codec Play function */
    if (audio_drv->Play(AUDIO_I2C_ADDRESS, pBuffer, Size) != 0)
    {
        ret = AUDIO_ERROR;
    }

    /* Initiate a DMA transfer of PCM samples towards the serial audio interface */
    if (ret == AUDIO_OK)
    {
        if (HAL_SAI_Transmit_DMA(&haudio_out_sai, (uint8_t *)pBuffer, DMA_MAX(Size / AUDIODATA_SIZE)) != HAL_OK)
        {
            ret = AUDIO_ERROR;
        }
    }

    return ret;
}

void BSP_AUDIO_OUT_ChangeBuffer(uint16_t *pData, uint16_t Size)
{
    HAL_SAI_Transmit_DMA(&haudio_out_sai, (uint8_t *)pData, Size);
}

uint8_t BSP_AUDIO_OUT_Pause(void)
{
    uint8_t ret = AUDIO_OK;

    /* Call the Audio Codec Pause/Resume function */
    if (audio_drv->Pause(AUDIO_I2C_ADDRESS) != 0)
    {
        ret = AUDIO_ERROR;
    }

    /* Pause DMA transfer of PCM samples towards the serial audio interface */
    if (ret == AUDIO_OK)
    {
        if (HAL_SAI_DMAPause(&haudio_out_sai) != HAL_OK)
        {
            ret = AUDIO_ERROR;
        }
    }

    /* Return AUDIO_OK when all operations are correctly done */
    return ret;
}

uint8_t BSP_AUDIO_OUT_Resume(void)
{
    uint8_t ret = AUDIO_OK;

    /* Call the Audio Codec Pause/Resume function */
    if (audio_drv->Resume(AUDIO_I2C_ADDRESS) != 0)
    {
        ret = AUDIO_ERROR;
    }

    /* Resume DMA transfer of PCM samples towards the serial audio interface */
    if (ret == AUDIO_OK)
    {
        if (HAL_SAI_DMAResume(&haudio_out_sai) != HAL_OK)
        {
            ret = AUDIO_ERROR;
        }
    }

    /* Return AUDIO_OK when all operations are correctly done */
    return ret;
}

uint8_t BSP_AUDIO_OUT_Stop(uint32_t Option)
{
    uint8_t ret = AUDIO_OK;

    /* Call Audio Codec Stop function */
    if (audio_drv->Stop(AUDIO_I2C_ADDRESS, Option) != 0)
    {
        ret = AUDIO_ERROR;
    }

    if (ret == AUDIO_OK)
    {
        if (Option == CODEC_PDWN_HW)
        {
            /* Wait at least 100us */
            HAL_Delay(2);
        }

        /* Stop DMA transfer of PCM samples towards the serial audio interface */
        if (HAL_SAI_DMAStop(&haudio_out_sai) != HAL_OK)
        {
            ret = AUDIO_ERROR;
        }
    }
    /* Return AUDIO_OK when all operations are correctly done */
    return ret;
}

uint8_t BSP_AUDIO_OUT_SetVolume(uint8_t Volume)
{
    uint8_t ret = AUDIO_OK;

    /* Call the codec volume control function with converted volume value */
    if (audio_drv->SetVolume(AUDIO_I2C_ADDRESS, Volume) != 0)
    {
        ret = AUDIO_ERROR;
    }

    /* Return AUDIO_OK when all operations are correctly done */
    return ret;
}

uint8_t BSP_AUDIO_OUT_SetMute(uint32_t Cmd)
{
    uint8_t ret = AUDIO_OK;

    /* Call the Codec Mute function */
    if (audio_drv->SetMute(AUDIO_I2C_ADDRESS, Cmd) != 0)
    {
        ret = AUDIO_ERROR;
    }

    /* Return AUDIO_OK when all operations are correctly done */
    return ret;
}

uint8_t BSP_AUDIO_OUT_SetOutputMode(uint8_t Output)
{
    uint8_t ret = AUDIO_OK;

    /* Call the Codec output device function */
    if (audio_drv->SetOutputMode(AUDIO_I2C_ADDRESS, Output) != 0)
    {
        ret = AUDIO_ERROR;
    }

    /* Return AUDIO_OK when all operations are correctly done */
    return ret;
}

void BSP_AUDIO_OUT_SetFrequency(uint32_t AudioFreq)
{
    /* PLL clock is set depending by the AudioFreq (44.1khz vs 48khz groups) */
    BSP_AUDIO_OUT_ClockConfig(&haudio_out_sai, AudioFreq, NULL);

    /* Disable SAI peripheral to allow access to SAI internal registers */
    __HAL_SAI_DISABLE(&haudio_out_sai);

    /* Update the SAI audio frequency configuration */
    haudio_out_sai.Init.AudioFrequency = AudioFreq;
    HAL_SAI_Init(&haudio_out_sai);

    /* Enable SAI peripheral to generate MCLK */
    __HAL_SAI_ENABLE(&haudio_out_sai);
}

void BSP_AUDIO_OUT_ChangeAudioConfig(uint32_t AudioOutOption)
{
    /********** Playback Buffer circular/normal mode **********/
    if (AudioOutOption & BSP_AUDIO_OUT_CIRCULARMODE)
    {
        /* Deinitialize the Stream to update DMA mode */
        HAL_DMA_DeInit(haudio_out_sai.hdmatx);

        /* Update the SAI audio Transfer DMA mode */
        haudio_out_sai.hdmatx->Init.Mode = DMA_CIRCULAR;

        /* Configure the DMA Stream with new Transfer DMA mode */
        HAL_DMA_Init(haudio_out_sai.hdmatx);
    }
    else /* BSP_AUDIO_OUT_NORMALMODE */
    {
        /* Deinitialize the Stream to update DMA mode */
        HAL_DMA_DeInit(haudio_out_sai.hdmatx);

        /* Update the SAI audio Transfer DMA mode */
        haudio_out_sai.hdmatx->Init.Mode = DMA_NORMAL;

        /* Configure the DMA Stream with new Transfer DMA mode */
        HAL_DMA_Init(haudio_out_sai.hdmatx);
    }

    /********** Playback Buffer stereo/mono mode **********/
    if (AudioOutOption & BSP_AUDIO_OUT_STEREOMODE)
    {
        /* Disable SAI peripheral to allow access to SAI internal registers */
        __HAL_SAI_DISABLE(&haudio_out_sai);

        /* Update the SAI audio frame slot configuration */
        haudio_out_sai.Init.MonoStereoMode = SAI_STEREOMODE;
        HAL_SAI_Init(&haudio_out_sai);

        /* Enable SAI peripheral to generate MCLK */
        __HAL_SAI_ENABLE(&haudio_out_sai);
    }
    else /* BSP_AUDIO_OUT_MONOMODE */
    {
        /* Disable SAI peripheral to allow access to SAI internal registers */
        __HAL_SAI_DISABLE(&haudio_out_sai);

        /* Update the SAI audio frame slot configuration */
        haudio_out_sai.Init.MonoStereoMode = SAI_MONOMODE;
        HAL_SAI_Init(&haudio_out_sai);

        /* Enable SAI peripheral to generate MCLK */
        __HAL_SAI_ENABLE(&haudio_out_sai);
    }
}

void BSP_AUDIO_OUT_SetAudioFrameSlot(uint32_t AudioFrameSlot)
{
    /* Disable SAI peripheral to allow access to SAI internal registers */
    __HAL_SAI_DISABLE(&haudio_out_sai);

    /* Update the SAI audio frame slot configuration */
    haudio_out_sai.SlotInit.SlotActive = AudioFrameSlot;
    HAL_SAI_Init(&haudio_out_sai);

    /* Enable SAI peripheral to generate MCLK */
    __HAL_SAI_ENABLE(&haudio_out_sai);
}

void BSP_AUDIO_OUT_DeInit(void)
{
    SAIx_DeInit();
    /* DeInit the SAI MSP : this __weak function can be rewritten by the applic */
    BSP_AUDIO_OUT_MspDeInit(&haudio_out_sai, NULL);

    /* Reset the audio output context */
    memset(&audio_drv, 0, sizeof(audio_drv));
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32469i_discovery_audio.h) */
    BSP_AUDIO_OUT_TransferComplete_CallBack();
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32469i_discovery_audio.h) */
    BSP_AUDIO_OUT_HalfTransfer_CallBack();
}

void DMA2_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void DMA1_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(haudio_out_sai.hdmarx);
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    BSP_AUDIO_OUT_Error_CallBack();
}

__weak void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
}

__weak void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
}

__weak void BSP_AUDIO_OUT_Error_CallBack(void)
{
}

__weak void BSP_AUDIO_OUT_MspInit(SAI_HandleTypeDef *hsai, void *Params)
{
    static DMA_HandleTypeDef hdma_sai_tx;
    GPIO_InitTypeDef gpio_init_structure;

    /* Put CS43L2 codec reset high -----------------------------------*/
    AUDIO_RESET_ENABLE();
    gpio_init_structure.Pin = AUDIO_RESET_PIN;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(AUDIO_RESET_GPIO_PORT, &gpio_init_structure);
    HAL_GPIO_WritePin(AUDIO_RESET_GPIO_PORT, AUDIO_RESET_PIN, GPIO_PIN_SET);

    /* Enable SAI clock */
    AUDIO_SAIx_CLK_ENABLE();

    /* Enable GPIO clock */
    AUDIO_SAIx_MCLK_ENABLE();
    AUDIO_SAIx_SCK_SD_FS_ENABLE();

    /* CODEC_SAI pins configuration: MCK pin -----------------------------------*/
    gpio_init_structure.Pin = AUDIO_SAIx_MCK_PIN;
    gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;
    gpio_init_structure.Alternate = AUDIO_SAIx_MCLK_SCK_SD_FS_AF;
    HAL_GPIO_Init(AUDIO_SAIx_MCLK_GPIO_PORT, &gpio_init_structure);

    /* CODEC_SAI pins configuration: FS, SCK, MCK and SD pins ------------------*/
    gpio_init_structure.Pin = AUDIO_SAIx_FS_PIN | AUDIO_SAIx_SCK_PIN | AUDIO_SAIx_SD_PIN;
    gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;
    gpio_init_structure.Alternate = AUDIO_SAIx_MCLK_SCK_SD_FS_AF;
    HAL_GPIO_Init(AUDIO_SAIx_SCK_SD_FS_GPIO_PORT, &gpio_init_structure);

    /* Enable the DMA clock */
    AUDIO_SAIx_DMAx_CLK_ENABLE();

    if (hsai->Instance == AUDIO_SAIx)
    {
        /* Configure the hdma_saiTx handle parameters */
        hdma_sai_tx.Init.Channel = AUDIO_SAIx_DMAx_CHANNEL;
        hdma_sai_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_sai_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sai_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sai_tx.Init.PeriphDataAlignment = AUDIO_SAIx_DMAx_PERIPH_DATA_SIZE;
        hdma_sai_tx.Init.MemDataAlignment = AUDIO_SAIx_DMAx_MEM_DATA_SIZE;
        hdma_sai_tx.Init.Mode = DMA_CIRCULAR;
        hdma_sai_tx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_sai_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_sai_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_sai_tx.Init.MemBurst = DMA_MBURST_SINGLE;
        hdma_sai_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;

        hdma_sai_tx.Instance = AUDIO_SAIx_DMAx_STREAM;

        /* Associate the DMA handle */
        __HAL_LINKDMA(hsai, hdmatx, hdma_sai_tx);

        /* Deinitialize the Stream for new transfer */
        HAL_DMA_DeInit(&hdma_sai_tx);

        /* Configure the DMA Stream */
        HAL_DMA_Init(&hdma_sai_tx);
    }

    /* SAI DMA IRQ Channel configuration */
    HAL_NVIC_SetPriority(AUDIO_SAIx_DMAx_IRQ, AUDIO_OUT_IRQ_PREPRIO, 0);
    HAL_NVIC_EnableIRQ(AUDIO_SAIx_DMAx_IRQ);
}

__weak void BSP_AUDIO_OUT_MspDeInit(SAI_HandleTypeDef *hsai, void *Params)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* SAI DMA IRQ Channel deactivation */
    HAL_NVIC_DisableIRQ(AUDIO_SAIx_DMAx_IRQ);

    if (hsai->Instance == AUDIO_SAIx)
    {
        /* Deinitialize the DMA stream */
        HAL_DMA_DeInit(hsai->hdmatx);
    }

    /* Disable SAI peripheral */
    __HAL_SAI_DISABLE(hsai);

    /* Put CS43L2 codec reset low -----------------------------------*/
    HAL_GPIO_WritePin(AUDIO_RESET_GPIO_PORT, AUDIO_RESET_PIN, GPIO_PIN_RESET);

    /* Deactives CODEC_SAI pins FS, SCK, MCK and SD by putting them in input mode */
    gpio_init_structure.Pin = AUDIO_SAIx_MCK_PIN;
    HAL_GPIO_DeInit(AUDIO_SAIx_MCLK_GPIO_PORT, gpio_init_structure.Pin);

    gpio_init_structure.Pin = AUDIO_SAIx_FS_PIN | AUDIO_SAIx_SCK_PIN | AUDIO_SAIx_SD_PIN;
    HAL_GPIO_DeInit(AUDIO_SAIx_SCK_SD_FS_GPIO_PORT, gpio_init_structure.Pin);

    gpio_init_structure.Pin = AUDIO_RESET_PIN;
    HAL_GPIO_DeInit(AUDIO_RESET_GPIO_PORT, gpio_init_structure.Pin);

    /* Disable SAI clock */
    AUDIO_SAIx_CLK_DISABLE();

    /* GPIO pins clock and DMA clock can be shut down in the applic 
     by surcgarging this __weak function */
}

__weak void BSP_AUDIO_OUT_ClockConfig(SAI_HandleTypeDef *hsai, uint32_t AudioFreq, void *Params)
{
    RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;

    HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

    /* Set the PLL configuration according to the audio frequency */
    if ((AudioFreq == AUDIO_FREQUENCY_11K) || (AudioFreq == AUDIO_FREQUENCY_22K) || (AudioFreq == AUDIO_FREQUENCY_44K))
    {
        /* Configure PLLI2S prescalers */
        /* PLLI2S_VCO: VCO_429M
    I2S_CLK(first level) = PLLI2S_VCO/PLLI2SQ = 429/2 = 214.5 Mhz
    I2S_CLK_x = I2S_CLK(first level)/PLLI2SDIVQ = 214.5/19 = 11.289 Mhz */
        rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI_PLLI2S;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 429;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SQ = 2;
        rcc_ex_clk_init_struct.PLLI2SDivQ = 19;

        HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
    }
    else /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, AUDIO_FREQUENCY_48K), AUDIO_FREQUENCY_96K */
    {
        /* SAI clock config 
    PLLSAI_VCO: VCO_344M 
    I2S_CLK(first level) = PLLI2S_VCO/PLLI2SQ = 344/7 = 49.142 Mhz 
    I2S_CLK_x = SAI_CLK(first level)/PLLI2SDIVQ = 49.142/1 = 49.142 Mhz */
        rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI_PLLI2S;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 344;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SQ = 7;
        rcc_ex_clk_init_struct.PLLI2SDivQ = 1;

        HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
    }
}

static uint8_t SAIx_Init(uint32_t AudioFreq)
{
    uint8_t ret = AUDIO_OK;

    /* Initialize the haudio_out_sai Instance parameter */
    haudio_out_sai.Instance = AUDIO_SAIx;

    /* Disable SAI peripheral to allow access to SAI internal registers */
    __HAL_SAI_DISABLE(&haudio_out_sai);

    /* Configure SAI_Block_x
  LSBFirst: Disabled
  DataSize: 16 */
    haudio_out_sai.Init.AudioFrequency = AudioFreq;
    haudio_out_sai.Init.ClockSource = SAI_CLKSOURCE_PLLI2S;
    haudio_out_sai.Init.AudioMode = SAI_MODEMASTER_TX;
    haudio_out_sai.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    haudio_out_sai.Init.Protocol = SAI_FREE_PROTOCOL;
    haudio_out_sai.Init.DataSize = SAI_DATASIZE_16;
    haudio_out_sai.Init.FirstBit = SAI_FIRSTBIT_MSB;
    haudio_out_sai.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    haudio_out_sai.Init.Synchro = SAI_ASYNCHRONOUS;
    haudio_out_sai.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    haudio_out_sai.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    /*
  haudio_out_sai.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;
  haudio_out_sai.Init.SynchroExt     = SAI_SYNCEXT_DISABLE;
  haudio_out_sai.Init.Mckdiv         = SAIClockDivider(AudioFreq);
  haudio_out_sai.Init.MonoStereoMode = SAI_STEREOMODE;
  haudio_out_sai.Init.CompandingMode = SAI_NOCOMPANDING;
  haudio_out_sai.Init.TriState       = SAI_OUTPUT_NOTRELEASED;
*/

    /* Configure SAI_Block_x Frame
  Frame Length: 64
  Frame active Length: 32
  FS Definition: Start frame + Channel Side identification
  FS Polarity: FS active Low
  FS Offset: FS asserted one bit before the first bit of slot 0 */
    haudio_out_sai.FrameInit.FrameLength = 64;
    haudio_out_sai.FrameInit.ActiveFrameLength = 32;
    haudio_out_sai.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    haudio_out_sai.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    haudio_out_sai.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;

    /* Configure SAI Block_x Slot
  Slot First Bit Offset: 0
  Slot Size  : 16
  Slot Number: 4
  Slot Active: All slot actives */
    haudio_out_sai.SlotInit.FirstBitOffset = 0;
    haudio_out_sai.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
    haudio_out_sai.SlotInit.SlotNumber = 4;
    haudio_out_sai.SlotInit.SlotActive = CODEC_AUDIOFRAME_SLOT_0123;

    /* Initializes the SAI peripheral*/
    if (HAL_SAI_Init(&haudio_out_sai) != HAL_OK)
    {
        ret = AUDIO_ERROR;
    }

    /* Enable SAI peripheral to generate MCLK */
    __HAL_SAI_ENABLE(&haudio_out_sai);

    return ret;
}

/**
  * @brief  Deinitializes the Audio Codec audio interface (SAI).
  */
static void SAIx_DeInit(void)
{
    /* Initialize the hAudioOutSai Instance parameter */
    haudio_out_sai.Instance = AUDIO_SAIx;

    /* Disable SAI peripheral */
    __HAL_SAI_DISABLE(&haudio_out_sai);

    HAL_SAI_DeInit(&haudio_out_sai);
}

static I2C_HandleTypeDef heval_I2c2;

static void I2C2_MspInit(void);
static void I2C2_Init(void);

static HAL_StatusTypeDef I2C2_ReadMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static HAL_StatusTypeDef I2C2_WriteMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddSize, uint8_t *Buffer, uint16_t Length);
static void I2C2_Error(uint8_t Addr);

void AUDIO_IO_Init(void);
void AUDIO_IO_DeInit(void);
void AUDIO_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
uint8_t AUDIO_IO_Read(uint8_t Addr, uint8_t Reg);
void AUDIO_IO_Delay(uint32_t Delay);

/**
  * @brief  Initializes I2C MSP.
  */
static void I2C2_MspInit(void)
{
    GPIO_InitTypeDef gpio_init_structure;

    /*** Configure the GPIOs ***/
    /* Enable GPIO clock */
    DISCO_I2C2_SCL_SDA_GPIO_CLK_ENABLE();

    /* Configure I2C Tx as alternate function */
    gpio_init_structure.Pin = DISCO_I2C2_SCL_PIN;
    gpio_init_structure.Mode = GPIO_MODE_AF_OD;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = DISCO_I2C2_SCL_SDA_AF;
    HAL_GPIO_Init(DISCO_I2C2_SCL_SDA_GPIO_PORT, &gpio_init_structure);

    /* Configure I2C Rx as alternate function */
    gpio_init_structure.Pin = DISCO_I2C2_SDA_PIN;
    HAL_GPIO_Init(DISCO_I2C2_SCL_SDA_GPIO_PORT, &gpio_init_structure);

    /*** Configure the I2C peripheral ***/
    /* Enable I2C clock */
    DISCO_I2C2_CLK_ENABLE();

    /* Force the I2C peripheral clock reset */
    DISCO_I2C2_FORCE_RESET();

    /* Release the I2C peripheral clock reset */
    DISCO_I2C2_RELEASE_RESET();

    /* Enable and set I2C1 Interrupt to a lower priority */
    HAL_NVIC_SetPriority(DISCO_I2C2_EV_IRQn, 0x05, 0);
    HAL_NVIC_EnableIRQ(DISCO_I2C2_EV_IRQn);

    /* Enable and set I2C1 Interrupt to a lower priority */
    HAL_NVIC_SetPriority(DISCO_I2C2_ER_IRQn, 0x05, 0);
    HAL_NVIC_EnableIRQ(DISCO_I2C2_ER_IRQn);
}

static HAL_StatusTypeDef I2C2_WriteMultiple(uint8_t Addr,
                                            uint16_t Reg,
                                            uint16_t MemAddress,
                                            uint8_t *Buffer,
                                            uint16_t Length)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Write(&heval_I2c2,
                               Addr,
                               (uint16_t)Reg,
                               MemAddress,
                               Buffer,
                               Length,
                               1000);

    if (status != HAL_OK)
    {
        I2C2_Error(Addr);
    }
    return status;
}

static void I2C2_Init(void)
{
    if (HAL_I2C_GetState(&heval_I2c2) == HAL_I2C_STATE_RESET)
    {
        heval_I2c2.Instance = I2C2;
        heval_I2c2.Init.ClockSpeed = I2C2_SCL_FREQ_KHZ;
        heval_I2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
        heval_I2c2.Init.OwnAddress1 = 0;
        heval_I2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        heval_I2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        heval_I2c2.Init.OwnAddress2 = 0;
        heval_I2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        heval_I2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

        /* Init the I2C */
        I2C2_MspInit();
        HAL_I2C_Init(&heval_I2c2);
    }
}

static HAL_StatusTypeDef I2C2_ReadMultiple(uint8_t Addr,
                                           uint16_t Reg,
                                           uint16_t MemAddress,
                                           uint8_t *Buffer,
                                           uint16_t Length)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Read(&heval_I2c2,
                              Addr,
                              (uint16_t)Reg,
                              MemAddress,
                              Buffer,
                              Length,
                              1000);

    if (status != HAL_OK)
    {
        I2C2_Error(Addr);
    }
    return status;
}

static void I2C2_Error(uint8_t Addr)
{
    HAL_I2C_DeInit(&heval_I2c2);
    I2C2_Init();
}

void AUDIO_IO_Init(void)
{
    I2C2_Init();
}

void AUDIO_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
    I2C2_WriteMultiple(Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&Value, 1);
}

uint8_t AUDIO_IO_Read(uint8_t Addr, uint8_t Reg)
{
    uint8_t read_value = 0;

    I2C2_ReadMultiple(Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&read_value, 1);

    return read_value;
}

void AUDIO_IO_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}
