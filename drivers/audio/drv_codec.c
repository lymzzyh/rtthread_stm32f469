/*
 * File      : drv_codec.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author         Notes
 * 2018-09-08     liu2guang      the first version for STM32F469NI-Discovery.
 */

#include "drv_codec.h" 
#include "drv_cs43l22.h"

/* Device Control Commands */
#define CODEC_CMD_RESET             0
#define CODEC_CMD_SET_VOLUME        1
#define CODEC_CMD_GET_VOLUME        2
#define CODEC_CMD_SAMPLERATE        3
#define CODEC_CMD_EQ                4
#define CODEC_CMD_3D                5
#define CODEC_CMD_SUSPEND           6
#define CODEC_CMD_RESUME            7

struct drv_sai
{
    SAI_HandleTypeDef hsai; 
    DMA_HandleTypeDef hdma;
}; 
static struct drv_sai sai = {0}; 

rt_err_t sai_init(rt_uint32_t freq)
{
    /* 调用时先关闭SAI再初始化 */ 
    __HAL_SAI_DISABLE(&sai.hsai);
    HAL_SAI_DeInit(&sai.hsai);
    
    /* Clock Tree Config */ 
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct; 

    if(freq == AUDIO_FREQUENCY_011K || freq == AUDIO_FREQUENCY_022K || freq == AUDIO_FREQUENCY_044K)
    {
        // PLLI2S_VCO: VCO_429M
        // I2S_CLK(first level) = PLLI2S_VCO/PLLI2SQ = 429/2 = 214.5 MHz
        // I2S_CLK_x = I2S_CLK(first level)/PLLI2SDIVQ = 214.5/19 = 11.289 MHz
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI_PLLI2S; 
        PeriphClkInitStruct.PLLI2S.PLLI2SN       = 429;
        PeriphClkInitStruct.PLLI2S.PLLI2SQ       = 2;
        PeriphClkInitStruct.PLLI2SDivQ           = 19;
    }
    else
    {
        // PLLI2S_VCO: VCO_344M
        // I2S_CLK(first level) = PLLI2S_VCO/PLLI2SQ = 344/7 = 49.142 MHz
        // I2S_CLK_x = SAI_CLK(first level)/PLLI2SDIVQ = 49.142/1 = 49.142 MHz
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI_PLLI2S; 
        PeriphClkInitStruct.PLLI2S.PLLI2SN       = 344;
        PeriphClkInitStruct.PLLI2S.PLLI2SQ       = 7;
        PeriphClkInitStruct.PLLI2SDivQ           = 1;
    }

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    
    /* 打开GPIO SAI DMA2外设时钟 */ 
    __HAL_RCC_GPIOG_CLK_ENABLE(); 
    __HAL_RCC_GPIOE_CLK_ENABLE(); 
    __HAL_RCC_SAI1_CLK_ENABLE(); 
    __HAL_RCC_DMA2_CLK_ENABLE(); 

    /* GPIO Config */ 
    GPIO_InitTypeDef GPIO_InitStruct;

    /* CODEC_SAI pins configuration: MCK pin */ 
    GPIO_InitStruct.Pin       = GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* CODEC_SAI pins configuration: FS, SCK, MCK and SD pins */ 
    GPIO_InitStruct.Pin       = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct); 

    /* DAM Config */ 
    sai.hdma.Instance                 = DMA2_Stream3;
    sai.hdma.Init.Channel             = DMA_CHANNEL_0;
    sai.hdma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    sai.hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
    sai.hdma.Init.MemInc              = DMA_MINC_ENABLE;
    sai.hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    sai.hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    sai.hdma.Init.Mode                = DMA_CIRCULAR;
    sai.hdma.Init.Priority            = DMA_PRIORITY_HIGH;
    sai.hdma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    sai.hdma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    sai.hdma.Init.MemBurst            = DMA_MBURST_SINGLE;
    sai.hdma.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    __HAL_LINKDMA(&sai.hsai, hdmatx, sai.hdma); 
    HAL_DMA_DeInit(&sai.hdma);
    HAL_DMA_Init(&sai.hdma); 

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

    /* SAI */ 
    sai.hsai.Instance = SAI1_Block_A;

    __HAL_SAI_DISABLE(&sai.hsai); 

    sai.hsai.Init.AudioFrequency         = freq;
    sai.hsai.Init.ClockSource            = SAI_CLKSOURCE_PLLI2S;
    sai.hsai.Init.AudioMode              = SAI_MODEMASTER_TX;
    sai.hsai.Init.NoDivider              = SAI_MASTERDIVIDER_ENABLE;
    sai.hsai.Init.Protocol               = SAI_FREE_PROTOCOL;
    sai.hsai.Init.DataSize               = SAI_DATASIZE_16;
    sai.hsai.Init.FirstBit               = SAI_FIRSTBIT_MSB;
    sai.hsai.Init.ClockStrobing          = SAI_CLOCKSTROBING_FALLINGEDGE;
    sai.hsai.Init.Synchro                = SAI_ASYNCHRONOUS;
    sai.hsai.Init.OutputDrive            = SAI_OUTPUTDRIVE_ENABLE;
    sai.hsai.Init.FIFOThreshold          = SAI_FIFOTHRESHOLD_1QF;

    sai.hsai.FrameInit.FrameLength       = 64;
    sai.hsai.FrameInit.ActiveFrameLength = 32;
    sai.hsai.FrameInit.FSDefinition      = SAI_FS_CHANNEL_IDENTIFICATION;
    sai.hsai.FrameInit.FSPolarity        = SAI_FS_ACTIVE_LOW;
    sai.hsai.FrameInit.FSOffset          = SAI_FS_BEFOREFIRSTBIT; 

    sai.hsai.SlotInit.FirstBitOffset     = 0;
    sai.hsai.SlotInit.SlotSize           = SAI_SLOTSIZE_DATASIZE;
    sai.hsai.SlotInit.SlotNumber         = 4;
    sai.hsai.SlotInit.SlotActive         = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1 | 
                                           SAI_SLOTACTIVE_2 | SAI_SLOTACTIVE_3;
    HAL_SAI_DeInit(&sai.hsai);
    
    if (HAL_SAI_Init(&sai.hsai) != HAL_OK)
    {
        return (-RT_ERROR); 
    }

    __HAL_SAI_DISABLE(&sai.hsai);
    __HAL_SAI_ENABLE(&sai.hsai);

    return RT_EOK; 
}

