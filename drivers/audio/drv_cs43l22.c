/*
 * File      : drv_cs43l22.c
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

#include "drv_cs43l22.h" 

#define CS43L22_REG(n) (n) 

// const static unsigned char vol_table[100] =
// {
//     196, 180, 170, 160, 150, 140, 130, 120, 110, 100,
//     98, 96, 94, 92, 90, 89, 87, 86, 84, 83,
//     82, 81, 80, 79, 78, 76, 75, 74, 73, 72,
//     69, 68, 67, 66, 65, 64, 63, 62, 61, 60,
//     59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
//     49, 48, 47, 46, 45, 44, 43, 42, 41, 40,
//     39, 38, 37, 36, 35, 34, 33, 32, 31, 30,
//     29, 28, 27, 26, 25, 24, 23, 22, 21, 20,
//     19, 18, 17, 16, 15, 14, 13, 12, 11, 10,
//     9, 8, 7, 6, 5, 4, 3, 2, 1, 0
// };

struct rt_cs43l22
{
    struct rt_i2c_bus_device *i2c; 
    
    rt_bool_t is_play; 
    rt_bool_t is_init; 

    rt_uint8_t addr; 
    rt_uint8_t output; 
    rt_uint8_t volume; 
    rt_int8_t  pin; 
}; 
static struct rt_cs43l22 cs43l22 = {0}; 

rt_uint8_t reg_read(rt_uint8_t reg)
{
    struct rt_i2c_msg msg[2] = {0};
    uint8_t val = 0xff;

    RT_ASSERT(cs43l22.i2c != RT_NULL);

    msg[0].addr  = cs43l22.addr; 
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &reg;

    msg[1].addr  = cs43l22.addr; 
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = 1;
    msg[1].buf   = &val;

    if (rt_i2c_transfer(cs43l22.i2c, msg, 2) != 2)
    {
        rt_kprintf("I2C write data failed.\n");
        return 0xff;
    }

    return val;
}

static void reg_write(rt_uint8_t reg, rt_uint8_t val)
{
    struct rt_i2c_msg msgs[1] = {0};
    rt_uint8_t buff[2] = {0};

    RT_ASSERT(cs43l22.i2c != RT_NULL);

    buff[0] = reg;
    buff[1] = val;

    msgs[0].addr  = cs43l22.addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buff;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(cs43l22.i2c, msgs, 1) != 1)
    {
        rt_kprintf("I2C write data failed.\n");
        return; 
    }

    // Debug 
    rt_kprintf("REG WRITE(%.2x: %.2x)\n", reg, val); 
}

void reg_dump(void)
{
    rt_uint8_t reg = 0; 
    
    rt_kprintf("### Audio Dump Info: ###\n"); 
    
    for(reg = CS43L22_REG(0x01); reg <= CS43L22_REG(0x34); reg++)
    {
        rt_kprintf("reg(0x%.2X = 0x%.2X) ", reg, reg_read(reg));

        if(reg % 4 == 0) 
        {
            rt_kprintf("\n");
        }
    }
    
    rt_kprintf("\n"); 
}

/**
 * Read the cs43l22 chip id
 * 
 * @return  ID: cs43l22 id is 0x1C
 */
rt_uint8_t cs43l22_chip_id(void)
{
    return reg_read(CS43L22_REG(0x01)) & 0xF8; 
}

/**
 * Read the cs43l22 chip revision
 * 
 * @return  Revision: CS43L22 revision level(0: A0, 1: A1, 2: B0, 3: B1)
 */
rt_uint8_t cs43l22_chip_revision(void)
{
    const char *string[] = {"A0", "A1", "B0", "B1"}; 

    rt_uint8_t revision = reg_read(CS43L22_REG(0x01)) & 0x07; 

    rt_kprintf("revision is %s.\n", string[revision]); 

    return revision; 
}

/**
 * Set the cs43l22 volume
 * 
 * @return  Revision: CS43L22 revision level(0: A0, 1: A1, 2: B0, 3: B1)
 */
rt_err_t cs43l22_set_volume(rt_uint8_t volume)
{
    rt_err_t ret = RT_EOK; 
    rt_uint8_t tmp = 0x00;
    
    tmp = volume > 100 ? 255 : (rt_uint8_t)(volume*255/100); 
    
    if (tmp > 0xE6) 
    {
        reg_write(CS43L22_REG(0x20), tmp - 0xE7);
        reg_write(CS43L22_REG(0x21), tmp - 0xE7); 
    }
    else
    {
        reg_write(CS43L22_REG(0x20), tmp + 0x19);
        reg_write(CS43L22_REG(0x21), tmp + 0x19);
    }

    cs43l22.volume = volume; 
    
    return ret; 
} 

