/*
 * File      : drv_lcd.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author         Notes
 * 2018-07-28     liu2guang      the first version for STM32F469NI-Discovery.
 */

#include "drv_lcd.h"

#define LCD_RESET_PIN (103)

/* 横屏模式 */
#define LCD_WIDTH     ((rt_uint32_t)480)
#define LCD_HEIGHT    ((rt_uint32_t)800)

#define LCD_HSYNC     ((rt_uint32_t) 2)
#define LCD_HFP       ((rt_uint32_t)34)
#define LCD_HBP       ((rt_uint32_t)34)

#define LCD_VSYNC     ((rt_uint32_t) 1)
#define LCD_VFP       ((rt_uint32_t)15)
#define LCD_VBP       ((rt_uint32_t)16)

LTDC_HandleTypeDef  LTDC_Handle; 
DSI_HandleTypeDef   DSI_Handle; 
DSI_VidCfgTypeDef   DSI_VideoStru; 
DMA2D_HandleTypeDef DMA2D_Handle; 

#define LCD_FB_START_ADDRESS ((uint32_t)0xC0800000)

static rt_err_t otm8009a_init(void); 
static void otm8009a_reset(void); 
static void otm8009a_display_on(void); 
static void otm8009a_display_off(void); 
static void otm8009a_config(rt_uint32_t color_coding); 

