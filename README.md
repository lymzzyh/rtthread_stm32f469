#  STM32F469NI

## 1. 简介

BSP默认支持的STM32F469NI处理器具备以下简要的特性：

| 介绍 | 描述 |
| ---- | ---- |
| 主CPU平台 | ARM Cortex-M4 |
| 最高频率 | 214MHz |
| 内部存储器 | 2MB Flash 384KB+4KB RAM |

## 2. 编译说明

STM32F469NI板级包支持MDK5开发环境和GCC编译器，以下是具体版本信息：

| IDE/编译器 | 已测试版本 |
| ---------- | --------- |
| MDK5 | MDK522 |
| GCC | GCC 5.4.1 20160919 (release) |
| IAR | 非常讨厌IAR开发环境, 需要的自己去处理 |

## 3. 驱动支持情况及计划

| 驱动 | 支持情况  | 备注 |
| :------ | :----  | :------ |
| UART | 支持部分 | 目前支持串口1/2/3/6, 4/5/7/8待加入 |
| GPIO | 支持 | |
| IIC | 支持部分 | 支持模拟IIC2总线 |
| SPI | 支持部分 | 支持SPI1/2/3 但是未测试且SPI4/5/6待添加 |
| ETH | 不支持 | |
| LCD | 支持 | 目前只支持单framebuffer模式 |
| RTC | 不支持 | |
| SDCARD | 支持 | 待优化DMA中断处理方式, 目前DMA是阻塞读写, 非常不合理, 但是目前驱动非常稳定 |
| SDRAM | 支持 | 16M SDRAM，后面8M作为LCD FB区域 |
| AUDIO | 开发中 | 目前已经可以播放wav音乐, 待优化api, 对接RTT audio框架 |
| USB | 支持 | 支持主机和从机 但是待编写Kconfig |
| TIM | 不支持 | |
| QSPI | 不支持 | 计划中 |
| WDG | 不支持 | |
| ADC | 不支持 | |
| CAN | 不支持 | |
| DAC | 不支持 | |
| DCMI | 不支持 | |

## 4. 组件支持情况及计划 

| 组件 | 支持情况  | 备注 |
| :------ | :----  | :------ |
| FAL | 未知 |  |
| Littlevgl2RTT | 未知 |  | 
| Lua2RTT | 未知 |  | 
| optparse | 未知 |  | 
| libcsv | 未知 |  | 
| linmath | 未知 |  | 
| dwin | 未知 |  | 
| id3v2lib | 未知 |  | 
| bsdiff | 未知 |  | 
| MultiButton | 未知 |  | 

## 5. 联系人信息

维护人：
- [liu2guang](https://github.com/liu2guang)
