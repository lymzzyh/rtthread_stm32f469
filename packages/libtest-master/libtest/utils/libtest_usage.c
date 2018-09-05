/*
 * @File:   libtest_usage.c
 * @Author: liu2guang
 * @Date:   2018-08-21 20:57:10
 *
 * @LICENSE: MIT
 * https://github.com/liu2guang/optparse/blob/master/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-21     liu2guang    update RT-Thread testcase toolkis.
 */

#include "libtest_usage.h" 
#include "libtest_def.h" 
#include "optparse.h"
#include "string.h"

static rt_uint8_t  major = 0; 
static rt_uint8_t  minor = 0;
static rt_bool_t   usage_view = RT_TRUE; 
// static rt_uint32_t usage_load = 0;

static void libtest_usage_view_run(rt_bool_t enable)
{
    if(usage_view != enable)
    {
        LOG_I("usage view plugin %s.", (enable == RT_TRUE) ? "run" : "close"); 
    }
    usage_view = enable;
}

static void libtest_usage_view_entry(void *p)
{
    rt_uint8_t index = 0; 
    
    while(1)
    {
#if defined(LIBTEST_USING_USAGE_VIEW_TAIL)
        if(usage_view == RT_FALSE)
        {
            rt_kprintf("\33[s");            // 保存位置
            for(index = 0; index < 10; index++)
            {
                rt_kprintf("\33[1A");       // 上移动一行
                rt_kprintf("\33[10000C");   // 移动到末尾
                rt_kprintf("\33[11D\33[K"); // 回退光标到插件显示处, 并删除后面几行
            }
            rt_kprintf("\33[u");            // 恢复位置
        }

        while(usage_view == RT_FALSE)
        {
            rt_thread_mdelay(500); 
        }

        rt_kprintf("\33[?25l");         // 隐藏光标

        rt_kprintf("\33[s");            // 保存位置
        for(index = 0; index < 15; index++)
        {
            rt_kprintf("\33[1A");       // 上移动一行
            rt_kprintf("\33[10000C");   // 移动到末尾
            rt_kprintf("\33[11D\33[K"); // 回退光标到插件显示处, 并删除后面几行
        }
        rt_kprintf("\33[u");            // 恢复位置
         
        rt_kprintf("\33[s");            // 保持位置
        rt_kprintf("\33[10000C");
        rt_kprintf("\33[11D");
        rt_kprintf("\33[37;41m"); 
        rt_kprintf("[cpu %.2d.%.2d%%]", major, minor); 
        rt_kprintf("\33[0m");
        rt_kprintf("\33[u");            // 恢复位置

        rt_kprintf("\33[?25h");         // 显示光标
#endif 

#if defined(LIBTEST_USING_USAGE_VIEW_HEAD)
        while(usage_view == RT_FALSE)
        {
            rt_thread_mdelay(500); 
        }

        rt_kprintf("\33[?25l");         // 隐藏光标
        rt_kprintf("\33[s");            // 保存位置
        rt_kprintf("\33[500A");         // 上移动1000行
        rt_kprintf("\33[500C");         // 右移动1000行
        rt_kprintf("\33[11D");
        rt_kprintf("\33[37;41m");
        rt_kprintf("[cpu %.2d.%.2d%%]", major, minor);
        rt_kprintf("\33[0m");
        rt_kprintf("\33[u");            // 恢复位置
        rt_kprintf("\33[?25h");         // 显示光标
#endif 

        rt_thread_mdelay(200);       
    }
}

// static void libtest_usage_load_entry(void *p)
// {

// }

static void libtest_usage_info_outpu(void)
{
    LOG_I("cpu usage %.2d.%.2d%%, tick %d.", major, minor, rt_tick_get()); 
}

static struct optparse_long long_opts[] = 
{
    {"help" , 'h', OPTPARSE_NONE    }, 
    {"view" , 'v', OPTPARSE_REQUIRED}, 
    {"info" , 'i', OPTPARSE_NONE    },
    {"load" , 'l', OPTPARSE_REQUIRED}, 
    { NULL  ,  0 , OPTPARSE_NONE    }
}; 

