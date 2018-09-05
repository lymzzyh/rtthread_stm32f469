/*
 * File      : main.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-07-29     Arda.Fu      first implementation
 */
#include <rtthread.h>
#include <board.h>

#include "drv_codec.h"

#if defined(BSP_USING_SDCARD_MOUNT)
#include <dfs_file.h>
#include <dfs_posix.h>
#endif

struct rt_audio *haudio = RT_NULL; 

typedef enum 
{
    BUFFER_OFFSET_NONE = 0, 
    BUFFER_OFFSET_HALF, 
    BUFFER_OFFSET_FULL, 
} BUFFER_StateTypeDef;

#define AUDIO_BUFFER_SIZE 8192
#define WAV_FILE_NAME     "./3.wav"

struct rt_audio
{
    uint8_t buff[AUDIO_BUFFER_SIZE];
    BUFFER_StateTypeDef state;
    uint32_t fptr;
};

int main(void)
{   
#if defined(BSP_USING_SDCARD_MOUNT)
    if(dfs_mount("sd0", "/mnt/sd", "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", "/mnt/sd"); 
        return RT_ERROR; 
    }
    
    extern int chdir(const char *path); 
    chdir("/mnt/sd"); 
#endif
    
    rt_kprintf("[STM32] Welcome to use STM32F469-Discovery.\n"); 
    rt_kprintf("[STM32] You can find the laster from <\033[31mhttps://github.com/liu2guang/rtthread_stm32f469\033[0m>.\n"); 
    
    {
        int fd = -1; 
        
        if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, 70, I2S_AUDIOFREQ_22K) != 0)
        {
            rt_kprintf("BSP_AUDIO_OUT_Init failed.\n"); 
        }
        
        fd = open(WAV_FILE_NAME, O_RDONLY); 
        if(fd < 0)
        {
            rt_kprintf("1.wav open failed fd = %d.\n", fd); 
            return -RT_ERROR; 
        }
        
        // get file size
        struct stat buf; 
        stat(WAV_FILE_NAME, &buf);
        rt_kprintf("%s file size = %d bytes\n", WAV_FILE_NAME, buf.st_size); 
        
        //haudio = rt_malloc(sizeof(struct rt_audio)); 
        haudio = rt_malloc_align(sizeof(struct rt_audio), 4); 
        if(haudio == RT_NULL)
        {
            rt_kprintf("haudio malloc failed.\n"); 
            return -RT_ERROR; 
        }
        
        //BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
        int size = read(fd, (void *)&(haudio->buff[0]), AUDIO_BUFFER_SIZE);
        if(size != 0)
        {
            BSP_AUDIO_OUT_Play((uint16_t*)&(haudio->buff[0]), AUDIO_BUFFER_SIZE);
        }
        
        memset(haudio, 0x00, sizeof(struct rt_audio)); 
        
        rt_kprintf("Start Play %s.\n", WAV_FILE_NAME); 
        
        while(1)
        {
            if(haudio->fptr >= buf.st_size)
            {
                BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
                rt_kprintf("Play failed.\ns"); 
                return 0; 
            }
            
            if(haudio->state == BUFFER_OFFSET_HALF)
            {
                size = read(fd, (void *)(haudio->buff), AUDIO_BUFFER_SIZE/2);
                
                if(size != 0)
                {
                    haudio->state = BUFFER_OFFSET_NONE;
                    haudio->fptr += size; 
                }
            }
            
            if(haudio->state == BUFFER_OFFSET_FULL)
            {
                size = read(fd, (void *)&(haudio->buff[AUDIO_BUFFER_SIZE/2]), AUDIO_BUFFER_SIZE/2);
                
                if(size != 0)
                {
                    haudio->state = BUFFER_OFFSET_NONE;
                    haudio->fptr += size; 
                }
            }
            
            rt_thread_delay(10); 
        }
    }
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    haudio->state = BUFFER_OFFSET_FULL;
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{ 
    haudio->state = BUFFER_OFFSET_HALF;
}
