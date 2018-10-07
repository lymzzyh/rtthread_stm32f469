/*
 * File      : drv_codec.h
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

#ifndef __DRV_CODEC_H_ 
#define __DRV_CODEC_H_ 

#include <rtthread.h> 
#include <rtdevice.h>
#include <board.h> 

#define CODEC_PDWN_HW                 1
#define CODEC_PDWN_SW                 2

#define AUDIO_FREQUENCY_192K ((rt_uint32_t)192000)
#define AUDIO_FREQUENCY_096K ((rt_uint32_t) 96000)
#define AUDIO_FREQUENCY_048K ((rt_uint32_t) 48000)
#define AUDIO_FREQUENCY_044K ((rt_uint32_t) 44100)
#define AUDIO_FREQUENCY_032K ((rt_uint32_t) 32000)
#define AUDIO_FREQUENCY_022K ((rt_uint32_t) 22050)
#define AUDIO_FREQUENCY_016K ((rt_uint32_t) 16000)
#define AUDIO_FREQUENCY_011K ((rt_uint32_t) 11025)
#define AUDIO_FREQUENCY_008K ((rt_uint32_t)  8000)

#endif
