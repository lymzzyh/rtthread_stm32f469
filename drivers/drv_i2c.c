/*
 * File      : drv_i2c.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-06-05     tanek        first implementation.
 * 2018-03-08     ZYH          Porting for stm32f4xx
 */
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_i2c.h"
#include <board.h>

#define I2C_BUS_NAME  "i2c2"

#if 0
#define I2C_SCL_PIN                 GPIO_PIN_6
#define I2C_SCL_PORT                GPIOC
#define I2C_SCL_PORT_CLK_ENABLE     __HAL_RCC_GPIOC_CLK_ENABLE
#define I2C_SDA_PIN                 GPIO_PIN_7
#define I2C_SDA_PORT                GPIOC
#define I2C_SDA_PORT_CLK_ENABLE     __HAL_RCC_GPIOC_CLK_ENABLE
#else
#define I2C_SCL_PIN                 GPIO_PIN_4
#define I2C_SCL_PORT                GPIOH
#define I2C_SCL_PORT_CLK_ENABLE     __HAL_RCC_GPIOH_CLK_ENABLE
#define I2C_SDA_PIN                 GPIO_PIN_5
#define I2C_SDA_PORT                GPIOH
#define I2C_SDA_PORT_CLK_ENABLE     __HAL_RCC_GPIOH_CLK_ENABLE
#endif

static struct rt_i2c_bus_device i2c_bus;

static void drv_i2c_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    I2C_SCL_PORT_CLK_ENABLE();
    I2C_SDA_PORT_CLK_ENABLE();
    
    GPIO_Initure.Pin = I2C_SCL_PIN;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_Initure);
    
    GPIO_Initure.Pin = I2C_SDA_PIN;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_Initure);
    
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
}

static void drv_set_sda(void *data, rt_int32_t state)
{
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, (GPIO_PinState)state);
}

static void drv_set_scl(void *data, rt_int32_t state)
{
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, (GPIO_PinState)state);
}

static rt_int32_t drv_get_sda(void *data)
{
    return (rt_int32_t)HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN);
}

static rt_int32_t drv_get_scl(void *data)
{
    return (rt_int32_t)HAL_GPIO_ReadPin(I2C_SCL_PORT, I2C_SCL_PIN);
}

static void drv_udelay(rt_uint32_t us)
{
    rt_int32_t i;
    for (; us > 0; us--)
    {
        i = 30;
        while (i > 0)
        {
            i--;
        }
    }
}

static const struct rt_i2c_bit_ops drv_bit_ops =
{
    RT_NULL,
    drv_set_sda,
    drv_set_scl,
    drv_get_sda,
    drv_get_scl,
    drv_udelay,
    1,
    50
};

int rt_hw_i2c_init(void)
{
    i2c_bus.priv = (void *)&drv_bit_ops;

    drv_i2c_gpio_init();

    rt_i2c_bit_add_bus(&i2c_bus, "i2c2");

    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_i2c_init);