void DMA2_Stream3_IRQHandler(void) 
{
    HAL_DMA_IRQHandler(sai.hsai.hdmatx);
} 

// --------------------------------------------------------------------------------------
// Audio 
// --------------------------------------------------------------------------------------

#include <rtthread.h> 
#include <rtdevice.h>
#include <rthw.h>
#include <string.h>

struct rt_data_node
{
    char *data_ptr;
    rt_uint32_t data_size;
};

struct rt_data_node_list
{
    struct rt_data_node *node;
    rt_uint32_t size;
    rt_uint32_t read_index, write_index;
    rt_uint32_t read_offset;
    void (*read_complete)(struct rt_data_node *node, void *user_data);
    void *user_data;
};

struct audio_codec_device
{
    /* inherit from rt_device */
    struct rt_device parent;
    struct rt_data_node_list *node_list;

    char *send_fifo;
    rt_uint32_t stat;
    rt_uint32_t dma_irq_cnt;
};

#define AUDIO_SEND_BUFFER_SIZE 2048
#define codec_printf    rt_kprintf

struct audio_codec_device _g_audio_codec;

int rt_data_node_init(struct rt_data_node_list **node_list, rt_uint32_t size)
{
    int result = RT_EOK;
    struct rt_data_node_list *list = RT_NULL;
    struct rt_data_node *node = RT_NULL;

    list = rt_malloc(sizeof(struct rt_data_node_list));
    if (list == RT_NULL)
    {
        result = -RT_ENOMEM;
        goto __exit;
    }
    memset(list, 0, sizeof(struct rt_data_node_list));

    node = rt_malloc(sizeof(struct rt_data_node) * size);
    if (size == RT_NULL)
    {
        result = -RT_ENOMEM;
        goto __exit;
    }
    memset(node, 0, sizeof(struct rt_data_node));

    list->node = node;
    list->size = size;
    list->read_index = 0;
    list->write_index = 0;
    list->read_offset = 0;
    list->read_complete = RT_NULL;
    list->user_data = 0;

    *node_list = list;

    return result;

__exit:
    if (list)
        rt_free(list);

    if (node)
        rt_free(node);

    return result;
}

