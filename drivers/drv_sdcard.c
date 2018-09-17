/*
 * @File:   drv_sdcard.c 
 * @Author: liuguang 
 * @Date:   2017-10-02 23:48:19 
 * 
 * @LICENSE: 
 * The code owner is liuguang. Other personnel may 
 * not be used or modified.
 * 
 * Change Logs: 
 * Date           Author       Notes 
 * 2017-10-02     liuguang     The first version. 
 */ 
 
#include "drv_sdcard.h"
#include "stm32f4xx.h"

#define RT_SDCARD_DMA_MODE 0 

#ifndef SD_TIMEOUT
#define SD_TIMEOUT ((uint32_t)0x10000000)
#endif
#ifndef SDCARD_SECTOR_SIZE
#define SDCARD_SECTOR_SIZE 512
#endif
#ifndef RT_SDCARD_DMA_MODE
#define RT_SDCARD_DMA_MODE 0
#endif

static SD_HandleTypeDef hsdcard; 

#if RT_SDCARD_DMA_MODE
static DMA_HandleTypeDef hdma_rx; 
static DMA_HandleTypeDef hdma_tx; 
#endif

rt_err_t sdcard_init(void)
{
    hsdcard.Instance = SDIO;

    hsdcard.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING; 
    hsdcard.Init.ClockBypass         = SDIO_CLOCK_BYPASS_ENABLE; 
    hsdcard.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE; 
    hsdcard.Init.BusWide             = SDIO_BUS_WIDE_1B; 
    hsdcard.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE; 
    hsdcard.Init.ClockDiv            = 5; 

    if(HAL_SD_Init(&hsdcard) != HAL_OK)
    {
        return RT_ERROR;
    }
    
    if(HAL_SD_ConfigWideBusOperation(&hsdcard, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
        HAL_SD_DeInit(&hsdcard);
        return RT_ERROR;
    }

    return RT_EOK;
}

void HAL_SD_MspInit(SD_HandleTypeDef *hsdcard)
{
    DMA_HandleTypeDef TxDMAHandler; 
    DMA_HandleTypeDef RxDMAHandler; 
    GPIO_InitTypeDef  GPIO_Init_Structure;

    /* gpio and sdio and dma clock enable */
    __HAL_RCC_SDIO_CLK_ENABLE(); 
    __HAL_RCC_GPIOC_CLK_ENABLE(); 
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* common gpio configuration */
    GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP; 
    GPIO_Init_Structure.Pull      = GPIO_PULLUP; 
    GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH; 
    GPIO_Init_Structure.Alternate = GPIO_AF12_SDIO;

    /* sdcard configuration D0 ~ D3, CLK */
    GPIO_Init_Structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | 
                              GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &GPIO_Init_Structure); 

    /* sdcard configuration CMD */
    GPIO_Init_Structure.Pin = GPIO_PIN_2; 
    HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure); 

#if (RT_SDCARD_DMA_MODE == 1)
    __HAL_RCC_DMA2_CLK_ENABLE();
    
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);

    hdma_rx.Instance = DMA2_Stream3;

    hdma_rx.Init.Channel             = DMA_CHANNEL_4;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_rx.Init.Mode                = DMA_PFCTRL;
    hdma_rx.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

    __HAL_LINKDMA(hsdcard, hdmarx, hdma_rx);
    HAL_DMA_DeInit(&hdma_rx);
    HAL_DMA_Init(&hdma_rx);

    hdma_tx.Instance                 = DMA2_Stream6;
    hdma_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_tx.Init.Mode                = DMA_PFCTRL;
    hdma_tx.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;

    __HAL_LINKDMA(hsdcard, hdmatx, hdma_tx);
    HAL_DMA_DeInit(&hdma_tx);
    HAL_DMA_Init(&hdma_tx);

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 3, 0); 
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 3, 0); 
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
#endif
}

rt_uint8_t sdcard_read_blocks(rt_uint8_t *buf, uint32_t sector, rt_uint32_t cnt)
{
    HAL_StatusTypeDef ret = HAL_OK;
    uint32_t timeout = 0; 
    HAL_SD_StateTypeDef state_return;
    HAL_SD_CardStateTypeDef sd_card_state_return;

#if (RT_SDCARD_DMA_MODE==1)
    ret = HAL_SD_ReadBlocks_DMA(&hsdcard, buf, sector, cnt); 
    if(ret != HAL_OK)
    {
        return RT_ERROR;
    }
    
#else
    ret = HAL_SD_ReadBlocks(&hsdcard, buf, sector, cnt, SD_TIMEOUT);
    if(ret != HAL_OK)
    {
        return RT_ERROR;
    }
#endif
    
    do
    {
        state_return = HAL_SD_GetState(&hsdcard);
        timeout++;
    }while((HAL_SD_STATE_BUSY == state_return) && (SD_TIMEOUT > timeout));
    
    if(HAL_SD_STATE_READY != state_return)
    {
        return RT_ERROR; 
    }
    
    do
    {
        sd_card_state_return = HAL_SD_GetCardState(&hsdcard);
        timeout++;
    }while((HAL_SD_CARD_TRANSFER != sd_card_state_return) && (SD_TIMEOUT > timeout));
    
    if(SD_TIMEOUT <= timeout)
    {
        return RT_ETIMEOUT; 
    }

    return RT_EOK;
}

