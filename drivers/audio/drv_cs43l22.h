/*
 * File      : drv_cs43l22.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author         Notes
 * 2018-09-10     liu2guang      the first version for STM32F469NI-Discovery.
 */

 
#ifndef __DRV_CS43L22_H_ 
#define __DRV_CS43L22_H_ 

#include <rtthread.h> 
#include <rtdevice.h>
#include <board.h> 

/* cs43l22 output mode */ 
enum output_mode
{
    cs43l22_output_auto      = 0xAA, 
    cs43l22_output_speaker   = 0xFA, 
    cs43l22_output_headphone = 0xAF, 
    cs43l22_output_both      = 0x05, 
}; 

/* API */ 
rt_err_t   cs43l22_init(const char *i2c_name, rt_int8_t pin_reset, rt_uint8_t addr, 
    rt_uint8_t output, rt_uint8_t volume); 
rt_uint8_t cs43l22_chip_id(void); 
rt_uint8_t cs43l22_chip_revision(void); 
rt_err_t   cs43l22_set_volume(rt_uint8_t volume); 
rt_uint8_t cs43l22_get_volume(void); 
rt_err_t   cs43l22_set_mute(rt_bool_t enable); 
rt_err_t   cs43l22_set_output(rt_uint8_t mode); 
rt_err_t   cs43l22_reset(void); 
rt_err_t   cs43l22_play(void); 
rt_err_t   cs43l22_stop(void); 
rt_err_t   cs43l22_pause(void); 
rt_err_t   cs43l22_resume(void); 

void reg_dump(void); 

#endif