int rt_data_node_is_empty(struct rt_data_node_list *node_list)
{
    rt_uint32_t read_index, write_index;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    read_index = node_list->read_index;
    write_index = node_list->write_index;
    rt_hw_interrupt_enable(level);

    if (read_index == write_index)
    {
        return RT_TRUE;
    }
    else
    {
        return RT_FALSE;
    }
}

void wait_node_free(struct rt_data_node_list *node_list)
{
    while (node_list->read_index != node_list->write_index)
        rt_thread_delay(5);
}

int rt_data_node_write(struct rt_data_node_list *node_list, void *buffer, rt_uint32_t size)
{
    struct rt_data_node *node = RT_NULL;
    rt_uint32_t read_index, write_index, next_index;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    read_index = node_list->read_index;
    write_index = node_list->write_index;
    rt_hw_interrupt_enable(level);

    next_index = write_index + 1;
    if (next_index >= node_list->size)
        next_index = 0;

    if (next_index == read_index)
    {
        rt_kprintf("[node]:node list full, write index = %d, read index = %d \n", write_index, read_index);
        return -RT_ERROR;
    }

    level = rt_hw_interrupt_disable();
    /* set node attribute */
    node = &node_list->node[write_index];
    node->data_ptr = (char *) buffer;
    node->data_size = size;
    node_list->write_index = next_index;
    rt_hw_interrupt_enable(level);

    return size;
}

int rt_data_node_read(struct rt_data_node_list *node_list, void *buffer, rt_uint32_t size)
{
    struct rt_data_node *node = RT_NULL;
    rt_uint32_t read_index, write_index, next_index;
    rt_uint32_t offset, remain_len;
    rt_base_t level;
    rt_uint32_t result;

    level = rt_hw_interrupt_disable();
    read_index = node_list->read_index;
    write_index = node_list->write_index;
    rt_hw_interrupt_enable(level);

    if (read_index == write_index)
    {
        result = 0;
    }
    else
    {
        node = &node_list->node[node_list->read_index];
        offset = node_list->read_offset;
        remain_len = node->data_size - offset;

        if (remain_len > size)
        {
            memcpy(buffer, node->data_ptr + offset, size);
            node_list->read_offset += size;
            result = size;
        }
        else
        {
            level = rt_hw_interrupt_disable();
            read_index = node_list->read_index;
            write_index = node_list->write_index;
            rt_hw_interrupt_enable(level);

            next_index = read_index + 1;
            if (next_index >= node_list->size)
                next_index = 0;

            memcpy(buffer, node->data_ptr + offset, remain_len);
            node_list->read_offset += remain_len;
            /* notify transmitted complete. */
            if (node_list->read_complete != RT_NULL)
            {
                node_list->read_complete(node, node_list->user_data);
            }

            if (next_index == write_index)
            {
                node_list->read_index = next_index;
                node_list->read_offset = 0;
                result = remain_len;
            }
            else
            {
                node = &node_list->node[next_index];
                memcpy((char *)buffer + remain_len, node->data_ptr, size - remain_len);
                node_list->read_index = next_index;
                node_list->read_offset = size - remain_len;
                result = size;
            }
        }
    }

    return result;
}

static rt_err_t audio_codec_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t audio_codec_open(rt_device_t dev, rt_uint16_t oflag)
{
    if(cs43l22_play() != RT_EOK)
    {  
        return (-RT_ERROR);
    }
    
    return RT_EOK;
}

static rt_err_t audio_codec_close(rt_device_t dev)
{
    if(cs43l22_stop() != RT_EOK)
    {  
        return (-RT_ERROR);
    }
    
    rt_thread_mdelay(2);
    
    return RT_EOK; 
}

static rt_size_t audio_codec_write(rt_device_t dev, rt_off_t pos,
                                   const void *buffer, rt_size_t size)
{
    int ret;
    struct audio_codec_device *audio = RT_NULL;

    audio = (struct audio_codec_device *)dev;
    ret = rt_data_node_write(audio->node_list, (void *)buffer, size);

    return ret;
}

