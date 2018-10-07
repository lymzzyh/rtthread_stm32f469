/*
 * @File:   libtest.c
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

#include "libtest.h" 
#include "libtest_def.h" 

// void libtest_init_entry(void *p)
// {
//     libtest_usage_init(); 
//     LOG_I("libtest init successful."); 
// }

int libtest_init(void)
{
    // rt_thread_t thread = RT_NULL; 

    // thread = rt_thread_create("libtest_init", libtest_init_entry, 
    //     RT_NULL, 512, 30, 10); 
    // if(thread == RT_NULL)
    // {
    //     LOG_E("libtest init failed."); 
    //     return RT_EOK; 
    // }
    // rt_thread_startup(thread); 

    libtest_usage_init(); 
    LOG_I("libtest init successful."); 

    return RT_EOK; 
}
INIT_APP_EXPORT(libtest_init); 

// 开机自定义测试指令. 