rt_err_t cs43l22_set_mute(rt_bool_t enable)
{
    rt_err_t ret = RT_EOK; 

    if(enable == RT_TRUE)
    {
        reg_write(CS43L22_REG(0x04), 0xFF);
        reg_write(CS43L22_REG(0x22), 0x01);
        reg_write(CS43L22_REG(0x23), 0x01);
    }
    else
    {
        reg_write(CS43L22_REG(0x22), 0x00);
        reg_write(CS43L22_REG(0x23), 0x00);
        reg_write(CS43L22_REG(0x04), cs43l22.output);
    }
    
    return ret; 
}

rt_err_t cs43l22_set_output(rt_uint8_t mode)
{
    rt_err_t ret = RT_EOK; 

    reg_write(CS43L22_REG(0x04), cs43l22.output); 
    
    return ret; 
}

/**
 * Hardware reset for cs43l22
 * 
 * @return  RT_EOK:    Reset successfully
 *         -RT_ENOSYS: Not find cs43l22 reset pin 
 */
rt_err_t cs43l22_reset(void)
{
    rt_err_t ret = RT_EOK; 

    if (cs43l22.pin == (-1))
    {
        rt_kprintf("cs43l22 reset pin unknown.\n"); 
        ret = (-RT_ENOSYS); 
        goto __exit; 
    }

    rt_pin_write(cs43l22.pin, PIN_LOW); 
    rt_thread_mdelay(20); 
    rt_pin_write(cs43l22.pin, PIN_HIGH); 

__exit: 
    return ret; 
}

/**
 * Play the cs43l22 into play states
 * 
 * @return  RT_EOK:    Success Play
 *         -RT_ENOSYS: cs43l22 is uninitialized
 */
rt_err_t cs43l22_play(void)
{
    rt_err_t ret = RT_EOK; 

    if (cs43l22.is_init == RT_FALSE)
    {
        rt_kprintf("cs43l22 is uninitialized.\n"); 
        ret = (-RT_ENOSYS); 
        goto __exit; 
    }

    reg_write(CS43L22_REG(0x0E), 0x06); 
    cs43l22_set_mute(RT_FALSE); 
    reg_write(CS43L22_REG(0x02), 0x9E); 
    
__exit: 
    return ret; 
} 

/**
 * Stop the cs43l22 into stop states
 * 
 * @return  RT_EOK:    Success Stop
 *         -RT_ENOSYS: cs43l22 is uninitialized
 */
rt_err_t cs43l22_stop(void)
{
    rt_err_t ret = RT_EOK; 

    if (cs43l22.is_init == RT_FALSE)
    {
        rt_kprintf("cs43l22 is uninitialized.\n"); 
        ret = (-RT_ENOSYS); 
        goto __exit; 
    }

    cs43l22_set_mute(RT_TRUE); 
    reg_write(CS43L22_REG(0x0E), 0x04); 
    reg_write(CS43L22_REG(0x02), 0x9F); 
    
__exit: 
    return ret; 
} 

/**
 * Pause the cs43l22 into stop states
 * 
 * @return  RT_EOK:    Success pause
 *         -RT_ENOSYS: cs43l22 is uninitialized
 */
rt_err_t cs43l22_pause(void)
{
    rt_err_t ret = RT_EOK; 

    if (cs43l22.is_init == RT_FALSE)
    {
        rt_kprintf("cs43l22 is uninitialized.\n"); 
        ret = (-RT_ENOSYS); 
        goto __exit; 
    }

    cs43l22_set_mute(RT_TRUE); 
    reg_write(CS43L22_REG(0x02), 0x01);
    
__exit: 
    return ret; 
}

/**
 * Resume the cs43l22 into play states
 * 
 * @return  RT_EOK:    Success resume
 *         -RT_ENOSYS: cs43l22 is uninitialized
 */
rt_err_t cs43l22_resume(void)
{
    rt_err_t ret = RT_EOK; 
    volatile rt_uint8_t index = 0x00; 

    if (cs43l22.is_init == RT_FALSE)
    {
        rt_kprintf("cs43l22 is uninitialized.\n"); 
        ret = (-RT_ENOSYS); 
        goto __exit; 
    }
    
    cs43l22_set_mute(RT_FALSE); 
    
    for(index = 0x00; index < 0xFF; index++);
    
    reg_write(CS43L22_REG(0x04), cs43l22.output); 
    reg_write(CS43L22_REG(0x02), 0x9E);
    
__exit: 
    return ret; 
} 