static void libtest_usage_usage(void)
{
    rt_kprintf("usage: usage [option] [target] ...\n\n"); 
    rt_kprintf("usage options:\n"); 
    rt_kprintf("  -h, --help                         Print defined help message.\n"); 
    rt_kprintf("  -i, --info                         Output the cpu usage info.\n");
    rt_kprintf("  -l USAGE,  --load=USAGE            Training the cpu load.\n");
    rt_kprintf("  -v SWITCH, --view=SWITCH           On/off the cpu usage view plugin.\n"); 
}

static int libtest_usage_cmd(int argc, char **argv)
{
    int ch; 
    int option_index; 
    struct optparse options;

    rt_bool_t _usage_usage = RT_FALSE; 
    rt_bool_t _usage_view  = usage_view; 
    rt_bool_t _usage_info  = RT_FALSE; 

    if(argc == 1)
    {
        libtest_usage_usage(); 
        return RT_EOK; 
    }
    
    optparse_init(&options, argv); 
    while((ch = optparse_long(&options, long_opts, &option_index)) != -1)
    {
        switch(ch)
        {
        case 'h':
            _usage_usage = RT_TRUE; 
            break; 

        case 'v': 
            if (!strcmp(options.optarg, "on"))
            {
                _usage_view = RT_TRUE;  
            }
            else if(!strcmp(options.optarg, "off"))
            {
                _usage_view = RT_FALSE;  
            }
            break; 

        case 'i':
            _usage_info = RT_TRUE; 
            break; 

        case 'l':
            //usage_load = (options.optarg == RT_NULL) ? atoi(options.optarg) : usage_load; 
            break; 
        }
    }

    libtest_usage_view_run(_usage_view); 

    if(_usage_usage == RT_TRUE)
    {
        libtest_usage_usage(); 
    }

    if(_usage_info == RT_TRUE)
    {
        libtest_usage_info_outpu(); 
    }
    
    return RT_EOK; 
}
MSH_CMD_EXPORT_ALIAS(libtest_usage_cmd, usage, the usage test toolkits.); 

static void libtest_usage_hook(void)
{
    rt_tick_t tick;
    rt_uint32_t count;
    volatile rt_uint32_t loop;
    static rt_uint32_t total = 0; 

    if (total == 0)
    {
        /* get total count */
        rt_enter_critical();
        tick = rt_tick_get();
        while(rt_tick_get() - tick < 10)
        {
            total ++;
            loop = 0;
            while (loop < 100) loop ++;
        }
        rt_exit_critical();
    }

    count = 0;
    tick = rt_tick_get();
    while (rt_tick_get() - tick < 10)
    {
        count ++;
        loop  = 0;
        while (loop < 100) loop ++;
    }

    if (count < total)
    {
        count = total - count;
        major = (count * 100) / total;
        minor = ((count * 100) % total) * 100 / total;
    }
    else
    {
        total = count;
        major = 0;
        minor = 0;
    }
}

rt_err_t libtest_usage_init(void)
{
    rt_thread_t thread = RT_NULL; 

    rt_thread_idle_sethook(libtest_usage_hook);

    /* create usage view plugin */ 
    thread = rt_thread_create("libtest_view", libtest_usage_view_entry, 
        RT_NULL, 512, FINSH_THREAD_PRIORITY-1, 10); 
    if(thread == RT_NULL)
    {
        LOG_E("create libtest usage view plugin failed."); 
        return RT_EOK; 
    }
    rt_thread_startup(thread); 

    /* create usage load plugin */ 
    // thread = rt_thread_create("libtest_load", libtest_usage_load_entry, 
    //     RT_NULL, 512, 1, 10); 
    // if(thread == RT_NULL)
    // {
    //     LOG_E("create libtest usage load plugin failed."); 
    //     return RT_EOK; 
    // }
    // rt_thread_startup(thread); 

    return RT_EOK; 
}
