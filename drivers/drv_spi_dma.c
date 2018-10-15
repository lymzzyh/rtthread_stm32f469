/*
 * File      : drv_spi_dma.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006-2013, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-15     liu2guang    the first version.
 */
 
#include "drv_spi_dma.h" 

struct drv_spi_cs
{
    rt_uint32_t pin; 
};

struct drv_spi
{
    SPI_HandleTypeDef hspi;
    
    DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
    
    struct rt_spi_configuration *cfg; 
    volatile rt_uint8_t tran_state;
};
static struct drv_spi spi1 = {0}; 
static struct drv_spi spi2 = {0}; 
static struct drv_spi spi3 = {0}; 

static struct rt_spi_bus spi1_bus = {0}; 
static struct rt_spi_bus spi2_bus = {0}; 
static struct rt_spi_bus spi3_bus = {0}; 

void SPIx_DMA_RX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(spi1.hspi.hdmarx);
}

void SPIx_DMA_TX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(spi1.hspi.hdmatx);
}

void SPIx_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&spi1.hspi);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    struct drv_spi *spi = (struct drv_spi *)hspi; 
    
    spi->tran_state = 1; 
}

static rt_err_t _spi_init(struct drv_spi *spi, struct rt_spi_configuration *cfg)
{
    RT_ASSERT(cfg != RT_NULL); 
    
    if(cfg->data_width != 8 && cfg->data_width != 16 && cfg->data_width != 32)
    {
        return (-RT_EINVAL); 
    }
    
    if(spi == &spi1)
    {
        spi1.cfg = cfg; 
        spi1.tran_state = 0; 
        
        /* GPIO Init */ 
        GPIO_InitTypeDef GPIO_InitStruct;
        
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        
        GPIO_InitStruct.Pin       = GPIO_PIN_5;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin       = GPIO_PIN_4 | GPIO_PIN_5;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        /* SPI Init */ 
        if (cfg->mode & RT_SPI_CPHA)
            spi1.hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
        else 
            spi1.hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
        
        if (cfg->mode & RT_SPI_CPOL)
            spi1.hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
        else 
            spi1.hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
        
        if (cfg->mode & RT_SPI_MSB)
            spi1.hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
        else
            spi1.hspi.Init.FirstBit = SPI_FIRSTBIT_LSB;
        
        if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 2)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
        else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 4)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
        else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 8)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
        else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 16)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
        else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 32)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
        else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 64)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
        else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 128)
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
        else
            spi1.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
        
        spi1.hspi.Instance            = SPI1; 
        spi1.hspi.Init.Mode           = SPI_MODE_MASTER;
        spi1.hspi.Init.Direction      = SPI_DIRECTION_2LINES;
        spi1.hspi.Init.DataSize       = SPI_DATASIZE_8BIT;
        spi1.hspi.Init.NSS            = SPI_NSS_SOFT;
        spi1.hspi.Init.TIMode         = SPI_TIMODE_DISABLE;
        spi1.hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        spi1.hspi.Init.CRCPolynomial  = 7;
        spi1.hspi.State               = HAL_SPI_STATE_RESET; 
        
        /* DMA Init */ 
        __HAL_RCC_DMA2_CLK_ENABLE(); 
        
        spi1.hdma_tx.Instance                 = DMA2_Stream3;
        spi1.hdma_tx.Init.Channel             = DMA_CHANNEL_3;
        spi1.hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        spi1.hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        spi1.hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
        spi1.hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;
        spi1.hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        spi1.hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        spi1.hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
        spi1.hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        spi1.hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        spi1.hdma_tx.Init.Mode                = DMA_NORMAL;
        spi1.hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
        
        __HAL_LINKDMA(&spi1.hspi, hdmatx, spi1.hdma_tx);
        HAL_DMA_Init(&spi1.hdma_tx);
        
        spi1.hdma_rx.Instance                 = DMA2_Stream0;
        spi1.hdma_rx.Init.Channel             = DMA_CHANNEL_3;
        spi1.hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        spi1.hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        spi1.hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
        spi1.hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;
        spi1.hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        spi1.hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        spi1.hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
        spi1.hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        spi1.hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        spi1.hdma_rx.Init.Mode                = DMA_NORMAL;
        spi1.hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;

        __HAL_LINKDMA(&spi1.hspi, hdmarx, spi1.hdma_rx);
        HAL_DMA_Init(&spi1.hdma_rx);
        
        HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 1, 1);
        HAL_NVIC_EnableIRQ  (DMA2_Stream3_IRQn);
        
        HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ  (DMA2_Stream0_IRQn); 
        
        HAL_NVIC_SetPriority(SPI1_IRQn, 1, 2);
        HAL_NVIC_EnableIRQ  (SPI1_IRQn); 
        
        if (HAL_SPI_Init(&spi1.hspi) != HAL_OK)
        {
            return (-RT_EIO);
        }
        __HAL_SPI_ENABLE(&spi1.hspi);
    }
    else if(spi == &spi2)
    {
    
    }
    
    else if(spi == &spi3)
    {
    
    }
    
    return RT_EOK; 
}

rt_err_t stm32_spi_bus_attach_device(const char *bus_name, const char *device_name, rt_uint32_t pin)
{
    rt_err_t ret = RT_EOK; 
    
    struct rt_spi_device *spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device)); 
    RT_ASSERT(spi_device != RT_NULL); 

    struct drv_spi_cs *cs_pin = (struct drv_spi_cs *)rt_malloc(sizeof(struct drv_spi_cs)); 
    RT_ASSERT(cs_pin != RT_NULL);
    
    cs_pin->pin = pin;
    rt_pin_mode(pin, PIN_MODE_OUTPUT); 
    rt_pin_write(pin, PIN_HIGH); 
    
    ret = rt_spi_bus_attach_device(spi_device, device_name, bus_name, (void *)cs_pin); 
    
    return ret; 
}

static rt_err_t spi_configure(struct rt_spi_device *device, struct rt_spi_configuration *cfg)
{
    rt_err_t ret = RT_EOK; 
    struct drv_spi *spi = RT_NULL; 
    
    RT_ASSERT(cfg != RT_NULL);
    RT_ASSERT(device != RT_NULL);
    
    spi = (struct drv_spi *)(device->bus->parent.user_data); 
    spi->cfg = cfg; 
    ret = _spi_init(spi, cfg); 
    
    return ret;
}

static rt_uint32_t spixfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);

    struct drv_spi *spi = (struct drv_spi *)(device->bus->parent.user_data); 
    struct drv_spi_cs *cs = device->parent.user_data; 

    if(message->cs_take)
    {
        rt_pin_write(cs->pin, PIN_LOW);
    }
    
    const rt_uint8_t *sndb = message->send_buf;
    rt_uint8_t *rcvb       = message->recv_buf;
    rt_int32_t  length     = message->length;
    
    HAL_SPI_TransmitReceive_DMA(&(spi->hspi), (uint8_t*)sndb, (uint8_t *)rcvb, length);
    
    while(spi->tran_state == 0); 
    spi->tran_state = 0; 
    
    if(message->cs_release)
    {
        rt_pin_write(cs->pin, PIN_HIGH);
    }

    return message->length; 
}

static struct rt_spi_ops stm32_spi_ops = 
{
    .configure = spi_configure, 
    .xfer      = spixfer
}; 
 
int rt_hw_spi_init(void)
{
    spi1_bus.parent.user_data = &spi1; 
    rt_spi_bus_register(&spi1_bus, "spi1", &stm32_spi_ops); 
    
    return RT_EOK; 
}
INIT_DEVICE_EXPORT(rt_hw_spi_init); 
