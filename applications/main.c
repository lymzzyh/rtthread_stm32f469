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



#if defined(RT_USING_DFS)
#include <dfs_file.h>
#include <dfs_posix.h>
#endif
#ifdef BSP_USING_AUDIO
#include "drv_codec.h"
#include "drv_cs43l22.h" 
#endif
int main(void)
{   
#if defined(RT_USING_DFS) && defined(RT_USING_SDIO)
    int result = mmcsd_wait_cd_changed(RT_TICK_PER_SECOND);
    if (result == MMCSD_HOST_PLUGED)
    {
        /* mount sd card fat partition 1 as root directory */
        if (dfs_mount("sd0", "/mnt/sd", "elm", 0, 0) == 0)
        {
            rt_kprintf("sdcard mount '%s' failed.\n", "/mnt/sd"); 
            return RT_ERROR; 
        }
        extern int chdir(const char *path); 
        chdir("/mnt/sd");
    }
    else
    {
        rt_kprintf("sdcard not inserted!\n");
    }
#elif defined(BSP_USING_SDCARD_MOUNT)
    if(dfs_mount("sd0", "/mnt/sd", "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", "/mnt/sd"); 
        return RT_ERROR; 
    }
    
    extern int chdir(const char *path); 
    chdir("/mnt/sd"); 
#endif
#ifdef BSP_USING_AUDIO
    cs43l22_init("i2c2", 1, 0x94>>1, cs43l22_output_headphone, 100); 
#endif
}
