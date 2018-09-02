/*
 * @File:   drv_audio.c 
 * @Author: liuguang 
 * @Date:   2018-08-27 23:48:19 
 * 
 * @LICENSE: 
 * The code owner is liuguang. Other personnel may 
 * not be used or modified.
 * 
 * Change Logs: 
 * Date           Author       Notes 
 * 2018-08-27     liuguang     The first version. 
 */ 
 
#include "drv_audio.h" 

#define CS43L22_ADDR        (0x94 >> 1)
#define CS43L22_RESET_INDEX (1)

static void cs43l22_dump(void); 
static rt_err_t cs43l22_set_mute(rt_bool_t enable); 

enum AUDIO_OUTPUT_MODE
{
    OUTPUT_NONE = 0, 
    OUTPUT_AUTO, 
    OUTPUT_SPEAKER,
    OUTPUT_HEADPHONE,
    OUTPUT_BOTH,
}; 

struct rt_i2c_bus_device *i2c_bus   = RT_NULL; 
volatile uint8_t output_mode = OUTPUT_AUTO;
static rt_bool_t cs43l22_is_play = RT_FALSE;

static rt_err_t cs43l22_reg_read(rt_uint8_t reg, rt_uint8_t *dat) 
{
    RT_ASSERT(dat != RT_NULL); 
    
    if (rt_i2c_master_send(i2c_bus, CS43L22_ADDR, 0, &reg, 1) != 1)
    {
        return -RT_ERROR;
    }
    
    if (rt_i2c_master_recv(i2c_bus, CS43L22_ADDR, 0, dat, 1) != 1)
    {
        return -RT_ERROR;
    }
    
    return RT_EOK; 
}

static rt_err_t cs43l22_reg_write(rt_uint8_t reg, rt_uint8_t dat)
{
    rt_uint8_t buf[2]; 
    
    buf[0] = reg; 
    buf[1] = dat;
    
    if (rt_i2c_master_send(i2c_bus, CS43L22_ADDR, 0, buf, 2) != 2)
    {
        return -RT_ERROR;
    }
    
    return RT_EOK; 
}

static rt_uint8_t cs43l22_read_id(void)
{
    rt_uint8_t chip_id = 0; 
    
    if(cs43l22_reg_read(CS43L22_REG_ID, &chip_id) != RT_EOK) 
    {
        rt_kprintf("CS43l22 id read failed.\n"); 
        return 0; 
    }        
    
    chip_id = chip_id & 0xF8; 
    rt_kprintf("CS43l22 id is 0x%.2X.\n", chip_id); 
    
    return chip_id; 
}

static rt_uint8_t cs43l22_read_chip_revision(void)
{
    rt_uint8_t revision = 0; 
    
    if(cs43l22_reg_read(CS43L22_REG_ID, &revision) != RT_EOK) 
    {
        rt_kprintf("CS43l22 chip revision read failed.\n"); 
        return 0; 
    }        
    
    revision = revision & 0x07; 
    rt_kprintf("CS43l22 chip revision is %d.\n", revision); 
    
    return revision; 
}

RT_USED static rt_err_t cs43l22_play(rt_uint16_t* buffer, rt_uint16_t size)
{
    if(cs43l22_is_play == RT_TRUE)
    {
        rt_kprintf("cs43l22 is already play.\n"); 
        return -RT_EIO; 
    }
    
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_MISC_CTL, 0x06) == RT_EOK);
    RT_ASSERT(cs43l22_set_mute(RT_FALSE) == RT_EOK); 
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL1, 0x9E) == RT_EOK);
    
    cs43l22_is_play = RT_TRUE; 
    
    return RT_EOK; 
}

RT_USED static rt_err_t cs43l22_stop(void)
{
    if(cs43l22_is_play == RT_FALSE)
    {
        rt_kprintf("cs43l22 is already stop.\n"); 
        return -RT_EIO; 
    }
    
    RT_ASSERT(cs43l22_set_mute(RT_TRUE) == RT_EOK); 
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_MISC_CTL, 0x04) == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL1, 0x9F) == RT_EOK);
    
    cs43l22_is_play = RT_FALSE; 
    
    return RT_EOK; 
}

RT_USED static rt_err_t cs43l22_pause(void)
{
    RT_ASSERT(cs43l22_set_mute(RT_TRUE) == RT_EOK); 
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL1, 0x01) == RT_EOK);
    
    return RT_EOK; 
}

RT_USED static rt_err_t cs43l22_resume(void)
{
    rt_uint8_t index = 0; 
    
    RT_ASSERT(cs43l22_set_mute(RT_FALSE) == RT_EOK); 
    
    for(index = 0x00; index < 0xFF; index++);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2, output_mode) == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL1, 0x9E) == RT_EOK);
    
    return RT_EOK; 
}

RT_USED static rt_err_t cs43l22_set_freq(rt_uint8_t freq)
{
    return RT_EOK; 
}

static rt_err_t cs43l22_set_mute(rt_bool_t enable)
{
    if(enable == RT_TRUE)
    {
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2     , 0xFF)        == RT_EOK);
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_HEADPHONE_A_VOL, 0x01)        == RT_EOK);
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_HEADPHONE_B_VOL, 0x01)        == RT_EOK);
        
        rt_kprintf("CS43L22 input mute mode.\n"); 
    }
    else
    {
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_HEADPHONE_A_VOL, 0x00)        == RT_EOK);
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_HEADPHONE_B_VOL, 0x00)        == RT_EOK);
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2     , 0x05)        == RT_EOK);
        
        rt_kprintf("CS43L22 exit mute mode.\n"); 
    }
    
    return RT_EOK; 
}

