#  STM32F469NI

## 1. 简介

BSP默认支持的STM32F469NI处理器具备以下简要的特性：

| 介绍 | 描述 |
| ---- | ---- |
| 主CPU平台 | ARM Cortex-M4 |
| 最高频率 | 214MHz |
| 内部存储器 | 2MB FLASH 384B SRAM |
| 外部存储器接口 |  |

## 2. 编译说明

i.MX RT1050板级包支持MDK5﹑IAR开发环境和GCC编译器，以下是具体版本信息：

| IDE/编译器 | 已测试版本 |
| ---------- | --------- |
| MDK5 | MDK522 |
| GCC | GCC 5.4.1 20160919 (release) |

## 3. 驱动支持情况及计划

| 驱动 | 支持情况  | 备注 |
| ------ | ----  | ------ |
| UART | 支持 | |
| GPIO | 支持 | |
| IIC | 支持 | 模拟IIC2总线 |
| SPI | 不支持 |  |
| ETH | 不支持 | |
| LCD | 支持 | |
| RTC | 不支持 | |
| SDCARD | 支持 |  |
| SDRAM | 支持 | 32M SDRAM，后面2M作为Non Cache区域 |
| AUDIO | 不支持 | 32M SDRAM，后面2M作为Non Cache区域 |

## 5. 联系人信息

维护人：

- [liu2guang](https://github.com/liu2guang)

## 6. 参考
