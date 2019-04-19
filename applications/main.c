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
#include <stdio.h>
#include <rtthread.h>
#include <drv_audio_in.h>
#include <string.h>
#include <reent.h>
#include <newlib.h>
#include <board.h>
#define window  (1024 / 16)
extern int reco_main(void);

extern int32_t fft_1024(float * in_buffer, float * out_buffer, float * max_value);
int main(void)
{   
    reco_main();
    // int16_t * ppcm_data;
    // int16_t time = 0;
    // FILE * fp;
    // char str[30];
    // int16_t * ppcm_buffer = rt_malloc(window * 64);
    // int16_t * mono_pcm_buffer = rt_malloc(window * 32);
    // static float feq_data[window * 32];
    // static float fft_result[window * 16];
    // float max_value;
    // int index;
    // rt_mailbox_t mic_mb = mic_start_record(4);
    // while(1)
    // {
    //     if(rt_mb_recv(mic_mb, (rt_ubase_t *)&ppcm_data, RT_WAITING_FOREVER) != RT_EOK)
    //     {
    //         continue;
    //     }
    //     if(ppcm_data != RT_NULL)
    //     {
    //         memcpy(&ppcm_buffer[time * 32], ppcm_data, 64);
    //         time++;
    //         rt_mp_free(ppcm_data);
    //         ppcm_data = RT_NULL;
    //         if(time == window)
    //         {
    //             pcm_stereo_to_mono(ppcm_buffer, 32 * window, mono_pcm_buffer);
    //             for(time = 0; time < 16 * window; time++)
    //             {
    //                 feq_data[2 * time] = ppcm_buffer[2 * time] / 10000.0;
    //                 feq_data[2 * time + 1] = 0;
    //             }
    //             time = 0;
    //             index = fft_1024(feq_data, fft_result, &max_value);
    //             printf("[%d] %f\n", index, max_value);
    //         }
    //     }
    // }
    // // rt_kprintf("stop record %d\n", rt_tick_get());
    // // pcm_stereo_to_mono(ppcm_buffer, 32 * window, mono_pcm_buffer);
    // // rt_kprintf("convert stereo to mono %d\n", rt_tick_get());
    // // fp = fopen("/mnt/sd/1.pcm", "wb");
    // // fwrite(mono_pcm_buffer, 32, window, fp);
    // // fclose(fp);
    // // for(time = 0; time < 16 * window; time++)
    // // {
    // //     feq_data[2 * time] = ppcm_buffer[2 * time] / 10000.0;
    // //     feq_data[2 * time + 1] = 0;
    // // }
    // // printf("123456 %d\n", 20);
    // // index = fft_1024(feq_data, fft_result, &max_value);
    // // rt_kprintf("index %d\n", index);
    // // printf("[%d] %f\n", index, max_value);
    // // fp = fopen("/mnt/sd/2.fft", "wb");
    // // fwrite(fft_result, 16, window, fp);
    // // fclose(fp);

    // // rt_kprintf("write to sdcard done %d\n", rt_tick_get());
    // rt_free(ppcm_buffer);
    // rt_free(mono_pcm_buffer);
    return 0;
}
