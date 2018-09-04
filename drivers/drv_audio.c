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
#include "board.h" 

#define CS43L22_I2C_ADDR               (0x94 >> 1)
#define CS43L22_RESET_INDEX            (1)
#define CS43L22_VOLUME_CONVERT(volume) (((volume) > 100)? 100:((rt_uint8_t)(((volume) * 255) / 100)))

struct rt_i2c_bus_device *i2c_bus = RT_NULL; 
SAI_HandleTypeDef hsai;

static rt_err_t cs43l22_reg_read(rt_uint8_t reg, rt_uint8_t *dat) 
{
    RT_ASSERT(dat != RT_NULL); 
    
    if (rt_i2c_master_send(i2c_bus, CS43L22_I2C_ADDR, 0, &reg, 1) != 1)
    {
        return -RT_ERROR;
    }
    
    if (rt_i2c_master_recv(i2c_bus, CS43L22_I2C_ADDR, 0, dat, 1) != 1)
    {
        return -RT_ERROR;
    }
    
    return RT_EOK; 
}

static rt_err_t cs43l22_reg_write(rt_uint8_t reg, rt_uint8_t dat)
{
    rt_uint8_t buf[2] = {reg, dat}; 
    
    if (rt_i2c_master_send(i2c_bus, CS43L22_I2C_ADDR, 0, buf, 2) != 2)
    {
        RT_ASSERT(RT_FALSE); 
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

static rt_err_t cs43l22_set_volume(rt_uint8_t volume)
{
    uint8_t convertedvol = CS43L22_VOLUME_CONVERT(volume);

    if(volume > 0xE6)
    {
        cs43l22_reg_write(CS43L22_REG_MASTER_A_VOL, convertedvol - 0xE7);
        cs43l22_reg_write(CS43L22_REG_MASTER_B_VOL, convertedvol - 0xE7);
    }
    else
    {
        cs43l22_reg_write(CS43L22_REG_MASTER_A_VOL, convertedvol + 0x19);
        cs43l22_reg_write(CS43L22_REG_MASTER_B_VOL, convertedvol + 0x19);
    }
    
    return RT_EOK; 
}

static void cs43l22_reset(void)
{
    rt_pin_write(CS43L22_RESET_INDEX, PIN_LOW); 
    rt_thread_mdelay(20); 
    rt_pin_write(CS43L22_RESET_INDEX, PIN_HIGH); 
}

rt_err_t sai1_init(rt_uint32_t freq)
{
    GPIO_InitTypeDef gpio;
    static DMA_HandleTypeDef hdma;
    
    /* SAI1 GPIO CLK INIT */ 
    __HAL_RCC_GPIOE_CLK_ENABLE(); 
    __HAL_RCC_GPIOG_CLK_ENABLE(); 
    
    /* MCK PIN INIT */ 
    gpio.Pin       = GPIO_PIN_7;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_HIGH;
    gpio.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(GPIOG, &gpio);
    
    /* FS, SCK, MCK and SD PINS INIT */ 
    gpio.Pin       = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_HIGH;
    gpio.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(GPIOE, &gpio); 
    
    /* DMA INIT */ 
    __HAL_RCC_DMA2_CLK_ENABLE(); 
    
    hdma.Instance                 = DMA2_Stream3;
    hdma.Init.Channel             = DMA_CHANNEL_0;
    hdma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma.Init.MemInc              = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma.Init.Mode                = DMA_CIRCULAR;
    hdma.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    hdma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    __HAL_LINKDMA(&hsai, hdmatx, hdma); 

    HAL_DMA_DeInit(&hdma);
    // HAL_DMA_Init(&hdma);
    if (HAL_DMA_Init(&hdma) != HAL_OK)
    {
        rt_kprintf("HAL_DMA_Init failed.\n"); 
        return RT_ERROR; 
    }
    
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn); 
    
    /* SAI INIT */ 
    __HAL_RCC_SAI1_CLK_ENABLE(); 
    __HAL_SAI_DISABLE(&hsai);
    
    /* Configure SAI_Block_A LSBFirst: 禁用, DataSize: 16 */ 
    hsai.Instance            = SAI1_Block_A;
    hsai.Init.AudioFrequency = freq;
    hsai.Init.ClockSource    = SAI_CLKSOURCE_PLLI2S;
    hsai.Init.AudioMode      = SAI_MODEMASTER_TX;
    hsai.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;
    hsai.Init.Protocol       = SAI_FREE_PROTOCOL;
    hsai.Init.DataSize       = SAI_DATASIZE_16;
    hsai.Init.FirstBit       = SAI_FIRSTBIT_MSB;
    hsai.Init.ClockStrobing  = SAI_CLOCKSTROBING_FALLINGEDGE;
    hsai.Init.Synchro        = SAI_ASYNCHRONOUS;
    hsai.Init.OutputDrive    = SAI_OUTPUTDRIVE_ENABLE;
    hsai.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_1QF;
    
    /* Configure SAI_Block_A Frame: */ 
    // Frame Length: 64
    // Frame active Length: 32
    // FS Definition: Start frame + Channel Side identification
    // FS Polarity: FS active Low
    // FS Offset: FS asserted one bit before the first bit of slot 0
    hsai.FrameInit.FrameLength       = 64;
    hsai.FrameInit.ActiveFrameLength = 32;
    hsai.FrameInit.FSDefinition      = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai.FrameInit.FSPolarity        = SAI_FS_ACTIVE_LOW;
    hsai.FrameInit.FSOffset          = SAI_FS_BEFOREFIRSTBIT; 
    
    /* Configure SAI Block_A Slot: */ 
    // Slot First Bit Offset: 0
    // Slot Size  : 16
    // Slot Number: 4
    // Slot Active: All slot actives
    
    hsai.SlotInit.FirstBitOffset = 0;
    hsai.SlotInit.SlotSize       = SAI_SLOTSIZE_DATASIZE;
    hsai.SlotInit.SlotNumber     = 4;
    hsai.SlotInit.SlotActive     = SAI_SLOTACTIVE_0;
    
    HAL_SAI_DeInit(&hsai); 
    if (HAL_SAI_Init(&hsai) != HAL_OK)
    {
        rt_kprintf("HAL_SAI_Init failed.\n"); 
        return RT_ERROR; 
    }
    
    __HAL_SAI_ENABLE(&hsai);
    
    return RT_EOK; 
}

static void dump(void); 
static rt_err_t cs43l22_init(const char *i2c_bus_name, rt_uint32_t freq, rt_uint8_t volume) 
{
    RT_ASSERT(i2c_bus_name != RT_NULL); 
    
    i2c_bus = rt_i2c_bus_device_find(i2c_bus_name); 
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("can't find %s device\n", i2c_bus_name); 
        return RT_ERROR; 
    }
    
    rt_device_open(&(i2c_bus->parent), RT_DEVICE_OFLAG_RDWR); 
    
    // 1. 初始化RESET
    // 2. 初始化时钟树分配
    rt_pin_mode(CS43L22_RESET_INDEX, PIN_MODE_OUTPUT); 
    
    /* 复位芯片 */ 
    cs43l22_reset(); 
    
    /* 初始化 */ 
    cs43l22_reg_write(CS43L22_REG_POWER_CTL1, 0x01);
    cs43l22_reg_write(CS43L22_REG_POWER_CTL2, 0xAF);
    cs43l22_reg_write(CS43L22_REG_CLOCKING_CTL, 0x81);
    cs43l22_reg_write(CS43L22_REG_INTERFACE_CTL1, 0x04);
    
    cs43l22_set_volume(volume); 
    
    /* 编解码器的额外配置, 这些配置是为了减少解码器关机的需要的时间. 如果这些配置被
       移除了后, 会在关闭编码器和关闭I2S外围的MCLK时钟之间添加一个很长的延迟, 如果没有插入
       延迟, 编码器将无法正常关机, 关机后会产生高噪声. */
    cs43l22_reg_write(CS43L22_REG_ANALOG_ZC_SR_SETT, 0x00); // 禁用analog soft ramp
    cs43l22_reg_write(CS43L22_REG_MISC_CTL, 0x04);          // 禁用digital soft ramp
    cs43l22_reg_write(CS43L22_REG_LIMIT_CTL1, 0x00);        // 禁用limiter attack leve
    cs43l22_reg_write(CS43L22_REG_TONE_CTL, 0x0F);          // 降低音量和高音的等级
    cs43l22_reg_write(CS43L22_REG_PCMA_VOL, 0x0A);          // 调整PCM音量级别
    cs43l22_reg_write(CS43L22_REG_PCMB_VOL, 0x0A);
    
    // 初始化SAI
    sai1_init(freq); 

    dump(); 
    cs43l22_read_id(); 
    cs43l22_read_chip_revision(); 
    
    return RT_EOK; 
}

/* audio 初始化 */
int rt_hw_audio_init(void)
{
    cs43l22_init("i2c2", 16*1000, 60); 
    
    return RT_EOK; 
}
INIT_DEVICE_EXPORT(rt_hw_audio_init); 









































#include "finsh.h"
static void dump(void)
{
    rt_uint8_t ind = 0, reg = 0x5A; 
    
    rt_kprintf("Audio Dump Info:\n"); 
    rt_kprintf("================================================================\n");
    
    for(ind = CS43L22_REG_ID; ind <= CS43L22_REG_CHARGE_PUMP_FREQ; ind++)
    {
        reg = 0x5A; 
        if(cs43l22_reg_read(ind, &reg) == RT_EOK) {rt_kprintf("reg(0x%.2X = 0x%.2X) ", ind, reg);}  
        if(ind % 4 == 0) {rt_kprintf("\n");}
    }
    
    rt_kprintf("\n"); 
}
MSH_CMD_EXPORT(dump, dump); 
