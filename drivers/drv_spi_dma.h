/*
 * File      : drv_spi_dma.h 
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author         Notes
 * 2018-10-15     liu2guang      the first version for STM32F469NI-Discovery. 
 */

#ifndef __DRV_SPI_DMA_H_ 
#define __DRV_SPI_DMA_H_ 

#include <rtthread.h> 
#include <rtdevice.h>
#include <board.h> 

int rt_hw_spi_init(void);
rt_err_t stm32_spi_bus_attach_device(const char *bus_name, const char *device_name, rt_uint32_t pin);

#endif
