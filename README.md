#  STM32F469NI

## 1. 简介

该 BSP 是基于官方的 STM32F469IDISCOVERY探索板 定制开发, 具备以下简要的特性：

| 介绍 | 描述 |
| ---- | ---- |
| 主CPU平台 | ARM Cortex-M4 |
| 最高频率 | 214MHz |
| 内部存储器 | 2MB Flash 384KB+4KB RAM |
| 外部存储器 | 16MB 32bit SDRAM 8MB QSPI |

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
| GPIO | 支持 | 支持所有GPIO口 |
| IIC | 支持部分 | 支持模拟IIC2总线 |
| SPI | 支持部分 | 支持SPI1/2/3 但是未测试且SPI4/5/6待添加 |
| ETH | 无法支持 | 板子上没有ETH接口 |
| LCD | 支持 | 目前只支持单framebuffer模式 |
| RTC | 不支持 | |
| SDCARD | 支持 | 待优化DMA中断处理方式, 目前DMA是阻塞读写, 非常不合理, 但是目前驱动非常稳定, 同时也支持SDIO BUS驱动, 这里优化了ST HAL SD非对齐问题, 注意复制的时候同时修改这部分 |
| SDRAM | 支持 | 16M SDRAM，后面8M作为LCD FB区域 |
| AUDIO | 开发中 | 目前已经可以播放wav音乐, 待优化api, 对接RTT audio框架 |
| USB | 支持 | 支持主机和从机 但是待编写Kconfig |
| RAMDISK | 支持 | 支持自动挂载/mnt/tmp, 默认4MBytes大小, 可以再menuconfig中配置是否自动挂载, 挂载路径, ramdisk大小 |
| TIM | 不支持 | |
| QSPI | 不支持 | 计划中 |
| WDG | 不支持 | |
| ADC | 不支持 | |
| CAN | 不支持 | |
| DAC | 不支持 | |
| DCMI | 不支持 | |
| PWM | 不支持 | |

## 4. 组件支持情况及计划 

| 组件 | 支持情况  | 备注 |
| :------ | :----  | :------ |
| FAL | 未知 |  |
| Littlevgl2RTT | 支持 |  | 
| Lua2RTT | 支持 |  | 
| optparse | 支持 |  | 
| libcsv | 支持 |  | 
| linmath | 支持 |  | 
| dwin | 支持 |  | 
| id3v2lib | 支持 |  | 
| bsdiff | 支持 |  | 
| MultiButton | 支持 |  | 

## 5. 联系人信息

维护人：
- [liu2guang](https://github.com/liu2guang)