/**
 * Initialize the cs43l22 chip
 * This function cannot be called repeatedly, other apis can only be used after initialization.
 *
 * @param i2c_name i2c bus name
 * @param pin cs43l22 hardware reset pin
 * @param addr i2c device addr for cs43l22
 * @param output default output mode(see "drv_cs43l22.h enum output_mode") 
 * @param volume default volume(0~100)
 * 
 * @return  RT_EOK:    Success initialization
 *         -RT_EINVAL: Volume is too high
 *         -RT_ERROR:  Repeated initialization
 *         -RT_EIO:    I2C bus not find
 */
rt_err_t cs43l22_init(const char *i2c_name, rt_int8_t pin, rt_uint8_t addr, rt_uint8_t output, rt_uint8_t volume)
{
    rt_err_t ret = RT_EOK; 

    if (cs43l22.is_init == RT_TRUE)
    {
        rt_kprintf("cs43l22 is already initialized.\n"); 
        ret = (-RT_ERROR); 
        return ret; 
    }
    
    rt_memset(&cs43l22, 0x00, sizeof(cs43l22)); 

    if(volume > 100)
    {
        rt_kprintf("cs43l22 init volume is too high.\n"); 
        ret = (-RT_EINVAL); 
        return ret; 
    }
    
    cs43l22.addr   = addr; 
    cs43l22.output = output; 
    cs43l22.volume = volume; 
    cs43l22.pin    = pin; 

    /* Init hardware reset pin and reset */ 
    rt_pin_mode(pin, PIN_MODE_OUTPUT); 
    cs43l22_reset(); 

    /* Init cs43l22 i2c bus */ 
    cs43l22.i2c = rt_i2c_bus_device_find(i2c_name); 
    if (cs43l22.i2c == RT_NULL)
    {
        rt_kprintf("I2c bus not find.\n"); 
        ret = (-RT_EIO); 
        goto __fail; 
    }

    rt_device_open(&(cs43l22.i2c->parent), RT_DEVICE_OFLAG_RDWR); 
    
    reg_write(CS43L22_REG(0x02), 0x01);         /* Keep cs43l22 powered OFF */ 
    cs43l22_set_output(output);                 /* Set cs43l22 output mode */ 
    reg_write(CS43L22_REG(0x05), 0x81);         /* Clock configuration: Auto detection */ 
    reg_write(CS43L22_REG(0x06), 0x04);         /* Set the Slave Mode and the audio Standard  */ 
    cs43l22_set_volume(volume);                 /* Set the Master volume */
    
    /* If the Speaker is enabled, set the Mono mode and volume attenuation level */
    if(output != cs43l22_output_headphone) 
    {
        reg_write(CS43L22_REG(0x0F), 0x06);     /* Set the Speaker Mono mode */
        reg_write(CS43L22_REG(0x24), 0x00);     /* Set the Speaker attenuation level */
        reg_write(CS43L22_REG(0x25), 0x00);
    }
    
    /* Additional configuration for the CODEC. These configurations are done to reduce
       the time needed for the Codec to power off. If these configurations are removed, 
       then a long delay should be added between powering off the Codec and switching 
       off the I2S peripheral MCLK clock (which is the operating clock for Codec).
       If this delay is not inserted, then the codec will not shut down properly and
       it results in high noise after shut down. */

    reg_write(CS43L22_REG(0x0A), 0x00);         /* Disable the analog soft ramp */
    reg_write(CS43L22_REG(0x0E), 0x04);         /* Disable the digital soft ramp */
    reg_write(CS43L22_REG(0x27), 0x00);         /* Disable the limiter attack level */
    reg_write(CS43L22_REG(0x1F), 0x0F);         /* Adjust Bass and Treble levels */
    reg_write(CS43L22_REG(0x1A), 0x0A);         /* Adjust PCM volume level */
    reg_write(CS43L22_REG(0x1B), 0x0A);         /* Return communication control value */

    cs43l22.is_init = RT_TRUE; 
    
    reg_dump(); 
    
    return ret; 

__fail: 
    cs43l22.is_init = RT_FALSE; 
    cs43l22.is_play = RT_FALSE; 
    cs43l22.addr    = 0x00; 
    cs43l22.output  = cs43l22_output_both; 
    cs43l22.volume  = 0x00; 
    cs43l22.pin     = (-1); 
    cs43l22.i2c     = RT_NULL; 

    return ret; 
}