rt_uint8_t sdcard_write_blocks(rt_uint8_t *buf, uint32_t sector, rt_uint32_t cnt)
{
    HAL_StatusTypeDef ret = HAL_OK;
    uint32_t timeout = 0;
    HAL_SD_StateTypeDef state_return;
    HAL_SD_CardStateTypeDef sd_card_state_return;
    
#if (RT_SDCARD_DMA_MODE==1)
    ret = HAL_SD_WriteBlocks_DMA(&hsdcard, buf, sector, cnt); 
    if(ret != HAL_OK)
    {
        return RT_ERROR;
    }
#else
    ret = HAL_SD_WriteBlocks(&hsdcard, buf, sector, cnt, SD_TIMEOUT);
    if(ret != HAL_OK)
    {
        return RT_ERROR;
    }
#endif
    
    do
    {
        state_return = HAL_SD_GetState(&hsdcard);
        timeout++;
    }while((HAL_SD_STATE_BUSY == state_return) && (SD_TIMEOUT > timeout));
    
    if(HAL_SD_STATE_READY != state_return)
    {
        return RT_ERROR; 
    }
    
    do
    {
        sd_card_state_return = HAL_SD_GetCardState(&hsdcard);
        timeout++;
    }while((HAL_SD_CARD_TRANSFER != sd_card_state_return) && (SD_TIMEOUT > timeout));
    
    if(SD_TIMEOUT <= timeout)
    {
        return RT_ETIMEOUT; 
    }

    return RT_EOK;
}

#if (RT_SDCARD_DMA_MODE==1)
void SDMMC1_IRQHandler(void)
{
    HAL_SD_IRQHandler(&hsdcard);
}

void DMA2_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(hsdcard.hdmatx);
}

void DMA2_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(hsdcard.hdmarx);
}
#endif

static struct rt_device sdcard_device;
static struct rt_semaphore sd_lock;

/* RT-Thread Device Driver Interface */
static rt_err_t rt_sdcard_init(rt_device_t dev)
{
    if (rt_sem_init(&sd_lock, "sdlock", 1, RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t rt_sdcard_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_sdcard_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t rt_sdcard_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    rt_uint8_t ret = RT_EOK;

    rt_sem_take(&sd_lock, RT_WAITING_FOREVER);
    ret = sdcard_read_blocks((rt_uint8_t *)buffer, pos, size);
    rt_sem_release(&sd_lock);

    if(ret == RT_EOK) 
    {
        return (rt_size_t)size;
    }

    rt_kprintf("read failed: %d, buffer 0x%08x\n", ret, buffer);
    return 0;
}

static rt_size_t rt_sdcard_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    rt_uint8_t ret = RT_EOK;

    rt_sem_take(&sd_lock, RT_WAITING_FOREVER);
    ret = sdcard_write_blocks((rt_uint8_t *)buffer, pos, size);
    rt_sem_release(&sd_lock);

    if(ret == RT_EOK) 
    {
        return (rt_size_t)size;
    }

    rt_kprintf("write failed: %d, buffer 0x%08x\n", ret, buffer);
    return 0;
}

static rt_err_t rt_sdcard_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);

    if(cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;

        geometry->bytes_per_sector = 512;
        geometry->block_size       = hsdcard.SdCard.BlockSize;
        geometry->sector_count     = hsdcard.SdCard.BlockNbr; 
    }

    return RT_EOK;
}

int rt_hw_sdcard_init(void)
{
    rt_err_t ret = RT_EOK;

    ret = sdcard_init();
    if(ret != RT_EOK)
    {
        return RT_ERROR;
    }

    /* register sdcard device */
    sdcard_device.type = RT_Device_Class_Block;

    sdcard_device.init    = rt_sdcard_init;
    sdcard_device.open    = rt_sdcard_open;
    sdcard_device.close   = rt_sdcard_close;
    sdcard_device.read    = rt_sdcard_read;
    sdcard_device.write   = rt_sdcard_write;
    sdcard_device.control = rt_sdcard_control;

    ret = rt_device_register(&sdcard_device, "sd0",
        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
    if(ret != RT_EOK)
    {
        /* Deinit code */
        return RT_ERROR;
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_sdcard_init);