/* 长数据的格式 */
static const rt_uint8_t l_data01[] = {0x80, 0x09, 0x01, 0xFF};
static const rt_uint8_t l_data02[] = {0x80, 0x09, 0xFF};
static const rt_uint8_t l_data03[] = {0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01, 0xE1};
static const rt_uint8_t l_data04[] = {0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A, 0x01, 0xE2};
static const rt_uint8_t l_data05[] = {0x79, 0x79, 0xD8};
static const rt_uint8_t l_data06[] = {0x00, 0x01, 0xB3};
static const rt_uint8_t l_data07[] = {0x85, 0x01, 0x00, 0x84, 0x01, 0x00, 0xCE};
static const rt_uint8_t l_data08[] = {0x18, 0x04, 0x03, 0x39, 0x00, 0x00, 0x00, 0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00, 0xCE};
static const rt_uint8_t l_data09[] = {0x18, 0x02, 0x03, 0x3B, 0x00, 0x00, 0x00, 0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00, 0xCE};
static const rt_uint8_t l_data10[] = {0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0xCF};
static const rt_uint8_t l_data11[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data12[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data13[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data14[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data15[] = {0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data16[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data17[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB};
static const rt_uint8_t l_data18[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCB};
static const rt_uint8_t l_data19[] = {0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0xCC};
static const rt_uint8_t l_data20[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02, 0xCC};
static const rt_uint8_t l_data21[] = {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC};
static const rt_uint8_t l_data22[] = {0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00, 0xCC};
static const rt_uint8_t l_data23[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01, 0xCC};
static const rt_uint8_t l_data24[] = {0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC};
static const rt_uint8_t l_data25[] = {0xFF, 0xFF, 0xFF, 0xFF};
static const rt_uint8_t l_data27[] = {0x00, 0x00, 0x03, 0x1F, 0x2A};
static const rt_uint8_t l_data28[] = {0x00, 0x00, 0x01, 0xDF, 0x2B};

/* 短数据的格式 */
static const rt_uint8_t s_data01[] = {0x00, 0x00};
static const rt_uint8_t s_data02[] = {0x00, 0x80};
static const rt_uint8_t s_data03[] = {0xC4, 0x30};
static const rt_uint8_t s_data04[] = {0x00, 0x8A};
static const rt_uint8_t s_data05[] = {0xC4, 0x40};
static const rt_uint8_t s_data06[] = {0x00, 0xB1};
static const rt_uint8_t s_data07[] = {0xC5, 0xA9};
static const rt_uint8_t s_data08[] = {0x00, 0x91};
static const rt_uint8_t s_data09[] = {0xC5, 0x34};
static const rt_uint8_t s_data10[] = {0x00, 0xB4};
static const rt_uint8_t s_data11[] = {0xC0, 0x50};
static const rt_uint8_t s_data12[] = {0xD9, 0x4E};
static const rt_uint8_t s_data13[] = {0x00, 0x81};
static const rt_uint8_t s_data14[] = {0xC1, 0x66};
static const rt_uint8_t s_data15[] = {0x00, 0xA1};
static const rt_uint8_t s_data16[] = {0xC1, 0x08};
static const rt_uint8_t s_data17[] = {0x00, 0x92};
static const rt_uint8_t s_data18[] = {0xC5, 0x01};
static const rt_uint8_t s_data19[] = {0x00, 0x95};
static const rt_uint8_t s_data20[] = {0x00, 0x94};
static const rt_uint8_t s_data21[] = {0xC5, 0x33};
static const rt_uint8_t s_data22[] = {0x00, 0xA3};
static const rt_uint8_t s_data23[] = {0xC0, 0x1B};
static const rt_uint8_t s_data24[] = {0x00, 0x82};
static const rt_uint8_t s_data25[] = {0xC5, 0x83};
static const rt_uint8_t s_data26[] = {0xC4, 0x83};
static const rt_uint8_t s_data27[] = {0xC1, 0x0E};
static const rt_uint8_t s_data28[] = {0x00, 0xA6};
static const rt_uint8_t s_data29[] = {0x00, 0xA0};
static const rt_uint8_t s_data30[] = {0x00, 0xB0};
static const rt_uint8_t s_data31[] = {0x00, 0xC0};
static const rt_uint8_t s_data32[] = {0x00, 0xD0};
static const rt_uint8_t s_data33[] = {0x00, 0x90};
static const rt_uint8_t s_data34[] = {0x00, 0xE0};
static const rt_uint8_t s_data35[] = {0x00, 0xF0};
static const rt_uint8_t s_data36[] = {0x11, 0x00};
static const rt_uint8_t s_data37[] = {0x3A, 0x55};
static const rt_uint8_t s_data38[] = {0x3A, 0x77};
static const rt_uint8_t s_data39[] = {0x36, 0x60};
static const rt_uint8_t s_data40[] = {0x51, 0x7F};
static const rt_uint8_t s_data41[] = {0x53, 0x2C};
static const rt_uint8_t s_data42[] = {0x55, 0x02};
static const rt_uint8_t s_data43[] = {0x5E, 0xFF};
static const rt_uint8_t s_data44[] = {0x29, 0x00};
static const rt_uint8_t s_data45[] = {0x2C, 0x00};
static const rt_uint8_t s_data46[] = {0xCF, 0x00};
static const rt_uint8_t s_data47[] = {0xC5, 0x66};
static const rt_uint8_t s_data48[] = {0x00, 0xB6};
static const rt_uint8_t s_data49[] = {0xF5, 0x06};
static const rt_uint8_t s_data50[] = {0x00, 0xB1};
static const rt_uint8_t s_data51[] = {0xC6, 0x06};

static rt_err_t otm8009a_init(void)
{
    /* otm8009a reset pin */ 
    rt_pin_mode(LCD_RESET_PIN, PIN_MODE_OUTPUT); 
    
    uint32_t LcdClk          = 27429;
    uint32_t LaneByteClk_kHz = 62500;

    uint32_t HSA = LCD_HSYNC, HFP = LCD_HFP, HBP = LCD_HBP, HACT = LCD_WIDTH;
    uint32_t VSA = LCD_VSYNC, VFP = LCD_VFP, VBP = LCD_VBP, VACT = LCD_HEIGHT;
    
    /* Reset */
    otm8009a_reset(); 
    
    /* Init lcd clock */ 
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();

    __HAL_RCC_DMA2D_CLK_ENABLE();
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();

    __HAL_RCC_DSI_CLK_ENABLE();
    __HAL_RCC_DSI_FORCE_RESET();
    __HAL_RCC_DSI_RELEASE_RESET(); 
    
    HAL_NVIC_SetPriority(LTDC_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(LTDC_IRQn);
    
    HAL_NVIC_SetPriority(DMA2D_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DMA2D_IRQn);
    
    HAL_NVIC_SetPriority(DSI_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DSI_IRQn);

    /* INIT DSI */ 
    DSI_PLLInitTypeDef DSI_PLLInit; 
    DSI_PHY_TimerTypeDef PHY_Timings;
    
    DSI_Handle.Instance = DSI; 
    DSI_Handle.Init.TXEscapeCkdiv = LaneByteClk_kHz / 15620;
    DSI_Handle.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
    
    DSI_PLLInit.PLLNDIV = 125;
    DSI_PLLInit.PLLIDF  = DSI_PLL_IN_DIV2;
    DSI_PLLInit.PLLODF  = DSI_PLL_OUT_DIV1; 
    
    HAL_DSI_DeInit(&DSI_Handle);
    HAL_DSI_Init(&DSI_Handle, &DSI_PLLInit); 
    
    DSI_VideoStru.VirtualChannelID = 0;
    DSI_VideoStru.ColorCoding      = DSI_RGB888;
    //DSI_VideoStru.LooselyPacked    = DSI_LOOSELY_PACKED_DISABLE;
    DSI_VideoStru.Mode             = DSI_VID_MODE_BURST;
    DSI_VideoStru.PacketSize       = HACT;
    DSI_VideoStru.NumberOfChunks   = 0;
    DSI_VideoStru.NullPacketSize   = 0xFFF;

    DSI_VideoStru.HSPolarity       = DSI_HSYNC_ACTIVE_HIGH;
    DSI_VideoStru.VSPolarity       = DSI_VSYNC_ACTIVE_HIGH;
    DSI_VideoStru.DEPolarity       = DSI_DATA_ENABLE_ACTIVE_HIGH;

    DSI_VideoStru.HorizontalSyncActive = HSA * (LaneByteClk_kHz / LcdClk);
    DSI_VideoStru.HorizontalBackPorch  = HBP * (LaneByteClk_kHz / LcdClk);
    DSI_VideoStru.HorizontalLine       = (HACT + HSA + HBP + HFP) * (LaneByteClk_kHz / LcdClk);
    DSI_VideoStru.VerticalSyncActive   = VSA;
    DSI_VideoStru.VerticalBackPorch    = VBP;
    DSI_VideoStru.VerticalFrontPorch   = VFP;
    DSI_VideoStru.VerticalActive       = VACT;

    DSI_VideoStru.LPCommandEnable = DSI_LP_COMMAND_ENABLE;

    DSI_VideoStru.LPLargestPacketSize     = 16;
    DSI_VideoStru.LPVACTLargestPacketSize = 0;

    DSI_VideoStru.LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;
    DSI_VideoStru.LPHorizontalBackPorchEnable  = DSI_LP_HBP_ENABLE;
    DSI_VideoStru.LPVerticalActiveEnable       = DSI_LP_VACT_ENABLE;
    DSI_VideoStru.LPVerticalFrontPorchEnable   = DSI_LP_VFP_ENABLE;
    DSI_VideoStru.LPVerticalBackPorchEnable    = DSI_LP_VBP_ENABLE;
    DSI_VideoStru.LPVerticalSyncActiveEnable   = DSI_LP_VSYNC_ENABLE;

    HAL_DSI_ConfigVideoMode(&DSI_Handle, &DSI_VideoStru); 
    
    PHY_Timings.ClockLaneHS2LPTime  = 35;
    PHY_Timings.ClockLaneLP2HSTime  = 35;
    PHY_Timings.DataLaneHS2LPTime   = 35;
    PHY_Timings.DataLaneLP2HSTime   = 35;
    PHY_Timings.DataLaneMaxReadTime = 0;
    PHY_Timings.StopWaitTime        = 10;
    HAL_DSI_ConfigPhyTimer(&DSI_Handle, &PHY_Timings);
    
    /* INIT LTDC */ 
    static RCC_PeriphCLKInitTypeDef PeriphClkInitStruct; 
    
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN       = 384;
    PeriphClkInitStruct.PLLSAI.PLLSAIR       = 7;
    PeriphClkInitStruct.PLLSAIDivR           = RCC_PLLSAIDIVR_2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    LTDC_Handle.Instance                = LTDC; 
    LTDC_Handle.Init.PCPolarity         = LTDC_PCPOLARITY_IPC;
    LTDC_Handle.Init.HorizontalSync     = (HSA - 1);
    LTDC_Handle.Init.AccumulatedHBP     = (HSA + HBP - 1);
    LTDC_Handle.Init.AccumulatedActiveW = (LCD_WIDTH  + HSA + HBP - 1);
    LTDC_Handle.Init.TotalWidth         = (LCD_HEIGHT + HSA + HBP + HFP - 1);
    
    LTDC_Handle.Init.Backcolor.Red      = 30;
    LTDC_Handle.Init.Backcolor.Green    = 160;
    LTDC_Handle.Init.Backcolor.Blue     = 132;
    
    LTDC_Handle.LayerCfg->ImageWidth  = LCD_WIDTH;
    LTDC_Handle.LayerCfg->ImageHeight = LCD_HEIGHT;

    HAL_LTDCEx_StructInitFromVideoConfig(&LTDC_Handle, &DSI_VideoStru);
    HAL_LTDC_Init(&LTDC_Handle); 
    
    HAL_DSI_Start(&DSI_Handle);
    
    /* Config otm8009a */ 
    otm8009a_config(DSI_VideoStru.ColorCoding); 
    
    return RT_EOK;
}

static void otm8009a_reset(void)
{
    rt_pin_write(LCD_RESET_PIN, PIN_LOW);
    rt_thread_delay(rt_tick_from_millisecond(20));
    
    rt_pin_write(LCD_RESET_PIN, PIN_HIGH);
    rt_thread_delay(rt_tick_from_millisecond(20));
}

static void otm8009a_display_on(void)
{
    HAL_DSI_ShortWrite(&DSI_Handle, DSI_VideoStru.VirtualChannelID, DSI_DCS_SHORT_PKT_WRITE_P1, 0x29, 0x00);
}

static void otm8009a_display_off(void)
{
    HAL_DSI_ShortWrite(&DSI_Handle, DSI_VideoStru.VirtualChannelID, DSI_DCS_SHORT_PKT_WRITE_P1, 0x28, 0x00);
}

static void otm8009a_send_cmd(rt_uint8_t *params, rt_uint32_t num)
{
    if(num <= 1)
    {
        HAL_DSI_ShortWrite(&DSI_Handle, DSI_VideoStru.VirtualChannelID, 
            DSI_DCS_SHORT_PKT_WRITE_P1, params[0], params[1]);
    }
    else
    {
        HAL_DSI_LongWrite(&DSI_Handle, DSI_VideoStru.VirtualChannelID, 
            DSI_DCS_LONG_PKT_WRITE, num, params[num], params);
    }
}

void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
    if(NbrParams <= 1)
    {
        HAL_DSI_ShortWrite(&DSI_Handle, DSI_VideoStru.VirtualChannelID, 
            DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]);
    }
    else
    {
        HAL_DSI_LongWrite(&DSI_Handle, DSI_VideoStru.VirtualChannelID, 
            DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams], pParams);
    }
}

static void otm8009a_config(rt_uint32_t color_coding)
{
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data01, 3);

    otm8009a_send_cmd((rt_uint8_t *)s_data02, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data02, 2);

    otm8009a_send_cmd((rt_uint8_t *)s_data02, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data03, 0);
    rt_thread_delay(rt_tick_from_millisecond(10));
    
    otm8009a_send_cmd((rt_uint8_t *)s_data04, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data05, 0);
    rt_thread_delay(rt_tick_from_millisecond(10));

    otm8009a_send_cmd((rt_uint8_t *)s_data06, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data07, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data08, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data09, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data10, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data11, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data12, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data13, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data14, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data15, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data16, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data17, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data18, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data19, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data09, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data05, 2);
    otm8009a_send_cmd((rt_uint8_t *)s_data20, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data21, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data22, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data23, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data24, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data25, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data13, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data26, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data15, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data27, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data28, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data06, 2);
    otm8009a_send_cmd((rt_uint8_t *)s_data02, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data07, 6);
    otm8009a_send_cmd((rt_uint8_t *)s_data29, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data08, 14);
    otm8009a_send_cmd((rt_uint8_t *)s_data30, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data09, 14);
    otm8009a_send_cmd((rt_uint8_t *)s_data31, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data10, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data32, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data46, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data02, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data11, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data33, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data12, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data29, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data13, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data30, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data14, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data31, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data15, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data32, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data16, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data34, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data17, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data35, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data18, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data02, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data19, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data33, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data20, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data29, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data21, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data30, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data22, 10);
    otm8009a_send_cmd((rt_uint8_t *)s_data31, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data23, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data32, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data24, 15);
    otm8009a_send_cmd((rt_uint8_t *)s_data13, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data47, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data48, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data49, 0);
    
    otm8009a_send_cmd((rt_uint8_t *)s_data50, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data51, 0);
    
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data25, 3);
    
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data03, 16);
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data04, 16);
    otm8009a_send_cmd((rt_uint8_t *)s_data36, 0);
    rt_thread_delay(rt_tick_from_millisecond(120));

    switch(color_coding)
    {
    case DSI_RGB565:
        otm8009a_send_cmd((rt_uint8_t *)s_data37, 0);
        break;

    case DSI_RGB888:
        otm8009a_send_cmd((rt_uint8_t *)s_data38, 0);
        break;
    }

    /* LCD_LANDSCAPE */ 
    otm8009a_send_cmd((rt_uint8_t *)s_data39, 0);
    otm8009a_send_cmd((rt_uint8_t *)l_data27, 4);
    otm8009a_send_cmd((rt_uint8_t *)l_data28, 4);

    otm8009a_send_cmd((rt_uint8_t *)s_data40, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data41, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data42, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data43, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data44, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data01, 0);
    otm8009a_send_cmd((rt_uint8_t *)s_data45, 0);
}

