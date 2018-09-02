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

#if defined(BSP_USING_SDCARD_MOUNT)
#include <dfs_file.h>
#endif

int main(void)
{   
#if defined(BSP_USING_SDCARD_MOUNT)
    if(dfs_mount("sd0", "/mnt/sd", "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", "/mnt/sd"); 
        return RT_ERROR; 
    }
#endif
    
    rt_kprintf("[STM32] Welcome to use STM32F469-Discovery.\n"); 
    rt_kprintf("[STM32] You can find the laster from <\033[31mhttps://github.com/liu2guang/rtthread_stm32f469\033[0m>.\n"); 
    
    return 0;
}
