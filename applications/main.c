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

int main(void)
{   
#if defined(BSP_USING_RAMDISK) && defined(BSP_USING_RAMDISK_MOUNT) 
    dfs_mkfs("elm", "ram0"); 
    
    if(dfs_mount("ram0", BSP_USING_RAMDISK_PATH_MOUNT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_RAMDISK_PATH_MOUNT); 
        return RT_ERROR; 
    }
#endif
    
#if defined(BSP_USING_SDCARD_SDIO_BUS) && defined(BSP_USING_SDCARD_MOUNT)
    int result = mmcsd_wait_cd_changed(RT_TICK_PER_SECOND);
    if (result == MMCSD_HOST_PLUGED)
    {
        /* mount sd card fat partition 1 as root directory */
        if (dfs_mount("sd0", BSP_USING_SDCARD_PATH_MOUNT, "elm", 0, 0) == 0)
        {
            rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_SDCARD_PATH_MOUNT); 
            return RT_ERROR; 
        }
        extern int chdir(const char *path); 
        chdir(BSP_USING_SDCARD_PATH_MOUNT);
    }
    else
    {
        rt_kprintf("sdcard not inserted!\n");
    }
#elif defined(BSP_USING_SDCARD_BLOCK) && defined(BSP_USING_SDCARD_MOUNT)
    if(dfs_mount("sd0", BSP_USING_SDCARD_PATH_MOUNT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_SDCARD_PATH_MOUNT); 
        return RT_ERROR; 
    }
    
    extern int chdir(const char *path); 
    chdir(BSP_USING_SDCARD_PATH_MOUNT); 
#endif
}
