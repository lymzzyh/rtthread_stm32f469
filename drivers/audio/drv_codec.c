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

struct rt_audio_device audio; 

static rt_err_t getcaps(struct rt_audio_device *audio,struct rt_audio_caps *caps)
{
    return RT_EOK; 
}

static rt_err_t configure(struct rt_audio_device *audio,struct rt_audio_caps *caps)
{
    return RT_EOK; 
}

static rt_err_t init(struct rt_audio_device *audio)
{
    return RT_EOK; 
}

static rt_err_t shutdown(struct rt_audio_device *audio)
{
    return RT_EOK; 
}

static rt_err_t start(struct rt_audio_device *audio,int stream)
{
    return RT_EOK; 
}

static rt_err_t stop(struct rt_audio_device *audio,int stream)
{
    return RT_EOK; 
}

static rt_err_t suspend(struct rt_audio_device *audio,int stream)
{
    return RT_EOK; 
}

static rt_err_t resume(struct rt_audio_device *audio,int stream)
{
    return RT_EOK; 
}

static rt_err_t control(struct rt_audio_device *audio, int cmd, void *args)
{
    return RT_EOK; 
}

static rt_size_t transmit(struct rt_audio_device *audio, const void *writeBuf, void *readBuf, rt_size_t size)
{
    return 0; 
}

const struct rt_audio_ops ops = 
{
    .getcaps    = getcaps,
    .configure  = configure,

    .init       = init,
    .shutdown   = shutdown,
    .start      = start,
    .stop       = stop,
    .suspend    = suspend,
    .resume     = resume,
    .control    = control,

    .transmit   = transmit,
}; 

int rt_hw_codec_init(void)
{
    rt_err_t ret = RT_EOK; 

    rt_memset(&audio, 0x00, sizeof(struct rt_audio_device)); 

    audio.ops = (struct rt_audio_ops*)&ops; 

    // RT_DEVICE_FLAG_RDONLY : 录音设备
    // RT_DEVICE_FLAG_WRONLY : 放音设备
    // RT_DEVICE_FLAG_RDWR   : 声卡设备
    ret = rt_audio_register(&audio, "sound0", RT_DEVICE_FLAG_WRONLY, RT_NULL); 
    if(ret != RT_EOK)
    {
        rt_kprintf("Register sound\n"); 
        return ret; 
    }

    return RT_EOK; 
}
INIT_DEVICE_EXPORT(rt_hw_codec_init); 
