/*
 * File      : drv_sdio_adapter.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rtthread.h>
#include <rthw.h>
#include <drivers/mmcsd_core.h>
#include <drivers/sdio.h>

#include "board.h"

#ifdef PKG_USING_STM32_SDIO

#include <stm32_sdio.h>

// #define DBG_ENABLE
#define DBG_SECTION_NAME    "SDIO"
#define DBG_COLOR
#define DBG_LEVEL           DBG_LOG
#include <rtdbg.h>

static DMA_HandleTypeDef SDTxDMAHandler;
static DMA_HandleTypeDef SDRxDMAHandler;

static struct rt_mmcsd_host *host;

void SDIO_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    /* Process All SDIO Interrupt Sources */
    rthw_sdio_irq_process(host);

    /* leave interrupt */
    rt_interrupt_leave();
}
/**
  * @brief  Configures the DMA2 Channel4 for SDIO Tx request.
  * @param  BufferSRC: pointer to the source buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
void SD_LowLevel_DMA_TxConfig(uint32_t *src, uint32_t *dst, uint32_t BufferSize)
{
    //LOG_I("DMA TX src:0x%08X, dst:0x%08X, len:0x%08X",src,dst,BufferSize);
    static uint32_t size = 0;
    size += BufferSize*4;
    SDTxDMAHandler.Instance                 = DMA2_Stream6;
    SDTxDMAHandler.Init.Channel             = DMA_CHANNEL_4;
    SDTxDMAHandler.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    SDTxDMAHandler.Init.PeriphInc           = DMA_PINC_DISABLE;
    SDTxDMAHandler.Init.MemInc              = DMA_MINC_ENABLE;
    SDTxDMAHandler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    SDTxDMAHandler.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    SDTxDMAHandler.Init.Mode                = DMA_PFCTRL;
    SDTxDMAHandler.Init.Priority            = DMA_PRIORITY_MEDIUM;
    SDTxDMAHandler.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    SDTxDMAHandler.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    SDTxDMAHandler.Init.MemBurst            = DMA_MBURST_INC4;
    SDTxDMAHandler.Init.PeriphBurst         = DMA_PBURST_INC4;
    //DMA_PFCTRL
    HAL_DMA_DeInit(&SDTxDMAHandler);
    HAL_DMA_Init(&SDTxDMAHandler);
    
    HAL_DMA_Start(&SDTxDMAHandler, (uint32_t)src, (uint32_t)dst, BufferSize);
}

/**
  * @brief  Configures the DMA2 Channel4 for SDIO Rx request.
  * @param  BufferDST: pointer to the destination buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
void SD_LowLevel_DMA_RxConfig(uint32_t *src, uint32_t *dst, uint32_t BufferSize)
{
    //LOG_I("DMA RX src:0x%08X, dst:0x%08X, len:0x%08X",src,dst,BufferSize);    
    
    SDRxDMAHandler.Instance                 = DMA2_Stream3;
    SDRxDMAHandler.Init.Channel             = DMA_CHANNEL_4;
    SDRxDMAHandler.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    SDRxDMAHandler.Init.PeriphInc           = DMA_PINC_DISABLE;
    SDRxDMAHandler.Init.MemInc              = DMA_MINC_ENABLE;
    SDRxDMAHandler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    SDRxDMAHandler.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    SDRxDMAHandler.Init.Mode                = DMA_PFCTRL;
    SDRxDMAHandler.Init.Priority            = DMA_PRIORITY_MEDIUM;
    SDRxDMAHandler.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    SDRxDMAHandler.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    SDRxDMAHandler.Init.MemBurst            = DMA_MBURST_INC4;
    SDRxDMAHandler.Init.PeriphBurst         = DMA_PBURST_INC4;

    HAL_DMA_DeInit(&SDRxDMAHandler);
    HAL_DMA_Init(&SDRxDMAHandler);

    HAL_DMA_Start(&SDRxDMAHandler, (uint32_t)src, (uint32_t)dst, BufferSize);
}

void SD_LowLevel_Init(void)
{
    GPIO_InitTypeDef  GPIO_Init_Structure;
    // clock enable
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_SDIO_CLK_ENABLE();
    HAL_NVIC_SetPriority(SDIO_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
    // init sdio hardware
    

    /* Setup GPIO pins for SDIO data & clock */
    /**SDMMC1 GPIO Configuration
    PC8     ------> SDMMC1_D0
    PC9     ------> SDMMC1_D1
    PC10     ------> SDMMC1_D2
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PD2     ------> SDMMC1_CMD
    */
    GPIO_Init_Structure.Mode = GPIO_MODE_AF_PP;
    GPIO_Init_Structure.Pull = GPIO_PULLUP;
    GPIO_Init_Structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init_Structure.Alternate = GPIO_AF12_SDIO;
    
    GPIO_Init_Structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
                          | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &GPIO_Init_Structure);

    GPIO_Init_Structure.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);
}

static rt_uint32_t stm32_sdio_clock_get(struct stm32_sdio *hw_sdio)
{
    return 48000000UL;
    //return HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDIO);
}

static rt_err_t DMA_TxConfig(rt_uint32_t *src, rt_uint32_t *dst, int Size)
{
    SD_LowLevel_DMA_TxConfig((uint32_t *)src, (uint32_t *)dst, Size / 4);
    return RT_EOK;
}

static rt_err_t DMA_RxConfig(rt_uint32_t *src, rt_uint32_t *dst, int Size)
{
    SD_LowLevel_DMA_RxConfig((uint32_t *)src, (uint32_t *)dst, Size / 4);
    return RT_EOK;
}

int stm32f4xx_sdio_init(void)
{
    struct stm32_sdio_des sdio_des;

    SD_LowLevel_Init();

    sdio_des.clk_get = stm32_sdio_clock_get;
    sdio_des.hw_sdio = (struct stm32_sdio *)SDIO;
    sdio_des.rxconfig = DMA_RxConfig;
    sdio_des.txconfig = DMA_TxConfig;

    host = sdio_host_create(&sdio_des);
    if (host == RT_NULL)
    {
        LOG_E("host create fail\n");
        return -1;
    }

    return 0;
}
INIT_DEVICE_EXPORT(stm32f4xx_sdio_init);

#endif