static rt_err_t audio_codec_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    
    switch (cmd)
    {
    case CODEC_CMD_SET_VOLUME:
    {
        rt_uint32_t volume = *(rt_uint32_t *)args;

        rt_kprintf("set volume %d \n", volume);

        cs43l22_set_volume((rt_uint8_t)volume); 
        break;
    }

    case CODEC_CMD_SAMPLERATE:
    {
        rt_uint32_t freq = *(rt_uint32_t *)args;

        rt_kprintf("set  dac sample rate %d \n", freq);
        
        /* Todo: 设置波特率 */ 

        break;
    }

    default:
        result = RT_ERROR;
    }

    return result;
}

static void data_node_read_complete(struct rt_data_node *node, void *user_data)
{
    struct rt_device *dev = RT_NULL;

    dev = (struct rt_device *)user_data;
    if (dev->tx_complete != RT_NULL)
    {
        dev->tx_complete(dev, node->data_ptr);
    }
}

static int print_cnt = 0;
void _dma_half_handler(void)
{
    int result;
    struct audio_codec_device *audio = RT_NULL;

    audio = &_g_audio_codec;
    result = rt_data_node_is_empty(audio->node_list);
    if (result)
    {
        // rt_kprintf("#");
        print_cnt++;
        memset(audio->send_fifo, 0, AUDIO_SEND_BUFFER_SIZE / 2);
    }
    else
    {
        print_cnt = 0;
        memset(audio->send_fifo, 0, AUDIO_SEND_BUFFER_SIZE / 2);
        rt_data_node_read(audio->node_list, audio->send_fifo, AUDIO_SEND_BUFFER_SIZE / 2);
    }
}

void _dma_finish_handler(void)
{
    int result;
    struct audio_codec_device *audio = RT_NULL;

    audio = &_g_audio_codec;
    audio->dma_irq_cnt++;
    result = rt_data_node_is_empty(audio->node_list);
    if (result)
    {
        // rt_kprintf("*");
        memset(audio->send_fifo + (AUDIO_SEND_BUFFER_SIZE / 2), 0, AUDIO_SEND_BUFFER_SIZE / 2);
    }
    else
    {
        memset(audio->send_fifo + (AUDIO_SEND_BUFFER_SIZE / 2), 0, AUDIO_SEND_BUFFER_SIZE / 2);
        rt_data_node_read(audio->node_list, audio->send_fifo + (AUDIO_SEND_BUFFER_SIZE / 2), AUDIO_SEND_BUFFER_SIZE / 2);
    }
}

/* SAI DMA半传输回调 */ 
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    _dma_half_handler(); 
}

/* SAI DMA全部传输完成回调  */ 
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    _dma_finish_handler(); 
}


int rt_audio_codec_hw_init(void)
{
    int result = RT_EOK;
    struct audio_codec_device *audio = &_g_audio_codec;

    audio->send_fifo = rt_malloc(AUDIO_SEND_BUFFER_SIZE);
    if (audio->send_fifo == RT_NULL)
    {
        result = -RT_ENOMEM;
        goto __exit;
    }
    memset(audio->send_fifo, 0, AUDIO_SEND_BUFFER_SIZE);

    rt_data_node_init(&audio->node_list, 100);
    audio->node_list->read_complete = data_node_read_complete;
    audio->node_list->user_data = audio;

    audio->parent.type = RT_Device_Class_Sound;
    audio->parent.rx_indicate = RT_NULL;
    audio->parent.tx_complete = RT_NULL;
    audio->parent.user_data   = RT_NULL;

    audio->parent.control = audio_codec_control;
    audio->parent.init    = audio_codec_init;
    audio->parent.open    = audio_codec_open;
    audio->parent.close   = audio_codec_close;
    audio->parent.read    = RT_NULL;
    audio->parent.write   = audio_codec_write;

    /* register the device */
    rt_device_register(&audio->parent, "sound", RT_DEVICE_FLAG_WRONLY | RT_DEVICE_FLAG_DMA_TX);

    cs43l22_init("i2c2", 1, 0x94>>1, cs43l22_output_headphone, 65); 
    sai_init(AUDIO_FREQUENCY_022K);
    HAL_SAI_Transmit_DMA(&sai.hsai, (uint8_t *)(audio->send_fifo), AUDIO_SEND_BUFFER_SIZE/2); 
    
    rt_device_init(&audio->parent);

    return RT_EOK;

__exit:
    if (audio->send_fifo)
    {
        rt_free(audio->send_fifo);
        audio->send_fifo = RT_NULL;
    }

    return result;
}

INIT_DEVICE_EXPORT(rt_audio_codec_hw_init);