void LTDC_IRQHandler(void)
{
    HAL_LTDC_IRQHandler(&LTDC_Handle);
}

struct stm32_lcd
{
    struct rt_device device;
    struct rt_device_graphic_info info; 
    
    rt_uint32_t *frame_buffer; 
};
static struct stm32_lcd lcd;

static rt_err_t stm32_lcd_init(rt_device_t device)
{
    otm8009a_init(); 
    
    lcd.info.width          = LCD_WIDTH;
    lcd.info.height         = LCD_HEIGHT;
    lcd.info.pixel_format   = RTGRAPHIC_PIXEL_FORMAT_ARGB888;
    lcd.info.bits_per_pixel = 32;
    lcd.info.framebuffer    = (void *)(lcd.frame_buffer); 
    
    return RT_EOK;
}

static rt_err_t stm32_lcd_control(rt_device_t device, int cmd, void *args)
{
    switch(cmd)
    {
    case RTGRAPHIC_CTRL_RECT_UPDATE:
        break;

    case RTGRAPHIC_CTRL_POWERON: 
        otm8009a_display_on(); 
        break;

    case RTGRAPHIC_CTRL_POWEROFF: 
        otm8009a_display_off(); 
        break;

    case RTGRAPHIC_CTRL_GET_INFO:
        rt_memcpy(args, &lcd.info, sizeof(lcd.info));
        break;

    case RTGRAPHIC_CTRL_SET_MODE:
        break;

    case RTGRAPHIC_CTRL_GET_EXT:
        break;
    }

    return RT_EOK;
}

int rt_hw_lcd_init(void)
{
    rt_err_t ret; 
    
    rt_memset(&lcd, 0x00, sizeof(lcd)); 

    lcd.device.type    = RT_Device_Class_Graphic;
    lcd.device.init    = stm32_lcd_init;
    lcd.device.open    = RT_NULL;
    lcd.device.close   = RT_NULL;
    lcd.device.read    = RT_NULL;
    lcd.device.write   = RT_NULL;
    lcd.device.control = stm32_lcd_control;

    lcd.device.user_data = (void *)&lcd.info;

    ret = rt_device_register(&lcd.device, "lcd", RT_DEVICE_FLAG_RDWR);

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_lcd_init);
