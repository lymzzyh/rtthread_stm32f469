/*
 * File      : drv_sdram.c 
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author         Notes
 * 2018-07-28     liu2guang      the first version for STM32F469NI-Discovery
 *                               MICRON MT48LC4M32B2B5-6A. 
 */
 
#include "drv_sdram.h"

static SDRAM_HandleTypeDef SDRAM_Handle; 
static FMC_SDRAM_TimingTypeDef TimStructure;

#define SDRAM_TIME_OUT       ((uint32_t)0XFFFF)
#define SDRAM_REFRESH_COUNT  ((uint32_t)0X0569) 

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0003)

#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)

#define SDRAM_MODEREG_CAS_LATENCY_1              ((uint16_t)0x0010)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)

#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static rt_err_t stm32_sdram_init_seq(rt_uint32_t refresh_count)
{
    HAL_StatusTypeDef ret = HAL_OK; 
    static FMC_SDRAM_CommandTypeDef CommandStructure;
    
    CommandStructure.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE; 
    CommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1; 
    CommandStructure.AutoRefreshNumber      = 1; 
    CommandStructure.ModeRegisterDefinition = 0; 
    ret = HAL_SDRAM_SendCommand(&SDRAM_Handle, &CommandStructure, SDRAM_TIME_OUT); 
    if(ret != HAL_OK)
    {
        rt_kprintf("SDRAM config sdram send clk enable cmd failed.\n"); 
        return RT_ERROR; 
    }
    
    rt_uint32_t count = 100000; 
    while(count--); 
    
    CommandStructure.CommandMode            = FMC_SDRAM_CMD_PALL;
    CommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1; 
    CommandStructure.AutoRefreshNumber      = 1; 
    CommandStructure.ModeRegisterDefinition = 0; 
    ret = HAL_SDRAM_SendCommand(&SDRAM_Handle, &CommandStructure, SDRAM_TIME_OUT); 
    if(ret != HAL_OK)
    {
        rt_kprintf("SDRAM config sdram send pall cmd failed.\n"); 
        return RT_ERROR; 
    }
    
    CommandStructure.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE; 
    CommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1; 
    CommandStructure.AutoRefreshNumber      = 8; 
    CommandStructure.ModeRegisterDefinition = 0; 
    ret = HAL_SDRAM_SendCommand(&SDRAM_Handle, &CommandStructure, SDRAM_TIME_OUT); 
    if(ret != HAL_OK)
    {
        rt_kprintf("SDRAM config sdram send autorefresh mode cmd failed.\n"); 
        return RT_ERROR; 
    }
    
    CommandStructure.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE; 
    CommandStructure.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1; 
    CommandStructure.AutoRefreshNumber      = 1; 
    CommandStructure.ModeRegisterDefinition = SDRAM_MODEREG_BURST_LENGTH_1           |
                                              SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL    |
                                              SDRAM_MODEREG_CAS_LATENCY_3            |
                                              SDRAM_MODEREG_OPERATING_MODE_STANDARD  |
                                              SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED;
    ret = HAL_SDRAM_SendCommand(&SDRAM_Handle, &CommandStructure, SDRAM_TIME_OUT);
    if(ret != HAL_OK)
    {
        rt_kprintf("SDRAM config send load mode cmd failed.\n"); 
        return RT_ERROR; 
    }

    ret = HAL_SDRAM_ProgramRefreshRate(&SDRAM_Handle, refresh_count);
    if(ret != HAL_OK)
    {
        rt_kprintf("Programs the SDRAM Memory Refresh rate failed.\n"); 
        return RT_ERROR; 
    }
    
    return RT_EOK; 
}

static rt_err_t stm32_sdram_init(void)
{
    /* Init clock */ 
    __HAL_RCC_FMC_CLK_ENABLE();
    
    __HAL_RCC_GPIOC_CLK_ENABLE(); 
    __HAL_RCC_GPIOD_CLK_ENABLE(); 
    __HAL_RCC_GPIOE_CLK_ENABLE(); 
    __HAL_RCC_GPIOF_CLK_ENABLE(); 
    __HAL_RCC_GPIOG_CLK_ENABLE(); 
    __HAL_RCC_GPIOH_CLK_ENABLE(); 
    __HAL_RCC_GPIOI_CLK_ENABLE(); 
    
    /* Init gpio */ 
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.Mode      = GPIO_MODE_AF_PP; 
    GPIO_InitStructure.Pull      = GPIO_PULLUP; 
    GPIO_InitStructure.Speed     = GPIO_SPEED_FAST; 
    GPIO_InitStructure.Alternate = GPIO_AF12_FMC; 
    
    GPIO_InitStructure.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_InitStructure.Pin = GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0  | GPIO_PIN_1  | \
                             GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    GPIO_InitStructure.Pin = GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | \
                             GPIO_PIN_11 |GPIO_PIN_12  | GPIO_PIN_13 | GPIO_PIN_14 | \
                             GPIO_PIN_15 | GPIO_PIN_0  | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);
    
    GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  | \
                             GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_12 | GPIO_PIN_13 | \
                             GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_11; 
    HAL_GPIO_Init(GPIOF, &GPIO_InitStructure); 
    
    GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_4  | GPIO_PIN_5  | \
                             GPIO_PIN_8  | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);
    
    GPIO_InitStructure.Pin = GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 | \
                             GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | \
                             GPIO_PIN_2  | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  | \
                             GPIO_PIN_6  | GPIO_PIN_7  | GPIO_PIN_9  | GPIO_PIN_10 | \
                             GPIO_PIN_4  | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);
    
    /* Init sdram */ 
    SDRAM_Handle.Instance = FMC_SDRAM_DEVICE; 
    
    TimStructure.LoadToActiveDelay       = 2; 
    TimStructure.ExitSelfRefreshDelay    = 7; 
    TimStructure.SelfRefreshTime         = 4; 
    TimStructure.RowCycleDelay           = 7; 
    TimStructure.WriteRecoveryTime       = 2; 
    TimStructure.RPDelay                 = 2; 
    TimStructure.RCDDelay                = 2; 
    
    SDRAM_Handle.Init.SDBank             = FMC_SDRAM_BANK1; 
    SDRAM_Handle.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8; 
    SDRAM_Handle.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12; 
    SDRAM_Handle.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32; 
    SDRAM_Handle.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4; 
    SDRAM_Handle.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3; 
    SDRAM_Handle.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE; 
    
    SDRAM_Handle.Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2; 
    SDRAM_Handle.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE; 
    SDRAM_Handle.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0; 
    
    if(HAL_SDRAM_Init(&SDRAM_Handle, &TimStructure) != HAL_OK)
    {
        rt_kprintf("Init STM32 sdram handle failed.\n"); 
        return RT_ERROR; 
    }
    
    if(stm32_sdram_init_seq(SDRAM_REFRESH_COUNT) != RT_EOK)
    {
        rt_kprintf("Init STM32 sdram seq failed.\n"); 
        return RT_ERROR; 
    } 
    
    return RT_EOK; 
}

int rt_hw_sdram_init(void)
{
    rt_err_t ret = RT_EOK; 
    
    ret = stm32_sdram_init(); 
    if(ret != RT_EOK)
    {
        rt_kprintf("Init STM32 sdram failed.\n"); 
        return RT_ERROR; 
    }
    
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_sdram_init); 