static rt_err_t cs43l22_set_volume(rt_uint8_t volume)
{
    rt_uint8_t conv = 0; 
    
    if(volume >= 100)
    {
        conv = 255; 
    }
    else
    {
        conv = (uint8_t)((volume * 255) / 100); 
    }
    
    if(conv > 0xE6)
    {
        cs43l22_reg_write(CS43L22_REG_MASTER_A_VOL, conv - 0xE7); 
        cs43l22_reg_write(CS43L22_REG_MASTER_B_VOL, conv - 0xE7); 
    }
    else
    {
        cs43l22_reg_write(CS43L22_REG_MASTER_A_VOL, conv + 0x19); 
        cs43l22_reg_write(CS43L22_REG_MASTER_B_VOL, conv + 0x19); 
    }
    
    return RT_EOK; 
}

static rt_err_t cs43l22_output_mode(rt_uint8_t mode)
{
    switch(mode)
    {
    case OUTPUT_AUTO:
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2, 0x05)        == RT_EOK);
        output_mode = OUTPUT_AUTO;
        break; 
    
    case OUTPUT_SPEAKER:
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2, 0xFA)        == RT_EOK);
        output_mode = OUTPUT_SPEAKER;
        break; 
    
    case OUTPUT_HEADPHONE:
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2, 0xAF)        == RT_EOK);
        output_mode = OUTPUT_HEADPHONE;
        break; 
    
    case OUTPUT_BOTH:
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL2, 0xAA)        == RT_EOK);
        output_mode = OUTPUT_BOTH;
        break; 
    
    default:
        rt_kprintf("CS43L22 output mode set failed.\n"); 
        output_mode = OUTPUT_NONE;
        return -RT_ERROR; 
    }
    
    return RT_EOK; 
}

static void cs43l22_reset(void)
{
    rt_pin_write(CS43L22_RESET_INDEX, PIN_LOW); 
    rt_thread_mdelay(20); 
    rt_pin_write(CS43L22_RESET_INDEX, PIN_HIGH); 
}

static rt_err_t cs43l22_init(const char *i2c_bus_name) 
{
    RT_ASSERT(i2c_bus_name != RT_NULL); 
    
    i2c_bus = rt_i2c_bus_device_find(i2c_bus_name); 
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("can't find %s device\n", i2c_bus_name); 
        return RT_ERROR; 
    }
    
    rt_device_open(&(i2c_bus->parent), RT_DEVICE_OFLAG_RDWR); 
    
    /* 没有这个会读取错误 */ 
    rt_pin_mode(CS43L22_RESET_INDEX, PIN_MODE_OUTPUT); 
    
    /* 复位芯片 */ 
    cs43l22_reset(); 
    
    cs43l22_dump(); 
    
    /* 初始化 */ 
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_POWER_CTL1, 0x01)        == RT_EOK);
    cs43l22_output_mode(OUTPUT_AUTO); 
    
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_CLOCKING_CTL, 0x81)        == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_INTERFACE_CTL1, 0x04)      == RT_EOK);
    cs43l22_set_volume(70);
    
    if(output_mode != OUTPUT_HEADPHONE)
    {
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_PLAYBACK_CTL2, 0x06)      == RT_EOK);
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_SPEAKER_A_VOL, 0x00)      == RT_EOK);
        RT_ASSERT(cs43l22_reg_write(CS43L22_REG_SPEAKER_B_VOL, 0x00)      == RT_EOK);
    }
    
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_ANALOG_ZC_SR_SETT, 0x00) == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_MISC_CTL, 0x04)          == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_LIMIT_CTL1, 0x00)        == RT_EOK); 
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_TONE_CTL, 0x0F)          == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_PCMA_VOL, 0x0A)          == RT_EOK);
    RT_ASSERT(cs43l22_reg_write(CS43L22_REG_PCMB_VOL, 0x0A)          == RT_EOK);

    /* 开机自检ID */ 
    while(cs43l22_read_id() != 0xE0) rt_thread_mdelay(100); 
    cs43l22_read_chip_revision(); 
    
    return RT_EOK; 
}

#include "finsh.h"
static void cs43l22_dump(void)
{
    rt_uint8_t index = 0; 
    rt_uint8_t reg   = 0x5A; 
    
    rt_kprintf("cs43l22 dump info:\n"); 
    rt_kprintf("-----------------------------------------------------------\n");
    
    for(index = CS43L22_REG_ID; index <= CS43L22_REG_CHARGE_PUMP_FREQ; index++)
    {
        reg = 0x5A; 
        if(cs43l22_reg_read(index, &reg) == RT_EOK)
        {
            rt_kprintf("reg(0x%.2X = 0x%.2X) ", index, reg); 
        }
        
        if(index % 4 == 0)
        {
            rt_kprintf("\n"); 
        }
    }
    
    rt_kprintf("\n"); 
}
MSH_CMD_EXPORT(cs43l22_dump, cs43l22 dump.); 

/* audio 初始化 */
int rt_hw_audio_init(void)
{
    cs43l22_init("i2c2"); 
    
    return RT_EOK; 
}

INIT_DEVICE_EXPORT(rt_hw_audio_init); 
